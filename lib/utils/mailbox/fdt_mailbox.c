/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>

/* List of FDT mailbox drivers generated at compile time */
extern struct fdt_mailbox *fdt_mailbox_drivers[];
extern unsigned long fdt_mailbox_drivers_size;

static struct fdt_mailbox *fdt_mailbox_driver(struct mbox_controller *mbox)
{
	int pos;

	if (!mbox)
		return NULL;

	for (pos = 0; pos < fdt_mailbox_drivers_size; pos++) {
		if (mbox->driver == fdt_mailbox_drivers[pos])
			return fdt_mailbox_drivers[pos];
	}

	return NULL;
}

static int fdt_mailbox_init(void *fdt, u32 phandle)
{
	int pos, nodeoff, rc;
	struct fdt_mailbox *drv;
	const struct fdt_match *match;

	/* Find node offset */
	nodeoff = fdt_node_offset_by_phandle(fdt, phandle);
	if (nodeoff < 0)
		return nodeoff;

	/* Try all mailbox drivers one-by-one */
	for (pos = 0; pos < fdt_mailbox_drivers_size; pos++) {
		drv = fdt_mailbox_drivers[pos];

		match = fdt_match_node(fdt, nodeoff, drv->match_table);
		if (match && drv->init) {
			rc = drv->init(fdt, nodeoff, phandle, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
			return 0;
		}
	}

	return SBI_ENOSYS;
}

static int fdt_mbox_controller_find(void *fdt, u32 phandle,
				    struct mbox_controller **out_mbox)
{
	int rc;
	struct mbox_controller *mbox = mbox_controller_find(phandle);

	if (!mbox) {
		/* mailbox not found so initialize matching driver */
		rc = fdt_mailbox_init(fdt, phandle);
		if (rc)
			return rc;

		/* Try to find mailbox controller again */
		mbox = mbox_controller_find(phandle);
		if (!mbox)
			return SBI_ENOSYS;
	}

	if (out_mbox)
		*out_mbox = mbox;

	return 0;
}

int fdt_mailbox_request_chan(void *fdt, int nodeoff, int index,
			     struct mbox_chan **out_chan)
{
	int rc;
	struct mbox_chan *chan;
	struct fdt_mailbox *drv;
	struct fdt_phandle_args pargs;
	struct mbox_controller *mbox = NULL;
	u32 phandle, chan_args[MBOX_CHAN_MAX_ARGS];

	if (!fdt || (nodeoff < 0) || (index < 0) || !out_chan)
		return SBI_EINVAL;

	pargs.node_offset = pargs.args_count = 0;
	rc = fdt_parse_phandle_with_args(fdt, nodeoff,
					 "mboxes", "#mbox-cells",
					 index, &pargs);
	if (rc)
		return rc;

	phandle = fdt_get_phandle(fdt, pargs.node_offset);
	rc = fdt_mbox_controller_find(fdt, phandle, &mbox);
	if (rc)
		return rc;

	drv = fdt_mailbox_driver(mbox);
	if (!drv || !drv->xlate)
		return SBI_ENOSYS;

	rc = drv->xlate(mbox, &pargs, chan_args);
	if (rc)
		return rc;

	chan = mbox_controller_request_chan(mbox, chan_args);
	if (!chan)
		return SBI_ENOENT;

	*out_chan = chan;
	return 0;
}

int fdt_mailbox_simple_xlate(struct mbox_controller *mbox,
			     const struct fdt_phandle_args *pargs,
			     u32 *out_chan_args)
{
	int i;

	if (pargs->args_count < 1)
		return SBI_EINVAL;

	out_chan_args[0] = pargs->args[0];
	for (i = 1; i < MBOX_CHAN_MAX_ARGS; i++)
		out_chan_args[i] = 0;

	return 0;
}
