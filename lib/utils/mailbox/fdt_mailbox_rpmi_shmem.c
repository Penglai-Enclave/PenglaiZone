/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_timer.h>
#include <sbi/riscv_io.h>
#include <sbi/riscv_locks.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mailbox/mailbox.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

/**************** RPMI Transport Structures and Macros ***********/

#define SIZE_WRITEIDX			(4)	/* bytes */
#define SIZE_READIDX			(4)	/* bytes */
#define RPMI_QUEUE_HEADER_SIZE		(SIZE_WRITEIDX + SIZE_READIDX)
#define RPMI_MAILBOX_CHANNELS_MAX	(16)

#define GET_TOKEN(msg)				\
({						\
	struct rpmi_message *mbuf = msg;	\
	le32_to_cpu(mbuf->header.token);	\
})

#define GET_MESSAGE_ID(msg)						\
({									\
	le32_to_cpu(*(u32 *)((char *)msg + RPMI_MSG_IDN_OFFSET));	\
})

#define GET_ENTITY_ID(msg)						\
({									\
	le32_to_cpu(*(u32 *)((char *)msg + RPMI_MSG_ENTYID_OFFSET));	\
})

#define GET_MESSAGE_TYPE(msg)						\
({									\
	u32 msgidn = *(u32 *)((char *)msg + RPMI_MSG_IDN_OFFSET);	\
	le32_to_cpu((u32)((msgidn & RPMI_MSG_IDN_TYPE_MASK) >>		\
			  RPMI_MSG_IDN_TYPE_POS));			\
})

#define GET_SERVICE_ID(msg)						\
({									\
	u32 msgidn = *(u32 *)((char *)msg + RPMI_MSG_IDN_OFFSET);	\
	le32_to_cpu((u32)((msgidn & RPMI_MSG_IDN_SERVICE_ID_MASK) >>	\
					RPMI_MSG_IDN_SERVICE_ID_POS));	\
})

#define GET_SERVICEGROUP_ID(msg)					\
({									\
	u32 msgidn = *(u32 *)((char *)msg + RPMI_MSG_IDN_OFFSET);	\
	le32_to_cpu((u32)((msgidn & RPMI_MSG_IDN_SERVICEGROUP_ID_MASK) >>\
				RPMI_MSG_IDN_SERVICEGROUP_ID_POS));	\
})

#define GET_DLEN(msg)				\
({						\
	struct rpmi_message *mbuf = msg;	\
	le32_to_cpu(mbuf->header.datalen);	\
})

enum rpmi_queue_type {
	RPMI_QUEUE_TYPE_REQ = 0,
	RPMI_QUEUE_TYPE_ACK = 1,
};

enum rpmi_queue_idx {
	RPMI_QUEUE_IDX_A2P_REQ = 0,
	RPMI_QUEUE_IDX_P2A_ACK = 1,
	RPMI_QUEUE_IDX_P2A_REQ = 2,
	RPMI_QUEUE_IDX_A2P_ACK = 3,
	RPMI_QUEUE_IDX_MAX_COUNT,
};

enum rpmi_reg_idx {
	RPMI_REG_IDX_DB_REG = 0, /* Doorbell register */
	RPMI_REG_IDX_MAX_COUNT,
};

/** Single Queue Memory View */
struct rpmi_queue {
	volatile le32_t readidx;
	volatile le32_t writeidx;
	uint8_t buffer[];
} __packed;

/** Mailbox registers */
struct rpmi_mb_regs {
	/* doorbell from AP -> PuC*/
	volatile le32_t db_reg;
} __packed;

/** Single Queue Context Structure */
struct smq_queue_ctx {
	u32 queue_id;
	u32 num_slots;
	spinlock_t queue_lock;
	/* Type of queue - REQ or ACK */
	enum rpmi_queue_type queue_type;
	/* Pointer to the queue shared memory */
	struct rpmi_queue *queue;
	char name[RPMI_NAME_CHARS_MAX];
};

