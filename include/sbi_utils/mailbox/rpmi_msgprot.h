/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 *   Subrahmanya Lingappa <slingappa@ventanamicro.com>
 */

#ifndef __RPMI_MSGPROT_H__
#define __RPMI_MSGPROT_H__

#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_error.h>

/*
 * 31                                            0
 * +---------------------------------------------+
 * |                    TOKEN                    |
 * +---------+------------------+----------------+
 * |   FLAGS |  SERVICEGROUP_ID |    SERVICE_ID  |
 * +---------+------------------+----------------+
 * |                 DATA LENGTH                 |
 * +---------------------------------------------+
 * |                 DATA/PAYLOAD                |
 * +---------------------------------------------+
 */

/** Message Header Offset */
#define RPMI_MSG_HDR_OFFSET			(0x0)
#define RPMI_MSG_HDR_SIZE			(12)	/* bytes */

/** Token is unique message identifier in the system */
#define RPMI_MSG_TOKEN_OFFSET			(0x0)
#define RPMI_MSG_TOKEN_SIZE			(4)	/* bytes */

/** Message Identity = Flags + Service Group ID + Service ID */
#define RPMI_MSG_IDN_OFFSET			(0x4)
#define RPMI_MSG_IDN_SIZE			(4)	/* bytes */

#define RPMI_MSG_IDN_SERVICE_ID_POS		(0U)
#define RPMI_MSG_IDN_SERVICE_ID_MASK		\
	((0xFF) << RPMI_MSG_IDN_SERVICE_ID_POS)

#define RPMI_MSG_IDN_SERVICEGROUP_ID_POS	(8U)
#define RPMI_MSG_IDN_SERVICEGROUP_ID_MASK	\
	((0xFFFFF) << RPMI_MSG_IDN_SERVICEGROUP_ID_POS)

#define RPMI_MSG_IDN_TYPE_POS			(28U)
#define RPMI_MSG_IDN_TYPE_MASK 			\
	((0x3) << RPMI_MSG_IDN_TYPE_POS)

#define RPMI_MSG_IDN_DOORBELL_POS		(30U)
#define RPMI_MSG_IDN_DOORBELL_MASK		\
	((0x1) << RPMI_MSG_IDN_DOORBELL_POS)

/** Data length field */
#define RPMI_MSG_DATALEN_OFFSET			(0x8)
#define RPMI_MSG_DATALEN_SIZE			(4)	/* bytes */

/** Data field */
#define RPMI_MSG_DATA_OFFSET			(0xc)
#define RPMI_MSG_DATA_SIZE			(52)	/* bytes */

/** Minimum message size Header + Data */
#define RPMI_MSG_SIZE_MIN			(RPMI_MSG_HDR_SIZE + \
						 RPMI_MSG_DATA_SIZE)

/** Name length of 16 characters */
#define RPMI_NAME_CHARS_MAX			(16)

/** Default timeout values */
#define RPMI_DEF_TX_TIMEOUT			20
#define RPMI_DEF_RX_TIMEOUT			20

/** RPMI Message Header */
struct rpmi_message_header {
	le32_t token;
	le32_t msgidn;
	le32_t datalen;
} __packed;

/** RPMI Message */
struct rpmi_message {
	struct rpmi_message_header header;
	u8 data[0];
} __packed;

/** RPMI Messages Types */
enum rpmi_message_type {
	/* Normal request backed with ack */
	RPMI_MSG_NORMAL_REQUEST = 0x0,
	/* Request without any ack */
	RPMI_MSG_POSTED_REQUEST = 0x1,
	/* Acknowledgment for normal request message */
	RPMI_MSG_ACKNOWLDGEMENT = 0x2,
	/* Notification message */
	RPMI_MSG_NOTIFICATION = 0x3,
};

/** RPMI Error Types */
enum rpmi_error {
	RPMI_SUCCESS = 0,
	RPMI_ERR_FAILED = -1,
	RPMI_ERR_NOTSUPP = -2,
	RPMI_ERR_INVAL = -3,
	RPMI_ERR_DENIED = -4,
	RPMI_ERR_NOTFOUND = -5,
	RPMI_ERR_OUTOFRANGE = -6,
	RPMI_ERR_OUTOFRES = -7,
	RPMI_ERR_HWFAULT = -8,
};

/** RPMI Message Arguments */
struct rpmi_message_args {
	u32 flags;
#define RPMI_MSG_FLAGS_NO_TX		(1U << 0)
#define RPMI_MSG_FLAGS_NO_RX		(1U << 1)
#define RPMI_MSG_FLAGS_NO_RX_TOKEN	(1U << 2)
	enum rpmi_message_type type;
	u32 service_id;
	u32 tx_endian_words;
	u32 rx_endian_words;
	u32 rx_token;
	u32 rx_data_len;
};

/*
 * RPMI SERVICEGROUPS AND SERVICES
 */

/** RPMI ServiceGroups IDs */
enum rpmi_servicegroup_id {
	RPMI_SRVGRP_ID_MIN = 0,
	RPMI_SRVGRP_BASE = 0x00001,
	RPMI_SRVGRP_SYSTEM_RESET = 0x00002,
	RPMI_SRVGRP_SYSTEM_SUSPEND = 0x00003,
	RPMI_SRVGRP_HSM = 0x00004,
	RPMI_SRVGRP_ID_MAX_COUNT,
};

/** RPMI enable notification request */
struct rpmi_enable_notification_req {
	u32 eventid;
};

/** RPMI enable notification response */
struct rpmi_enable_notification_resp {
	s32 status;
};

