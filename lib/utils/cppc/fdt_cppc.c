/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/cppc/fdt_cppc.h>

/* List of FDT CPPC drivers generated at compile time */
extern struct fdt_cppc *fdt_cppc_drivers[];
extern unsigned long fdt_cppc_drivers_size;

static struct fdt_cppc *current_driver = NULL;

void fdt_cppc_exit(void)
{
	if (current_driver && current_driver->exit)
		current_driver->exit();
}

static int fdt_cppc_warm_init(void)
{
	if (current_driver && current_driver->warm_init)
		return current_driver->warm_init();
	return 0;
}

static int fdt_cppc_cold_init(void)
{
	int pos, noff, rc;
	struct fdt_cppc *drv;
	const struct fdt_match *match;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_cppc_drivers_size; pos++) {
		drv = fdt_cppc_drivers[pos];

		noff = -1;
		while ((noff = fdt_find_match(fdt, noff,
					drv->match_table, &match)) >= 0) {
			/* drv->cold_init must not be NULL */
			if (drv->cold_init == NULL)
				return SBI_EFAIL;

			rc = drv->cold_init(fdt, noff, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
			current_driver = drv;

			/*
			 * We can have multiple CPPC devices on multi-die or
			 * multi-socket systems so we cannot break here.
			 */
		}
	}

	/*
	 * On some single-hart system there is no need for CPPC,
	 * so we cannot return a failure here
	 */
	return 0;
}

int fdt_cppc_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = fdt_cppc_cold_init();
		if (rc)
			return rc;
	}

	return fdt_cppc_warm_init();
}