struct rpmi_shmem_mbox_controller {
	/* Driver specific members */
	u32 slot_size;
	u32 queue_count;
	struct rpmi_mb_regs *mb_regs;
	struct smq_queue_ctx queue_ctx_tbl[RPMI_QUEUE_IDX_MAX_COUNT];
	/* Mailbox framework related members */
	struct mbox_controller controller;
	struct mbox_chan channels[RPMI_MAILBOX_CHANNELS_MAX];
	struct mbox_chan *base_chan;
	u32 impl_version;
	u32 impl_id;
	u32 spec_version;
	struct {
		bool f0_ev_notif_en;
		bool f0_msi_en;
	} base_flags;
};

/**************** Shared Memory Queues Helpers **************/

static bool __smq_queue_full(struct smq_queue_ctx *qctx)
{
	return
	((le32_to_cpu(qctx->queue->writeidx) + 1) % qctx->num_slots ==
			le32_to_cpu(qctx->queue->readidx)) ? true : false;
}

static bool __smq_queue_empty(struct smq_queue_ctx *qctx)
{
	return
	(le32_to_cpu(qctx->queue->readidx) ==
		le32_to_cpu(qctx->queue->writeidx)) ? true : false;
}

static int __smq_rx(struct smq_queue_ctx *qctx, u32 slot_size,
		    u32 service_group_id, struct mbox_xfer *xfer)
{
	void *dst, *src;
	struct rpmi_message *msg;
	u32 i, tmp, pos, dlen, msgidn, readidx, writeidx;
	struct rpmi_message_args *args = xfer->args;
	bool no_rx_token = (args->flags & RPMI_MSG_FLAGS_NO_RX_TOKEN) ?
			   true : false;

	/* Rx sanity checks */
	if ((sizeof(u32) * args->rx_endian_words) >
	    (slot_size - sizeof(struct rpmi_message_header)))
		return SBI_EINVAL;
	if ((sizeof(u32) * args->rx_endian_words) > xfer->rx_len)
		return SBI_EINVAL;

	/* There should be some message in the queue */
	if (__smq_queue_empty(qctx))
		return SBI_ENOENT;

	/* Get the read index and write index */
	readidx = le32_to_cpu(qctx->queue->readidx);
	writeidx = le32_to_cpu(qctx->queue->writeidx);

	/*
	 * Compute msgidn expected in the incoming message
	 * NOTE: DOORBELL bit is not expected to be set.
	 */
	msgidn = ((service_group_id << RPMI_MSG_IDN_SERVICEGROUP_ID_POS) &
		  RPMI_MSG_IDN_SERVICEGROUP_ID_MASK) |
		 ((args->service_id << RPMI_MSG_IDN_SERVICE_ID_POS) &
		  RPMI_MSG_IDN_SERVICE_ID_MASK) |
		 ((args->type << RPMI_MSG_IDN_TYPE_POS) &
		  RPMI_MSG_IDN_TYPE_MASK);

	/* Find the Rx message with matching token */
	pos = readidx;
	while (pos != writeidx) {
		src = (void *)qctx->queue->buffer + (pos * slot_size);
		if ((no_rx_token && GET_MESSAGE_ID(src) == msgidn) ||
		    (GET_TOKEN(src) == xfer->seq))
			break;
		pos = (pos + 1) % qctx->num_slots;
	}
	if (pos == writeidx)
		return SBI_ENOENT;

	/* If Rx message is not first message then make it first message */
	if (pos != readidx) {
		src = (void *)qctx->queue->buffer + (pos * slot_size);
		dst = (void *)qctx->queue->buffer + (readidx * slot_size);
		for (i = 0; i < slot_size / sizeof(u32); i++) {
			tmp = ((u32 *)dst)[i];
			((u32 *)dst)[i] = ((u32 *)src)[i];
			((u32 *)src)[i] = tmp;
		}
	}

	/* Update rx_token if not available */
	msg = (void *)qctx->queue->buffer + (readidx * slot_size);
	if (no_rx_token)
		args->rx_token = GET_TOKEN(msg);

	/* Extract data from the first message */
	if (xfer->rx) {
		args->rx_data_len = dlen = GET_DLEN(msg);
		if (dlen > xfer->rx_len)
			dlen = xfer->rx_len;
		src = (void *)msg + sizeof(struct rpmi_message_header);
		dst = xfer->rx;
		for (i = 0; i < args->rx_endian_words; i++)
			((u32 *)dst)[i] = le32_to_cpu(((u32 *)src)[i]);
		dst += sizeof(u32) * args->rx_endian_words;
		src += sizeof(u32) * args->rx_endian_words;
		sbi_memcpy(dst, src,
			xfer->rx_len - (sizeof(u32) * args->rx_endian_words));
	}

	/* Update the read index */
	qctx->queue->readidx = cpu_to_le32(readidx + 1) % qctx->num_slots;
	smp_wmb();

	return SBI_OK;
}

