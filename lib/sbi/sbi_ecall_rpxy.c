/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_rpxy.h>

static int sbi_ecall_rpxy_handler(unsigned long extid, unsigned long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_RPXY_PROBE:
		ret = sbi_rpxy_probe(regs->a0, regs->a1, out_val);
		break;
	case SBI_EXT_RPXY_SET_SHMEM:
		ret = sbi_rpxy_set_shmem(regs->a0,
					 regs->a1, regs->a2, regs->a3);
		break;
	case SBI_EXT_RPXY_SEND_NORMAL_MESSAGE:
		ret = sbi_rpxy_send_message(regs->a0, regs->a1, regs->a2,
					    regs->a3, out_val);
		break;
	case SBI_EXT_RPXY_SEND_POSTED_MESSAGE:
		ret = sbi_rpxy_send_message(regs->a0, regs->a1, regs->a2,
					    regs->a3, NULL);
		break;
	case SBI_EXT_RPXY_GET_NOTIFICATION_EVENTS:
		ret = sbi_rpxy_get_notification_events(regs->a0, regs->a1,
							out_val);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_rpxy;

static int sbi_ecall_rpxy_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_rpxy);
}

struct sbi_ecall_extension ecall_rpxy = {
	.extid_start		= SBI_EXT_RPXY,
	.extid_end		= SBI_EXT_RPXY,
	.register_extensions	= sbi_ecall_rpxy_register_extensions,
	.handle			= sbi_ecall_rpxy_handler,
};