/** RPMI Base ServiceGroup Service IDs */
enum rpmi_base_service_id {
	RPMI_BASE_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_BASE_SRV_GET_IMPLEMENTATION_VERSION = 0x02,
	RPMI_BASE_SRV_GET_IMPLEMENTATION_IDN = 0x03,
	RPMI_BASE_SRV_GET_SPEC_VERSION = 0x04,
	RPMI_BASE_SRV_GET_HW_INFO = 0x05,
	RPMI_BASE_SRV_PROBE_SERVICE_GROUP = 0x06,
	RPMI_BASE_SRV_GET_ATTRIBUTES = 0x07,
	RPMI_BASE_SRV_SET_MSI = 0x08,
};

struct rpmi_base_get_attributes_resp {
	s32 status_code;
#define RPMI_BASE_FLAGS_F0_EV_NOTIFY		(1U << 31)
#define RPMI_BASE_FLAGS_F0_MSI_EN			(1U << 30)
	u32 f0;
	u32 f1;
	u32 f2;
	u32 f3;
};

/** RPMI System Reset ServiceGroup Service IDs */
enum rpmi_system_reset_service_id {
	RPMI_SYSRST_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_SYSRST_SRV_GET_SYSTEM_RESET_ATTRIBUTES = 0x02,
	RPMI_SYSRST_SRV_SYSTEM_RESET = 0x03,
	RPMI_SYSRST_SRV_ID_MAX_COUNT,
};

/** RPMI System Reset types */
enum rpmi_sysrst_reset_type {
	RPMI_SYSRST_SHUTDOWN = 0,
	RPMI_SYSRST_COLD_RESET = 1,
	RPMI_SYSRST_WARM_RESET = 2,
	RPMI_SYSRST_MAX_IDN_COUNT,
};

/** Response for system reset attributes */
struct rpmi_sysrst_get_reset_attributes_resp {
	s32 status;
#define RPMI_SYSRST_FLAGS_SUPPORTED_POS		(31)
#define RPMI_SYSRST_FLAGS_SUPPORTED_MASK		\
			(1U << RPMI_SYSRST_FLAGS_SUPPORTED_POS)
	u32 flags;
};

/** RPMI System Suspend ServiceGroup Service IDs */
enum rpmi_system_suspend_service_id {
	RPMI_SYSSUSP_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_SYSSUSP_SRV_GET_SYSTEM_SUSPEND_ATTRIBUTES = 0x02,
	RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND = 0x03,
	RPMI_SYSSUSP_SRV_ID_MAX_COUNT,
};

/** Response for system suspend attributes */
struct rpmi_syssusp_get_attr_resp {
	s32 status;
#define RPMI_SYSSUSP_FLAGS_CUSTOM_RESUME_ADDR_SUPPORTED	(1U << 31)
#define RPMI_SYSSUSP_FLAGS_SUPPORTED			(1U << 30)
	u32 flags;
};

struct rpmi_syssusp_suspend_req {
	u32 hartid;
	u32 suspend_type;
	u32 resume_addr_lo;
	u32 resume_addr_hi;
};

struct rpmi_syssusp_suspend_resp {
	s32 status;
};

/** RPMI HSM State Management ServiceGroup Service IDs */
enum rpmi_cpu_hsm_service_id {
	RPMI_HSM_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_HSM_SRV_HART_START = 0x02,
	RPMI_HSM_SRV_HART_STOP = 0x03,
	RPMI_HSM_SRV_HART_SUSPEND = 0x04,
	RPMI_HSM_SRV_GET_HART_STATUS = 0x05,
	RPMI_HSM_SRV_GET_HART_LIST = 0x06,
	RPMI_HSM_SRV_GET_SUSPEND_TYPES = 0x07,
	RPMI_HSM_SRV_GET_SUSPEND_INFO = 0x08,
	RPMI_HSM_SRV_ID_MAX_COUNT,
};

/* HSM service group request and response structs */
struct rpmi_hsm_hart_start_req {
	u32 hartid;
	u32 start_addr_lo;
	u32 start_addr_hi;
};

struct rpmi_hsm_hart_start_resp {
	s32 status;
};

struct rpmi_hsm_hart_stop_req {
	u32 hartid;
};

struct rpmi_hsm_hart_stop_resp {
	s32 status;
};

struct rpmi_hsm_hart_susp_req {
	u32 hartid;
	u32 suspend_type;
	u32 resume_addr_lo;
	u32 resume_addr_hi;
};

struct rpmi_hsm_hart_susp_resp {
	s32 status;
};

struct rpmi_hsm_get_hart_status_req {
	u32 hartid;
};

struct rpmi_hsm_get_hart_status_resp {
	s32 status;
	u32 hart_status;
};

struct rpmi_hsm_get_hart_list_req {
	u32 start_index;
};

struct rpmi_hsm_get_hart_list_resp {
	s32 status;
	u32 remaining;
	u32 returned;
	/* remaining space need to be adjusted for the above 3 u32's */
	u32 hartid[(RPMI_MSG_DATA_SIZE - (sizeof(u32) * 3)) / sizeof(u32)];
};

struct rpmi_hsm_get_susp_types_req {
	u32 start_index;
};

struct rpmi_hsm_get_susp_types_resp {
	s32 status;
	u32 remaining;
	u32 returned;
	/* remaining space need to be adjusted for the above 3 u32's */
	u32 types[(RPMI_MSG_DATA_SIZE - (sizeof(u32) * 3)) / sizeof(u32)];
};

struct rpmi_hsm_get_susp_info_req {
	u32 suspend_type;
};

struct rpmi_hsm_get_susp_info_resp {
	s32 status;
	u32 flags;
#define RPMI_HSM_FLAGS_LOCAL_TIME_STOP     (1U << 31)
	u32 entry_latency_us;
	u32 exit_latency_us;
	u32 wakeup_latency_us;
	u32 min_residency_us;
};

#endif /* !__RPMI_MSGPROT_H__ */
