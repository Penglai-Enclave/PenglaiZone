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
#include <sbi_utils/hsm/fdt_hsm.h>

/* List of FDT HSM drivers generated at compile time */
extern struct fdt_hsm *fdt_hsm_drivers[];
extern unsigned long fdt_hsm_drivers_size;

static struct fdt_hsm *current_driver = NULL;

int fdt_hsm_fixup(void *fdt)
{
	if (current_driver && current_driver->fdt_fixup)
		return current_driver->fdt_fixup(fdt);
	return 0;
}

void fdt_hsm_exit(void)
{
	if (current_driver && current_driver->exit)
		current_driver->exit();
}

static int fdt_hsm_warm_init(void)
{
	if (current_driver && current_driver->warm_init)
		return current_driver->warm_init();
	return 0;
}

static int fdt_hsm_cold_init(void)
{
	int pos, noff, rc;
	struct fdt_hsm *drv;
	const struct fdt_match *match;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_hsm_drivers_size; pos++) {
		drv = fdt_hsm_drivers[pos];

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
			 * We can have multiple HSM devices on multi-die or
			 * multi-socket systems so we cannot break here.
			 */
		}
	}

	/*
	 * On some single-hart system there is no need for HSM,
	 * so we cannot return a failure here
	 */
	return 0;
}

int fdt_hsm_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = fdt_hsm_cold_init();
		if (rc)
			return rc;
	}

	return fdt_hsm_warm_init();
}