static int __smq_tx(struct smq_queue_ctx *qctx, struct rpmi_mb_regs *mb_regs,
		    u32 slot_size, u32 service_group_id, struct mbox_xfer *xfer)
{
	void *dst, *src;
	u32 i, msgidn, writeidx;
	struct rpmi_message_header header = { 0 };
	struct rpmi_message_args *args = xfer->args;

	/* Tx sanity checks */
	if ((sizeof(u32) * args->tx_endian_words) >
	    (slot_size - sizeof(struct rpmi_message_header)))
		return SBI_EINVAL;
	if ((sizeof(u32) * args->tx_endian_words) > xfer->tx_len)
		return SBI_EINVAL;

	/* There should be some room in the queue */
	if (__smq_queue_full(qctx))
		return SBI_ENOMEM;

	/* Get the write index */
	writeidx = le32_to_cpu(qctx->queue->writeidx);

	/*
	 * Compute msgidn for the outgoing message
	 * NOTE: DOORBELL bit is not set currently because we always poll.
	 */
	msgidn = ((service_group_id << RPMI_MSG_IDN_SERVICEGROUP_ID_POS) &
		  RPMI_MSG_IDN_SERVICEGROUP_ID_MASK) |
		 ((args->service_id << RPMI_MSG_IDN_SERVICE_ID_POS) &
		  RPMI_MSG_IDN_SERVICE_ID_MASK) |
		 ((args->type << RPMI_MSG_IDN_TYPE_POS) &
		  RPMI_MSG_IDN_TYPE_MASK);

	/* Prepare the header to be written into the slot */
	header.token = cpu_to_le32((u32)xfer->seq);
	header.msgidn = cpu_to_le32(msgidn);
	header.datalen = cpu_to_le32(xfer->tx_len);

	/* Write header into the slot */
	dst = (char *)qctx->queue->buffer + (writeidx * slot_size);
	sbi_memcpy(dst, &header, sizeof(header));
	dst += sizeof(header);

	/* Write data into the slot */
	if (xfer->tx) {
		src = xfer->tx;
		for (i = 0; i < args->tx_endian_words; i++)
			((u32 *)dst)[i] = cpu_to_le32(((u32 *)src)[i]);
		dst += sizeof(u32) * args->tx_endian_words;
		src += sizeof(u32) * args->tx_endian_words;
		sbi_memcpy(dst, src,
			xfer->tx_len - (sizeof(u32) * args->tx_endian_words));
	}

	/* Update the write index */
	qctx->queue->writeidx = cpu_to_le32(writeidx + 1) % qctx->num_slots;
	smp_wmb();

	/* Ring the RPMI doorbell if present */
	if (mb_regs)
		writel(cpu_to_le32(1), &mb_regs->db_reg);

	return SBI_OK;
}

