/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_RPXY_H__
#define __FDT_RPXY_H__

#include <sbi/sbi_types.h>

#ifdef CONFIG_FDT_RPXY

struct fdt_rpxy {
	const struct fdt_match *match_table;
	int (*init)(void *fdt, int nodeoff, const struct fdt_match *match);
	void (*exit)(void);
};

int fdt_rpxy_init(void);

#else

static inline int fdt_rpxy_init(void) { return 0; }

#endif

#endif
