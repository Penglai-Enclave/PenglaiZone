/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_rpxy.h>
#include <sbi/sbi_scratch.h>

struct rpxy_state {
	unsigned long shmem_size;
	unsigned long shmem_addr;
};

/** Offset of pointer to RPXY state in scratch space */
static unsigned long rpxy_state_offset;

/** List of RPMI proxy service groups */
static SBI_LIST_HEAD(rpxy_group_list);

static struct sbi_rpxy_service *rpxy_find_service(
					struct sbi_rpxy_service_group *grp,
					u8 service_id)
{
	int i;

	for (i = 0; i < grp->num_services; i++)
		if (grp->services[i].id == service_id)
			return &grp->services[i];

	return NULL;
}

static struct sbi_rpxy_service_group *rpxy_find_group(u32 transport_id,
						      u32 service_group_id)
{
	struct sbi_rpxy_service_group *grp;

	sbi_list_for_each_entry(grp, &rpxy_group_list, head)
		if (grp->transport_id == transport_id &&
		    grp->service_group_id == service_group_id)
		    return grp;

	return NULL;
}

bool sbi_rpxy_service_group_available(void)
{
	return sbi_list_empty(&rpxy_group_list) ? false : true;
}

int sbi_rpxy_probe(u32 transport_id, u32 service_group_id,
		   unsigned long *out_max_data_len)
{
	int rc = SBI_ENOTSUPP;
	struct sbi_rpxy_service_group *grp;

	*out_max_data_len = 0;
	grp = rpxy_find_group(transport_id, service_group_id);
	if (grp) {
		*out_max_data_len = grp->max_message_data_len;
		rc = 0;
	}

	return rc;
}

int sbi_rpxy_set_shmem(unsigned long shmem_size,
		       unsigned long shmem_phys_lo,
		       unsigned long shmem_phys_hi,
		       unsigned long flags)
{
	struct rpxy_state *rs =
		sbi_scratch_thishart_offset_ptr(rpxy_state_offset);

	if (shmem_phys_lo == -1UL && shmem_phys_hi == -1UL) {
		rs->shmem_size = 0;
		rs->shmem_addr = 0;
		return 0;
	}

	if (flags || !shmem_size ||
	    (shmem_size & ~PAGE_MASK) ||
	    (shmem_phys_lo & ~PAGE_MASK))
		return SBI_EINVAL;
	if (shmem_phys_hi ||
	    !sbi_domain_check_addr_range(sbi_domain_thishart_ptr(),
					 shmem_phys_lo, shmem_size, PRV_S,
					 SBI_DOMAIN_READ|SBI_DOMAIN_WRITE))
		return SBI_EINVALID_ADDR;

	rs->shmem_size = shmem_size;
	rs->shmem_addr = shmem_phys_lo;
	return 0;
}

int sbi_rpxy_send_message(u32 transport_id,
			  u32 service_group_id,
			  u8 service_id,
			  unsigned long message_data_len,
			  unsigned long *ack_data_len)
{
	int rc;
	u32 tx_len = 0, rx_len = 0;
	void *tx = NULL, *rx = NULL;
	struct sbi_rpxy_service *srv = NULL;
	struct sbi_rpxy_service_group *grp;
	struct rpxy_state *rs =
		sbi_scratch_thishart_offset_ptr(rpxy_state_offset);

	if (!rs->shmem_size)
		return SBI_ENOSHMEM;

	grp = rpxy_find_group(transport_id, service_group_id);
	if (grp)
		srv = rpxy_find_service(grp, service_id);
	if (!srv)
		return SBI_ENOTSUPP;

	tx_len = message_data_len;
	if (tx_len > rs->shmem_size || tx_len > grp->max_message_data_len)
		return SBI_EINVAL;
	if (tx_len < srv->min_tx_len || srv->max_tx_len < tx_len)
		return SBI_EFAIL;

	sbi_hart_map_saddr(rs->shmem_addr, rs->shmem_size);

	tx = (void *)rs->shmem_addr;
	if (ack_data_len) {
		rx = (void *)rs->shmem_addr;
		if (srv->min_rx_len == srv->max_rx_len)
			rx_len = srv->min_rx_len;
		else if (srv->max_rx_len < grp->max_message_data_len)
			rx_len = srv->max_rx_len;
		else
			rx_len = grp->max_message_data_len;
	}

	rc = grp->send_message(grp, srv, tx, tx_len, rx, rx_len, ack_data_len);
	sbi_hart_unmap_saddr();
	if (rc)
		return rc;

	if (ack_data_len &&
	    (*ack_data_len > rs->shmem_size ||
	     *ack_data_len > grp->max_message_data_len))
		return SBI_EFAIL;

	return 0;
}

int sbi_rpxy_get_notification_events(u32 transport_id, u32 service_group_id,
				     unsigned long *events_len)
{
	int rc;
	struct sbi_rpxy_service_group *grp;
	struct rpxy_state *rs =
		sbi_scratch_thishart_offset_ptr(rpxy_state_offset);

	if (!rs->shmem_size)
		return SBI_ENOSHMEM;

	grp = rpxy_find_group(transport_id, service_group_id);
	if (!grp)
		return SBI_ENOTSUPP;

	if (!grp->get_notification_events || !events_len)
		return SBI_EFAIL;

	sbi_hart_map_saddr(rs->shmem_addr, rs->shmem_size);
	rc = grp->get_notification_events(grp, (void *)rs->shmem_addr,
					  rs->shmem_size,
					  events_len);
	sbi_hart_unmap_saddr();
	if (rc)
		return rc;

	if (*events_len > rs->shmem_size)
		return SBI_EFAIL;

	return 0;
}

int sbi_rpxy_register_service_group(struct sbi_rpxy_service_group *grp)
{
	int i;
	struct sbi_rpxy_service *srv;

	if (!grp ||
	    !grp->max_message_data_len ||
	    !grp->num_services || !grp->services ||
	    !grp->send_message)
		return SBI_EINVAL;
	for (i = 0; i < grp->num_services; i++) {
		srv = &grp->services[i];
		if (!srv->id ||
		    (srv->min_tx_len > srv->max_tx_len) ||
		    (srv->min_tx_len > grp->max_message_data_len) ||
		    (srv->min_rx_len > srv->max_rx_len) ||
		    (srv->min_rx_len > grp->max_message_data_len))
			return SBI_EINVAL;
	}

	if (rpxy_find_group(grp->transport_id, grp->service_group_id))
		return SBI_EALREADY;

	SBI_INIT_LIST_HEAD(&grp->head);
	sbi_list_add_tail(&grp->head, &rpxy_group_list);

	return 0;
}

int sbi_rpxy_init(struct sbi_scratch *scratch)
{
	rpxy_state_offset = sbi_scratch_alloc_type_offset(struct rpxy_state);
	if (!rpxy_state_offset)
		return SBI_ENOMEM;

	return sbi_platform_rpxy_init(sbi_platform_ptr(scratch));
}
