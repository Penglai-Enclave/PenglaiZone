/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Subrahmanya Lingappa <slingappa@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_cppc.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/cppc/fdt_cppc.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

struct rpmi_cppc {
	struct mbox_chan *chan;
	bool fast_chan_supported;
	ulong fast_chan_addr;
	bool fast_chan_db_supported;
	enum rpmi_cppc_fast_channel_db_width fast_chan_db_width;
	ulong fast_chan_db_addr;
	u64 fast_chan_db_id;
};

static unsigned long rpmi_cppc_offset;

static struct rpmi_cppc *rpmi_cppc_get_pointer(u32 hartid)
{
	struct sbi_scratch *scratch;

	scratch = sbi_hartid_to_scratch(hartid);
	if (!scratch || !rpmi_cppc_offset)
		return NULL;

	return sbi_scratch_offset_ptr(scratch, rpmi_cppc_offset);
}

static int rpmi_cppc_read(unsigned long reg, u64 *val)
{
	int rc = SBI_SUCCESS;
	struct rpmi_cppc_read_reg_req req;
	struct rpmi_cppc_read_reg_resp resp;
	struct rpmi_cppc *cppc;

	req.hart_id = current_hartid();
	req.reg_id = reg;
	cppc = rpmi_cppc_get_pointer(req.hart_id);

	rc = rpmi_normal_request_with_status(
			cppc->chan, RPMI_CPPC_SRV_READ_REG,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (rc)
		return rc;

#if __riscv_xlen == 32
	*val = resp.data_lo;
#else
	*val = (u64)resp.data_hi << 32 | resp.data_lo;
#endif
	return rc;
}

static int rpmi_cppc_write(unsigned long reg, u64 val)
{
	int rc = SBI_SUCCESS;
	u32 hart_id = current_hartid();
	struct rpmi_cppc_write_reg_req req;
	struct rpmi_cppc_write_reg_resp resp;
	struct rpmi_cppc *cppc = rpmi_cppc_get_pointer(hart_id);

	if (reg != SBI_CPPC_DESIRED_PERF || !cppc->fast_chan_supported) {
		req.hart_id = hart_id;
		req.reg_id = reg;
		req.data_lo = val & 0xFFFFFFFF;
		req.data_hi = val >> 32;

		rc = rpmi_normal_request_with_status(
			cppc->chan, RPMI_CPPC_SRV_WRITE_REG,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	} else {
		/* use fast path writes */
#if __riscv_xlen != 32
		writeq(val, (void *)cppc->fast_chan_addr);
#else
		writel((u32)val, (void *)cppc->fast_chan_addr);
		writel((u32)(val >> 32), (void *)(cppc->fast_chan_addr + 4));
#endif
		if (cppc->fast_chan_db_supported) {
			switch (cppc->fast_chan_db_width) {
			case RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_8:
				writeb((u8)cppc->fast_chan_db_id,
				       (void *)cppc->fast_chan_db_addr);
				break;
			case RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_16:
				writew((u16)cppc->fast_chan_db_id,
				       (void *)cppc->fast_chan_db_addr);
				break;
			case RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_32:
				writel((u32)cppc->fast_chan_db_id,
				       (void *)cppc->fast_chan_db_addr);
				break;
			case RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_64:
#if __riscv_xlen != 32
				writeq(cppc->fast_chan_db_id,
				       (void *)cppc->fast_chan_db_addr);
#else
				writel((u32)cppc->fast_chan_db_id,
				       (void *)cppc->fast_chan_db_addr);
				writel((u32)(cppc->fast_chan_db_id >> 32),
				       (void *)(cppc->fast_chan_db_addr + 4));
#endif
				break;
			default:
				break;
			}
		}
	}

	return rc;
}

static int rpmi_cppc_probe(unsigned long reg)
{
	int rc;
	struct rpmi_cppc *cppc;
	struct rpmi_cppc_probe_resp resp;
	struct rpmi_cppc_probe_req req;

	req.hart_id = current_hartid();
	req.reg_id = reg;

	cppc = rpmi_cppc_get_pointer(req.hart_id);
	if (!cppc)
		return SBI_ENOSYS;

	rc = rpmi_normal_request_with_status(
			cppc->chan, RPMI_CPPC_SRV_PROBE_REG,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (rc)
		return rc;

	return resp.reg_len;
}

static struct sbi_cppc_device sbi_rpmi_cppc = {
	.name		= "rpmi-cppc",
	.cppc_read	= rpmi_cppc_read,
	.cppc_write	= rpmi_cppc_write,
	.cppc_probe	= rpmi_cppc_probe,
};

static int rpmi_cppc_update_hart_scratch(struct mbox_chan *chan)
{
	int rc, i;
	struct rpmi_cppc_hart_list_req req;
	struct rpmi_cppc_hart_list_resp resp;
	struct rpmi_cppc_get_fast_channel_addr_req freq;
	struct rpmi_cppc_get_fast_channel_addr_resp fresp;
	struct rpmi_cppc *cppc;

	req.start_index = 0;
	do {
		rc = rpmi_normal_request_with_status(
			chan, RPMI_CPPC_SRV_GET_HART_LIST,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
		if (rc)
			return rc;

		for (i = 0; i < resp.returned; i++) {
			cppc = rpmi_cppc_get_pointer(resp.hartid[i]);
			if (!cppc)
				return SBI_ENOSYS;
			cppc->chan = chan;

			freq.hart_id = resp.hartid[i];
			rc = rpmi_normal_request_with_status(
				chan, RPMI_CPPC_SRV_GET_FAST_CHANNEL_ADDR,
				&freq, rpmi_u32_count(freq), rpmi_u32_count(freq),
				&fresp, rpmi_u32_count(fresp), rpmi_u32_count(fresp));
			if (rc)
				continue;

			cppc->fast_chan_supported = true;
#if __riscv_xlen == 32
			cppc->fast_chan_addr = fresp.addr_lo;
#else
			cppc->fast_chan_addr = (ulong)fresp.addr_hi << 32 |
						fresp.addr_lo;
#endif
			cppc->fast_chan_db_supported = fresp.flags &
				RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_SUPPORTED;
			cppc->fast_chan_db_width = (fresp.flags &
				RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_WIDTH_MASK) >>
				RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_WIDTH_POS;
#if __riscv_xlen == 32
			cppc->fast_chan_db_addr = fresp.db_addr_lo;
#else
			cppc->fast_chan_db_addr = (ulong)fresp.db_addr_hi << 32 |
						fresp.db_addr_lo;
#endif
			cppc->fast_chan_db_id = (u64)fresp.db_id_hi << 32 |
						fresp.db_id_lo;
		}

		req.start_index = i;
	} while (resp.remaining);

	return 0;
}

static int rpmi_cppc_cold_init(void *fdt, int nodeoff,
			       const struct fdt_match *match)
{
	int rc;
	struct mbox_chan *chan;

	if (!rpmi_cppc_offset) {
		rpmi_cppc_offset =
			sbi_scratch_alloc_type_offset(struct rpmi_cppc);
		if (!rpmi_cppc_offset)
			return SBI_ENOMEM;
	}

	/*
	 * If channel request failed then other end does not support
	 * CPPC service group so do nothing.
	 */
	rc = fdt_mailbox_request_chan(fdt, nodeoff, 0, &chan);
	if (rc)
		return 0;

	/* Update per-HART scratch space */
	rc = rpmi_cppc_update_hart_scratch(chan);
	if (rc)
		return rc;

	sbi_cppc_set_device(&sbi_rpmi_cppc);

	return 0;
}

static const struct fdt_match rpmi_cppc_match[] = {
	{ .compatible = "riscv,rpmi-cppc" },
	{},
};

struct fdt_cppc fdt_cppc_rpmi = {
	.match_table = rpmi_cppc_match,
	.cold_init = rpmi_cppc_cold_init,
};
