/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_SUSPEND_H__
#define __FDT_SUSPEND_H__

#include <sbi/sbi_types.h>

struct fdt_suspend {
	const struct fdt_match *match_table;
	int (*init)(void *fdt, int nodeoff, const struct fdt_match *match);
};

#ifdef CONFIG_FDT_SUSPEND

/**
 * fdt_suspend_driver_init() - initialize suspend driver based on the device-tree
 */
int fdt_suspend_driver_init(void *fdt, struct fdt_suspend *drv);

/**
 * fdt_suspend_init() - initialize reset drivers based on the device-tree
 *
 * This function shall be invoked in final init.
 */
void fdt_suspend_init(void);

#else

static inline int fdt_suspend_driver_init(void *fdt, struct fdt_suspend *drv)
{
	return 0;
}

static inline void fdt_suspend_init(void) { }

#endif

#endif
