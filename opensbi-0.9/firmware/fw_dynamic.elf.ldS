/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	#include "fw_base.ldS"

#ifdef FW_PAYLOADMMSTUB_OFFSET
	. = FW_TEXT_START + FW_PAYLOADMMSTUB_OFFSET;

	.payloadmmstub :
	{
		PROVIDE(_payloadmmstub_start = .);
		*(.payloadmmstub)
		. = ALIGN(8);
		PROVIDE(_payloadmmstub_end = .);
        }
#endif

#ifdef FW_PAYLOADMM_OFFSET
	. = FW_TEXT_START + FW_PAYLOADMM_OFFSET;

	.payloadmm :
	{
		PROVIDE(_payloadmm_start = .);
		*(.payloadmm)
		. = ALIGN(8);
		PROVIDE(_payloadmm_end = .);
        }
#endif
	PROVIDE(_fw_reloc_end = .);
}