static int smq_rx(struct rpmi_shmem_mbox_controller *mctl,
		  u32 queue_id, u32 service_group_id, struct mbox_xfer *xfer)
{
	int ret, rxretry = 0;
	struct smq_queue_ctx *qctx;

	if (mctl->queue_count < queue_id ||
	    RPMI_MAILBOX_CHANNELS_MAX <= service_group_id) {
		sbi_printf("%s: invalid queue_id or service_group_id\n",
			   __func__);
		return SBI_EINVAL;
	}
	qctx = &mctl->queue_ctx_tbl[queue_id];

	/*
	 * Once the timeout happens and call this function is returned
	 * to the client then there is no way to deliver the response
	 * message after that if it comes later.
	 *
	 * REVISIT: In complete timeout duration how much duration
	 * it should wait(delay) before recv retry. udelay or mdelay
	 */
	do {
		spin_lock(&qctx->queue_lock);
		ret = __smq_rx(qctx, mctl->slot_size, service_group_id, xfer);
		spin_unlock(&qctx->queue_lock);
		if (!ret)
			return 0;

		sbi_timer_mdelay(1);
		rxretry += 1;
	} while (rxretry < xfer->rx_timeout);

	return SBI_ETIMEDOUT;
}

static int smq_tx(struct rpmi_shmem_mbox_controller *mctl,
		  u32 queue_id, u32 service_group_id, struct mbox_xfer *xfer)
{
	int ret, txretry = 0;
	struct smq_queue_ctx *qctx;

	if (mctl->queue_count < queue_id ||
	    RPMI_MAILBOX_CHANNELS_MAX <= service_group_id) {
		sbi_printf("%s: invalid queue_id or service_group_id\n",
			   __func__);
		return SBI_EINVAL;
	}
	qctx = &mctl->queue_ctx_tbl[queue_id];

	/*
	 * Ignoring the tx timeout since in RPMI has no mechanism
	 * with which other side can let know about the reception of
	 * message which marks as tx complete. For RPMI tx complete is
	 * marked as done when message in successfully copied in queue.
	 *
	 * REVISIT: In complete timeout duration how much duration
	 * it should wait(delay) before send retry. udelay or mdelay
	 */
	do {
		spin_lock(&qctx->queue_lock);
		ret = __smq_tx(qctx, mctl->mb_regs, mctl->slot_size,
				service_group_id, xfer);
		spin_unlock(&qctx->queue_lock);
		if (!ret)
			return 0;

		sbi_timer_mdelay(1);
		txretry += 1;
	} while (txretry < xfer->tx_timeout);

	return SBI_ETIMEDOUT;
}

static int smq_base_get_two_u32(struct rpmi_shmem_mbox_controller *mctl,
				u32 service_id, u32 *inarg, u32 *outvals)
{
	return rpmi_normal_request_with_status(
			mctl->base_chan, service_id,
			inarg, (inarg) ? 1 : 0, (inarg) ? 1 : 0,
			outvals, 2, 2);
}

/**************** Mailbox Controller Functions **************/

static int rpmi_shmem_mbox_xfer(struct mbox_chan *chan, struct mbox_xfer *xfer)
{
	int ret;
	u32 tx_qid = 0, rx_qid = 0;
	struct rpmi_shmem_mbox_controller *mctl =
			container_of(chan->mbox,
				     struct rpmi_shmem_mbox_controller,
				     controller);
	struct rpmi_message_args *args = xfer->args;
	bool do_tx = (args->flags & RPMI_MSG_FLAGS_NO_TX) ? false : true;
	bool do_rx = (args->flags & RPMI_MSG_FLAGS_NO_RX) ? false : true;

	if (!do_tx && !do_rx)
		return SBI_EINVAL;

	switch (args->type) {
	case RPMI_MSG_NORMAL_REQUEST:
		if (do_tx && do_rx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
			rx_qid = RPMI_QUEUE_IDX_P2A_ACK;
		} else if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
		} else if (do_rx) {
			rx_qid = RPMI_QUEUE_IDX_P2A_REQ;
		}
		break;
	case RPMI_MSG_POSTED_REQUEST:
		if (do_tx && do_rx)
			return SBI_EINVAL;
		if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
		} else {
			rx_qid = RPMI_QUEUE_IDX_P2A_REQ;
		}
		break;
	case RPMI_MSG_ACKNOWLDGEMENT:
		if (do_tx && do_rx)
			return SBI_EINVAL;
		if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_ACK;
		} else {
			rx_qid = RPMI_QUEUE_IDX_P2A_ACK;
		}
		break;
	default:
		return SBI_ENOTSUPP;
	}

	if (do_tx) {
		ret = smq_tx(mctl, tx_qid, chan - mctl->channels, xfer);
		if (ret)
			return ret;
	}

	if (do_rx) {
		ret = smq_rx(mctl, rx_qid, chan - mctl->channels, xfer);
		if (ret)
			return ret;
	}

	return 0;
}

