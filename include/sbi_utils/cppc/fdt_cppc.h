/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_CPPC_H__
#define __FDT_CPPC_H__

#include <sbi/sbi_types.h>

#ifdef CONFIG_FDT_CPPC

struct fdt_cppc {
	const struct fdt_match *match_table;
	int (*cold_init)(void *fdt, int nodeoff, const struct fdt_match *match);
	int (*warm_init)(void);
	void (*exit)(void);
};

void fdt_cppc_exit(void);

int fdt_cppc_init(bool cold_boot);

#else

static inline void fdt_cppc_exit(void) { }
static inline int fdt_cppc_init(bool cold_boot) { return 0; }

#endif

#endif
