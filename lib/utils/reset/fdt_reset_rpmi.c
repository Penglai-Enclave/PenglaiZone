/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

/* struct rpmi_sysreset: RPMI System Reset Context */
struct rpmi_sysreset {
	int warm_reset_support;
	struct mbox_chan *chan;
};

static struct rpmi_sysreset sysreset_ctx;

static int rpmi_system_reset_type_check(u32 reset_type)
{
	int ret;
	struct rpmi_sysrst_get_reset_attributes_resp resp;

	ret = rpmi_normal_request_with_status(sysreset_ctx.chan,
		RPMI_SYSRST_SRV_GET_SYSTEM_RESET_ATTRIBUTES, &reset_type,
		rpmi_u32_count(reset_type), rpmi_u32_count(reset_type),
		&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (ret) {
		return 0;
	}

	return (resp.flags & RPMI_SYSRST_FLAGS_SUPPORTED_MASK) ? 1 : 0;
}

/**
 * rpmi_do_system_reset: Do system reset
 *
 * @reset_type: RPMI System Reset Type
 */
static void rpmi_do_system_reset(u32 reset_type)
{
	int ret;

	ret = rpmi_posted_request(sysreset_ctx.chan,
				  RPMI_SYSRST_SRV_SYSTEM_RESET,
				  &reset_type, rpmi_u32_count(reset_type),
				  rpmi_u32_count(reset_type));
	if (ret)
		sbi_printf("system reset failed [type: %d]: ret: %d\n",
			   reset_type, ret);
}

/**
 * rpmi_system_reset_check: Check the support for
 * various reset types
 *
 * @type: SBI System Reset Type
 * @reason: Reason for system reset
 */
static int rpmi_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		return 1;
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return sysreset_ctx.warm_reset_support;
	default:
		return 0;
	}
}

static void rpmi_system_reset(u32 type, u32 reason)
{
	u32 reset_type;

	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		reset_type = RPMI_SYSRST_SHUTDOWN;
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		reset_type = RPMI_SYSRST_COLD_RESET;
		break;
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		reset_type = RPMI_SYSRST_WARM_RESET;
		break;
	default:
		return;
	}

	rpmi_do_system_reset(reset_type);
}

static struct sbi_system_reset_device rpmi_reset_dev = {
	.name = "rpmi-system-reset",
	.system_reset_check = rpmi_system_reset_check,
	.system_reset = rpmi_system_reset,
};

static int rpmi_reset_init(void *fdt, int nodeoff,
			    const struct fdt_match *match)
{
	int ret;

	/* If channel already available then do nothing. */
	if (sysreset_ctx.chan)
		return 0;

	/*
	 * If channel request failed then other end does not support
	 * system reset group so do nothing.
	 */
	ret = fdt_mailbox_request_chan(fdt, nodeoff, 0, &sysreset_ctx.chan);
	if (ret)
		return ret;

	sysreset_ctx.warm_reset_support =
			rpmi_system_reset_type_check(RPMI_SYSRST_WARM_RESET);

	sbi_system_reset_add_device(&rpmi_reset_dev);

	return SBI_OK;
}

static const struct fdt_match rpmi_reset_match[] = {
	{ .compatible = "riscv,rpmi-system-reset" },
	{},
};

struct fdt_reset fdt_reset_rpmi = {
	.match_table = rpmi_reset_match,
	.init = rpmi_reset_init,
};