static struct mbox_chan *rpmi_shmem_mbox_request_chan(
						struct mbox_controller *mbox,
						u32 *chan_args)
{
	int ret;
	u32 tval[2] = { 0 };
	struct rpmi_shmem_mbox_controller *mctl =
			container_of(mbox,
				     struct rpmi_shmem_mbox_controller,
				     controller);

	if (chan_args[0] >= RPMI_MAILBOX_CHANNELS_MAX)
		return NULL;

	/* Base serivce group is always present so probe other groups */
	if (chan_args[0] != RPMI_SRVGRP_BASE) {
		/* Probe service group */
		ret = smq_base_get_two_u32(mctl,
					   RPMI_BASE_SRV_PROBE_SERVICE_GROUP,
					   chan_args, tval);
		if (ret || !tval[1])
			return NULL;
	}

	return &mctl->channels[chan_args[0]];
}

static void *rpmi_shmem_mbox_free_chan(struct mbox_controller *mbox,
					struct mbox_chan *chan)
{
	/* Nothing to do here */
	return NULL;
}

extern struct fdt_mailbox fdt_mailbox_rpmi_shmem;

static int rpmi_shmem_transport_init(struct rpmi_shmem_mbox_controller *mctl,
				     void *fdt, int nodeoff)
{
	const char *name;
	int count, len, ret, qid;
	uint64_t reg_addr, reg_size;
	const fdt32_t *prop_slotsz;
	struct smq_queue_ctx *qctx;

	ret = fdt_node_check_compatible(fdt, nodeoff,
					"riscv,rpmi-shmem-mbox");
	if (ret)
		return ret;

	/* get queue slot size in bytes */
	prop_slotsz = fdt_getprop(fdt, nodeoff, "riscv,slot-size", &len);
	if (!prop_slotsz)
		return SBI_ENOENT;

	mctl->slot_size = fdt32_to_cpu(*prop_slotsz);
	if (mctl->slot_size < RPMI_MSG_SIZE_MIN) {
		sbi_printf("%s: slot_size < mimnum required message size\n",
			   __func__);
		mctl->slot_size = RPMI_MSG_SIZE_MIN;
	}

	/*
	 * queue names count is taken as the number of queues
	 * supported which make it mandatory to provide the
	 * name of the queue.
	 */
	count = fdt_stringlist_count(fdt, nodeoff, "reg-names");
	if (count < 0 ||
	    count > (RPMI_QUEUE_IDX_MAX_COUNT + RPMI_REG_IDX_MAX_COUNT))
		return SBI_EINVAL;

	mctl->queue_count = count - RPMI_REG_IDX_MAX_COUNT;

	/* parse all queues and populate queues context structure */
	for (qid = 0; qid < mctl->queue_count; qid++) {
		qctx = &mctl->queue_ctx_tbl[qid];

		/* get each queue share-memory base address and size*/
		ret = fdt_get_node_addr_size(fdt, nodeoff, qid,
					     &reg_addr, &reg_size);
		if (ret < 0 || !reg_addr || !reg_size)
			return SBI_ENOENT;

		qctx->queue = (void *)(unsigned long)reg_addr;

		/* calculate number of slots in each queue */
		qctx->num_slots =
			(reg_size - RPMI_QUEUE_HEADER_SIZE) / mctl->slot_size;

		/* get the queue name */
		name = fdt_stringlist_get(fdt, nodeoff, "reg-names",
					  qid, &len);
		if (!name || (name && len < 0))
			return len;

		sbi_memcpy(qctx->name, name, len);

		/* store the index as queue_id */
		qctx->queue_id = qid;

		SPIN_LOCK_INIT(qctx->queue_lock);
	}

	/* get the db-reg property name */
	name = fdt_stringlist_get(fdt, nodeoff, "reg-names", qid, &len);
	if (!name || (name && len < 0))
		return len;

	/* fetch doorbell register address*/
	ret = fdt_get_node_addr_size(fdt, nodeoff, qid, &reg_addr,
				       &reg_size);
	if (!ret && !(strncmp(name, "db-reg", strlen("db-reg"))))
		mctl->mb_regs = (void *)(unsigned long)reg_addr;

	return SBI_SUCCESS;
}

