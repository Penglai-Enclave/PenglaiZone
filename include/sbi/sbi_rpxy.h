/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __SBI_RPXY_H__
#define __SBI_RPXY_H__

#include <sbi/sbi_list.h>

struct sbi_scratch;

/** A RPMI proxy service accessible through SBI interface */
struct sbi_rpxy_service {
	u8 id;
	u32 min_tx_len;
	u32 max_tx_len;
	u32 min_rx_len;
	u32 max_rx_len;
};

/** A RPMI proxy service group accessible through SBI interface */
struct sbi_rpxy_service_group {
	/** List head to a set of service groups */
	struct sbi_dlist head;

	/** Details identifying this service group */
	u32 transport_id;
	u32 service_group_id;
	unsigned long max_message_data_len;

	/** Array of supported services */
	int num_services;
	struct sbi_rpxy_service *services;

	/**
	 * Send a normal/posted message for this service group
	 * NOTE: For posted message, ack_len == NULL
	 */
	int (*send_message)(struct sbi_rpxy_service_group *grp,
			    struct sbi_rpxy_service *srv,
			    void *tx, u32 tx_len,
			    void *rx, u32 rx_len,
			    unsigned long *ack_len);

	/** Get notification events for this service group */
	int (*get_notification_events)(struct sbi_rpxy_service_group *grp,
				       void *output_data,
				       u32 output_data_len,
				       unsigned long *events_len);
};

/** Check if some RPMI proxy service group is available */
bool sbi_rpxy_service_group_available(void);

/** Probe RPMI proxy service group */
int sbi_rpxy_probe(u32 transport_id, u32 service_group_id,
		   unsigned long *out_max_data_len);

/** Set RPMI proxy shared memory on the calling HART */
int sbi_rpxy_set_shmem(unsigned long shmem_size,
		       unsigned long shmem_phys_lo,
		       unsigned long shmem_phys_hi,
		       unsigned long flags);

/** Send a normal/posted RPMI proxy message */
int sbi_rpxy_send_message(u32 transport_id,
			  u32 service_group_id,
			  u8 service_id,
			  unsigned long message_data_len,
			  unsigned long *ack_data_len);

/** Get RPMI proxy notification events */
int sbi_rpxy_get_notification_events(u32 transport_id, u32 service_group_id,
				     unsigned long *events_len);

/** Register a RPMI proxy service group */
int sbi_rpxy_register_service_group(struct sbi_rpxy_service_group *grp);

/** Initialize RPMI proxy subsystem */
int sbi_rpxy_init(struct sbi_scratch *scratch);

#endif
