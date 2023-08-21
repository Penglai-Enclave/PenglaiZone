/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_rpxy.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/rpxy/fdt_rpxy.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>

struct rpxy_mbox_data {
	u32 service_group_id;
	int num_services;
	struct sbi_rpxy_service *services;
};

struct rpxy_mbox {
	struct sbi_rpxy_service_group group;
	struct mbox_chan *chan;
};

static int rpxy_mbox_send_message(struct sbi_rpxy_service_group *grp,
				  struct sbi_rpxy_service *srv,
				  void *tx, u32 tx_len,
				  void *rx, u32 rx_len,
				  unsigned long *ack_len)
{
	int ret;
	struct mbox_xfer xfer;
	struct rpmi_message_args args = { 0 };
	struct rpxy_mbox *rmb = container_of(grp, struct rpxy_mbox, group);

	if (ack_len) {
		args.type = RPMI_MSG_NORMAL_REQUEST;
		args.flags = (rx) ? 0 : RPMI_MSG_FLAGS_NO_RX;
		args.service_id = srv->id;
		mbox_xfer_init_txrx(&xfer, &args,
				    tx, tx_len, RPMI_DEF_TX_TIMEOUT,
				    rx, rx_len, RPMI_DEF_RX_TIMEOUT);
	} else {
		args.type = RPMI_MSG_POSTED_REQUEST;
		args.flags = RPMI_MSG_FLAGS_NO_RX;
		args.service_id = srv->id;
		mbox_xfer_init_tx(&xfer, &args,
				  tx, tx_len, RPMI_DEF_TX_TIMEOUT);
	}

	ret = mbox_chan_xfer(rmb->chan, &xfer);
	if (ret)
		return (ret == SBI_ETIMEDOUT) ? SBI_ETIMEDOUT : SBI_EFAIL;
	if (ack_len)
		*ack_len = args.rx_data_len;

	return 0;
}

static int rpxy_mbox_init(void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	int rc;
	struct rpxy_mbox *rmb;
	struct mbox_chan *chan;
	const struct rpxy_mbox_data *data = match->data;

	/* Allocate context for RPXY mbox client */
	rmb = sbi_zalloc(sizeof(*rmb));
	if (!rmb)
		return SBI_ENOMEM;

	/*
	 * If channel request failed then other end does not support
	 * service group so do nothing.
	 */
	rc = fdt_mailbox_request_chan(fdt, nodeoff, 0, &chan);
	if (rc) {
		sbi_free(rmb);
		return 0;
	}

	/* Match channel service group id */
	if (data->service_group_id != chan->chan_args[0]) {
		mbox_controller_free_chan(chan);
		sbi_free(rmb);
		return SBI_EINVAL;
	}

	/* Setup RPXY mbox client */
	rmb->group.transport_id = chan->mbox->id;
	rmb->group.service_group_id = data->service_group_id;
	rmb->group.max_message_data_len = chan->mbox->max_xfer_len;
	rmb->group.num_services = data->num_services;
	rmb->group.services = data->services;
	rmb->group.send_message = rpxy_mbox_send_message;
	rmb->chan = chan;

	/* Register RPXY service group */
	rc = sbi_rpxy_register_service_group(&rmb->group);
	if (rc) {
		mbox_controller_free_chan(chan);
		sbi_free(rmb);
		return rc;
	}

	return 0;
}

static struct sbi_rpxy_service clock_services[] = {
{
	.id = RPMI_CLOCK_SRV_GET_NUM_CLOCKS,
	.min_tx_len = 0,
	.max_tx_len = 0,
	.min_rx_len = sizeof(struct rpmi_clock_get_num_clocks_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_num_clocks_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_ATTRIBUTES,
	.min_tx_len = sizeof(struct rpmi_clock_get_attributes_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_attributes_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_attributes_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_attributes_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_SUPPORTED_RATES,
	.min_tx_len = sizeof(struct rpmi_clock_get_supported_rates_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_supported_rates_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_supported_rates_resp),
	.max_rx_len = -1U,
},
{
	.id = RPMI_CLOCK_SRV_SET_CONFIG,
	.min_tx_len = sizeof(struct rpmi_clock_set_config_req),
	.max_tx_len = sizeof(struct rpmi_clock_set_config_req),
	.min_rx_len = sizeof(struct rpmi_clock_set_config_resp),
	.max_rx_len = sizeof(struct rpmi_clock_set_config_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_CONFIG,
	.min_tx_len = sizeof(struct rpmi_clock_get_config_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_config_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_config_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_config_resp),
},
{
	.id = RPMI_CLOCK_SRV_SET_RATE,
	.min_tx_len = sizeof(struct rpmi_clock_set_rate_req),
	.max_tx_len = sizeof(struct rpmi_clock_set_rate_req),
	.min_rx_len = sizeof(struct rpmi_clock_set_rate_resp),
	.max_rx_len = sizeof(struct rpmi_clock_set_rate_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_RATE,
	.min_tx_len = sizeof(struct rpmi_clock_get_rate_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_rate_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_rate_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_rate_resp),
},
};

static struct rpxy_mbox_data clock_data = {
	.service_group_id = RPMI_SRVGRP_CLOCK,
	.num_services = array_size(clock_services),
	.services = clock_services,
};

static const struct fdt_match rpxy_mbox_match[] = {
	{ .compatible = "riscv,rpmi-clock", .data = &clock_data },
	{},
};

struct fdt_rpxy fdt_rpxy_mbox = {
	.match_table = rpxy_mbox_match,
	.init = rpxy_mbox_init,
};