static int rpmi_shmem_mbox_init(void *fdt, int nodeoff, u32 phandle,
				const struct fdt_match *match)
{
	int ret = 0;
	u32 tval[2];
	struct rpmi_base_get_attributes_resp resp;
	struct rpmi_shmem_mbox_controller *mctl;

	mctl = sbi_zalloc(sizeof(*mctl));
	if (!mctl)
		return SBI_ENOMEM;

	/* Initialization transport from device tree */
	ret = rpmi_shmem_transport_init(mctl, fdt, nodeoff);
	if (ret)
		goto fail_free_controller;

	/* Register mailbox controller */
	mctl->controller.id = phandle;
	mctl->controller.max_xfer_len =
			mctl->slot_size - sizeof(struct rpmi_message_header);
	mctl->controller.driver = &fdt_mailbox_rpmi_shmem;
	mctl->controller.request_chan = rpmi_shmem_mbox_request_chan;
	mctl->controller.free_chan = rpmi_shmem_mbox_free_chan;
	mctl->controller.xfer = rpmi_shmem_mbox_xfer;
	ret = mbox_controller_add(&mctl->controller);
	if (ret)
		goto fail_free_controller;

	/* Request base service group channel */
	tval[0] = RPMI_SRVGRP_BASE;
	mctl->base_chan = mbox_controller_request_chan(&mctl->controller,
							tval);
	if (!mctl->base_chan) {
		ret = SBI_ENOENT;
		goto fail_remove_controller;
	}

	/* Get implementation id */
	ret = smq_base_get_two_u32(mctl,
				   RPMI_BASE_SRV_GET_IMPLEMENTATION_VERSION,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->impl_version = tval[1];

	/* Get implementation version */
	ret = smq_base_get_two_u32(mctl, RPMI_BASE_SRV_GET_IMPLEMENTATION_IDN,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->impl_id = tval[1];

	/* Get specification version */
	ret = smq_base_get_two_u32(mctl, RPMI_BASE_SRV_GET_SPEC_VERSION,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->spec_version = tval[1];

	/* Get optional features implementation flags */
	ret = rpmi_normal_request_with_status(
			mctl->base_chan, RPMI_BASE_SRV_GET_ATTRIBUTES,
			NULL, 0, 0,
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (ret)
		goto fail_free_chan;

	mctl->base_flags.f0_ev_notif_en =
			resp.f0 & RPMI_BASE_FLAGS_F0_EV_NOTIFY ? 1 : 0;
	mctl->base_flags.f0_msi_en =
			resp.f0 & RPMI_BASE_FLAGS_F0_MSI_EN ? 1 : 0;

	return 0;

fail_free_chan:
	mbox_controller_free_chan(mctl->base_chan);
fail_remove_controller:
	mbox_controller_remove(&mctl->controller);
fail_free_controller:
	sbi_free(mctl);
	return ret;
}

static const struct fdt_match rpmi_shmem_mbox_match[] = {
	{ .compatible = "riscv,rpmi-shmem-mbox" },
	{ },
};

struct fdt_mailbox fdt_mailbox_rpmi_shmem = {
	.match_table = rpmi_shmem_mbox_match,
	.init = rpmi_shmem_mbox_init,
	.xlate = fdt_mailbox_simple_xlate,
};
