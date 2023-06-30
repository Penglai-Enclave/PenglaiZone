/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/suspend/fdt_suspend.h>

/* List of FDT suspend drivers generated at compile time */
extern struct fdt_suspend *fdt_suspend_drivers[];
extern unsigned long fdt_suspend_drivers_size;

int fdt_suspend_driver_init(void *fdt, struct fdt_suspend *drv)
{
	int noff, rc = SBI_ENODEV;
	const struct fdt_match *match;

	noff = fdt_find_match(fdt, -1, drv->match_table, &match);
	if (noff < 0)
		return SBI_ENODEV;

	if (drv->init) {
		rc = drv->init(fdt, noff, match);
		if (rc && rc != SBI_ENODEV) {
			sbi_printf("%s: %s init failed, %d\n",
				   __func__, match->compatible, rc);
		}
	}

	return rc;
}

void fdt_suspend_init(void)
{
	int pos;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_suspend_drivers_size; pos++)
		fdt_suspend_driver_init(fdt, fdt_suspend_drivers[pos]);
}
