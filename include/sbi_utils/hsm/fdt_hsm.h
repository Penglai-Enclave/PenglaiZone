/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_HSM_H__
#define __FDT_HSM_H__

#include <sbi/sbi_types.h>

#ifdef CONFIG_FDT_HSM

struct fdt_hsm {
	const struct fdt_match *match_table;
	int (*fdt_fixup)(void *fdt);
	int (*cold_init)(void *fdt, int nodeoff, const struct fdt_match *match);
	int (*warm_init)(void);
	void (*exit)(void);
};

int fdt_hsm_fixup(void *fdt);

void fdt_hsm_exit(void);

int fdt_hsm_init(bool cold_boot);

#else

static inline int fdt_hsm_fixup(void *fdt) { return 0; }
static inline void fdt_hsm_exit(void) { }
static inline int fdt_hsm_init(bool cold_boot) { return 0; }

#endif

#endif
