/* SPDX-License-Identifier: GPL-2.0*/
/* Huawei HiNIC PCI Express Linux driver
 * Copyright(c) 2017 Huawei Technologies Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#ifndef __HINIC_PORT_CMD_H__
#define __HINIC_PORT_CMD_H__

/* cmd of mgmt CPU message for NIC module */
enum hinic_port_cmd {
	HINIC_PORT_CMD_VF_REGISTER		= 0x0,
	/* not defined in base line, only for PFD and VFD */
	HINIC_PORT_CMD_VF_UNREGISTER		= 0x1,
	/* not defined in base line, only for PFD and VFD */

	HINIC_PORT_CMD_CHANGE_MTU		= 0x2,

	HINIC_PORT_CMD_ADD_VLAN			= 0x3,
	HINIC_PORT_CMD_DEL_VLAN,

	HINIC_PORT_CMD_SET_PFC			= 0x5,
	HINIC_PORT_CMD_GET_PFC,
	HINIC_PORT_CMD_SET_ETS,
	HINIC_PORT_CMD_GET_ETS,

	HINIC_PORT_CMD_SET_MAC			= 0x9,
	HINIC_PORT_CMD_GET_MAC,
	HINIC_PORT_CMD_DEL_MAC,

	HINIC_PORT_CMD_SET_RX_MODE		= 0xc,
	HINIC_PORT_CMD_SET_ANTI_ATTACK_RATE	= 0xd,

	HINIC_PORT_CMD_GET_AUTONEG_CAP		= 0xf,
	/* not defined in base line */
	HINIC_PORT_CMD_GET_AUTONET_STATE,
	/* not defined in base line */
	HINIC_PORT_CMD_GET_SPEED,
	/* not defined in base line */
	HINIC_PORT_CMD_GET_DUPLEX,
	/* not defined in base line */
	HINIC_PORT_CMD_GET_MEDIA_TYPE,
	/* not defined in base line */

	HINIC_PORT_CMD_GET_PAUSE_INFO		= 0x14,
	HINIC_PORT_CMD_SET_PAUSE_INFO,

	HINIC_PORT_CMD_GET_LINK_STATE		= 0x18,
	HINIC_PORT_CMD_SET_LRO			= 0x19,
	HINIC_PORT_CMD_SET_RX_CSUM		= 0x1a,
	HINIC_PORT_CMD_SET_RX_VLAN_OFFLOAD	= 0x1b,

	HINIC_PORT_CMD_GET_PORT_STATISTICS	= 0x1c,
	HINIC_PORT_CMD_CLEAR_PORT_STATISTICS,
	HINIC_PORT_CMD_GET_VPORT_STAT,
	HINIC_PORT_CMD_CLEAN_VPORT_STAT,

	HINIC_PORT_CMD_GET_RSS_TEMPLATE_INDIR_TBL = 0x25,
	HINIC_PORT_CMD_SET_RSS_TEMPLATE_INDIR_TBL,

	HINIC_PORT_CMD_SET_PORT_ENABLE		= 0x29,
	HINIC_PORT_CMD_GET_PORT_ENABLE,

	HINIC_PORT_CMD_SET_RSS_TEMPLATE_TBL	= 0x2b,
	HINIC_PORT_CMD_GET_RSS_TEMPLATE_TBL,
	HINIC_PORT_CMD_SET_RSS_HASH_ENGINE,
	HINIC_PORT_CMD_GET_RSS_HASH_ENGINE,
	HINIC_PORT_CMD_GET_RSS_CTX_TBL,
	HINIC_PORT_CMD_SET_RSS_CTX_TBL,
	HINIC_PORT_CMD_RSS_TEMP_MGR,

	/* 0x36 ~ 0x40 have defined in base line */

	HINIC_PORT_CMD_RSS_CFG			= 0x42,

	HINIC_PORT_CMD_GET_PHY_TYPE		= 0x44,
	HINIC_PORT_CMD_INIT_FUNC		= 0x45,
	HINIC_PORT_CMD_SET_LLI_PRI		= 0x46,

	HINIC_PORT_CMD_GET_LOOPBACK_MODE	= 0x48,
	HINIC_PORT_CMD_SET_LOOPBACK_MODE,

	HINIC_PORT_CMD_GET_JUMBO_FRAME_SIZE	= 0x4a,
	HINIC_PORT_CMD_SET_JUMBO_FRAME_SIZE,

	/* 0x4c ~ 0x57 have defined in base line */
	HINIC_PORT_CMD_DISABLE_PROMISC		= 0x4c,
	HINIC_PORT_CMD_ENABLE_SPOOFCHK		= 0x4e,
	HINIC_PORT_CMD_GET_MGMT_VERSION		= 0x58,
	HINIC_PORT_CMD_GET_BOOT_VERSION,
	HINIC_PORT_CMD_GET_MICROCODE_VERSION,

	HINIC_PORT_CMD_GET_PORT_TYPE		= 0x5b,
	/* not defined in base line */

	HINIC_PORT_CMD_GET_VPORT_ENABLE		= 0x5c,
	HINIC_PORT_CMD_SET_VPORT_ENABLE,

	HINIC_PORT_CMD_GET_PORT_ID_BY_FUNC_ID	= 0x5e,

	HINIC_PORT_CMD_SET_LED_TEST		= 0x5f,

	HINIC_PORT_CMD_SET_LLI_STATE		= 0x60,
	HINIC_PORT_CMD_SET_LLI_TYPE,
	HINIC_PORT_CMD_GET_LLI_CFG,

	HINIC_PORT_CMD_GET_LRO			= 0x63,

	HINIC_PORT_CMD_GET_DMA_CS		= 0x64,
	HINIC_PORT_CMD_SET_DMA_CS,

	HINIC_PORT_CMD_GET_GLOBAL_QPN		= 0x66,

	HINIC_PORT_CMD_SET_PFC_MISC		= 0x67,
	HINIC_PORT_CMD_GET_PFC_MISC,

	HINIC_PORT_CMD_SET_VF_RATE		= 0x69,
	HINIC_PORT_CMD_SET_VF_VLAN,
	HINIC_PORT_CMD_CLR_VF_VLAN,

	/* 0x6c,0x6e have defined in base line */
	HINIC_PORT_CMD_SET_UCAPTURE_OPT		= 0x6F,

	HINIC_PORT_CMD_SET_TSO			= 0x70,
	HINIC_PORT_CMD_SET_PHY_POWER		= 0x71,
	HINIC_PORT_CMD_UPDATE_FW		= 0x72,
	HINIC_PORT_CMD_SET_RQ_IQ_MAP		= 0x73,
	/* not defined in base line */
	HINIC_PORT_CMD_SET_PFC_THD		= 0x75,
	/* not defined in base line */
	HINIC_PORT_CMD_SET_PORT_LINK_STATUS	= 0x76,
	HINIC_PORT_CMD_SET_CGE_PAUSE_TIME_CFG	= 0x77,

	HINIC_PORT_CMD_GET_FW_SUPPORT_FLAG	= 0x79,

	HINIC_PORT_CMD_SET_PORT_REPORT		= 0x7B,

	HINIC_PORT_CMD_LINK_STATUS_REPORT	= 0xa0,

	HINIC_PORT_CMD_SET_LOSSLESS_ETH		= 0xa3,
	HINIC_PORT_CMD_UPDATE_MAC		= 0xa4,

	HINIC_PORT_CMD_GET_UART_LOG		= 0xa5,
	HINIC_PORT_CMD_SET_UART_LOG,

	HINIC_PORT_CMD_GET_PORT_INFO		= 0xaa,

	HINIC_MISC_SET_FUNC_SF_ENBITS		= 0xab,
	/* not defined in base line */
	HINIC_MISC_GET_FUNC_SF_ENBITS,
	/* not defined in base line */

	HINIC_PORT_CMD_GET_SFP_INFO		= 0xad,

	HINIC_PORT_CMD_SET_NETQ			= 0xc1,
	HINIC_PORT_CMD_ADD_RQ_FILTER		= 0xc2,
	HINIC_PORT_CMD_DEL_RQ_FILTER		= 0xc3,

	HINIC_PORT_CMD_GET_FW_LOG		= 0xca,
	HINIC_PORT_CMD_SET_IPSU_MAC		= 0xcb,
	HINIC_PORT_CMD_GET_IPSU_MAC		= 0xcc,

	HINIC_PORT_CMD_SET_XSFP_STATUS		= 0xD4,

	HINIC_PORT_CMD_SET_IQ_ENABLE		= 0xd6,

	HINIC_PORT_CMD_GET_LINK_MODE		= 0xD9,
	HINIC_PORT_CMD_SET_SPEED		= 0xDA,
	HINIC_PORT_CMD_SET_AUTONEG		= 0xDB,

	HINIC_PORT_CMD_CLEAR_SQ_RES		= 0xDD,
	HINIC_PORT_CMD_SET_SUPER_CQE		= 0xDE,
	HINIC_PORT_CMD_SET_VF_COS		= 0xDF,
	HINIC_PORT_CMD_GET_VF_COS		= 0xE1,

	HINIC_PORT_CMD_CABLE_PLUG_EVENT		= 0xE5,
	HINIC_PORT_CMD_LINK_ERR_EVENT		= 0xE6,

	HINIC_PORT_CMD_SET_PORT_FUNCS_STATE	= 0xE7,
	HINIC_PORT_CMD_SET_COS_UP_MAP		= 0xE8,

	HINIC_PORT_CMD_RESET_LINK_CFG		= 0xEB,
	HINIC_PORT_CMD_GET_STD_SFP_INFO		= 0xF0,

	HINIC_PORT_CMD_FORCE_PKT_DROP		= 0xF3,
	HINIC_PORT_CMD_SET_LRO_TIMER		= 0xF4,

	HINIC_PORT_CMD_SET_VHD_CFG		= 0xF7,
	HINIC_PORT_CMD_SET_LINK_FOLLOW		= 0xF8,
	HINIC_PORT_CMD_SET_VF_MAX_MIN_RATE	= 0xF9,
	HINIC_PORT_CMD_SET_RXQ_LRO_ADPT		= 0xFA,
	HINIC_PORT_CMD_GET_SFP_ABS		= 0xFB,
	HINIC_PORT_CMD_Q_FILTER			= 0xFC,
	HINIC_PORT_CMD_TCAM_FILTER		= 0xFE,
	HINIC_PORT_CMD_SET_VLAN_FILTER		= 0xFF,
};

/* cmd of mgmt CPU message for HW module */
enum hinic_mgmt_cmd {
	HINIC_MGMT_CMD_RESET_MGMT		= 0x0,
	HINIC_MGMT_CMD_START_FLR		= 0x1,
	HINIC_MGMT_CMD_FLUSH_DOORBELL		= 0x2,
	HINIC_MGMT_CMD_GET_IO_STATUS		= 0x3,
	HINIC_MGMT_CMD_DMA_ATTR_SET		= 0x4,

	HINIC_MGMT_CMD_CMDQ_CTXT_SET		= 0x10,
	HINIC_MGMT_CMD_CMDQ_CTXT_GET,

	HINIC_MGMT_CMD_VAT_SET			= 0x12,
	HINIC_MGMT_CMD_VAT_GET,

	HINIC_MGMT_CMD_L2NIC_SQ_CI_ATTR_SET	= 0x14,
	HINIC_MGMT_CMD_L2NIC_SQ_CI_ATTR_GET,

	HINIC_MGMT_CMD_MQM_FIX_INFO_GET		= 0x16,
	HINIC_MGMT_CMD_MQM_CFG_INFO_SET		= 0x18,
	HINIC_MGMT_CMD_MQM_SRCH_GPA_SET		= 0x20,
	HINIC_MGMT_CMD_PPF_TMR_SET		= 0x22,
	HINIC_MGMT_CMD_PPF_HT_GPA_SET		= 0x23,
	HINIC_MGMT_CMD_RES_STATE_SET		= 0x24,
	HINIC_MGMT_CMD_FUNC_CACHE_OUT		= 0x25,
	HINIC_MGMT_CMD_FFM_SET			= 0x26,
	HINIC_MGMT_CMD_SMF_TMR_CLEAR		= 0x27,
	/* 0x29 not defined in base line,
	 * only used in open source driver
	 */
	HINIC_MGMT_CMD_FUNC_RES_CLEAR		= 0x29,

	HINIC_MGMT_CMD_FUNC_TMR_BITMAT_SET	= 0x32,

	HINIC_MGMT_CMD_CEQ_CTRL_REG_WR_BY_UP	= 0x33,
	HINIC_MGMT_CMD_MSI_CTRL_REG_WR_BY_UP,
	HINIC_MGMT_CMD_MSI_CTRL_REG_RD_BY_UP,

	HINIC_MGMT_CMD_VF_RANDOM_ID_SET		= 0x36,
	HINIC_MGMT_CMD_FAULT_REPORT		= 0x37,
	HINIC_MGMT_CMD_HEART_LOST_REPORT	= 0x38,

	HINIC_MGMT_CMD_VPD_SET			= 0x40,
	HINIC_MGMT_CMD_VPD_GET,
	HINIC_MGMT_CMD_LABEL_SET,
	HINIC_MGMT_CMD_LABEL_GET,
	HINIC_MGMT_CMD_SATIC_MAC_SET,
	HINIC_MGMT_CMD_SATIC_MAC_GET,
	HINIC_MGMT_CMD_SYNC_TIME		= 0x46,

	HINIC_MGMT_CMD_REG_READ			= 0x48,

	HINIC_MGMT_CMD_SET_LED_STATUS		= 0x4A,
	HINIC_MGMT_CMD_L2NIC_RESET		= 0x4b,
	HINIC_MGMT_CMD_FAST_RECYCLE_MODE_SET	= 0x4d,
	HINIC_MGMT_CMD_BIOS_NV_DATA_MGMT	= 0x4E,
	HINIC_MGMT_CMD_ACTIVATE_FW		= 0x4F,
	HINIC_MGMT_CMD_PAGESIZE_SET		= 0x50,
	HINIC_MGMT_CMD_PAGESIZE_GET		= 0x51,
	HINIC_MGMT_CMD_GET_BOARD_INFO		= 0x52,
	HINIC_MGMT_CMD_WATCHDOG_INFO		= 0x56,
	HINIC_MGMT_CMD_FMW_ACT_NTC		= 0x57,
	HINIC_MGMT_CMD_SET_VF_RANDOM_ID		= 0x61,
	HINIC_MGMT_CMD_GET_PPF_STATE		= 0x63,
	HINIC_MGMT_CMD_PCIE_DFX_NTC		= 0x65,
	HINIC_MGMT_CMD_PCIE_DFX_GET		= 0x66,

	HINIC_MGMT_CMD_GET_HOST_INFO		= 0x67,

	HINIC_MGMT_CMD_GET_PHY_INIT_STATUS	= 0x6A,
	HINIC_MGMT_CMD_HEARTBEAT_SUPPORTED	= 0x6B,
	HINIC_MGMT_CMD_HEARTBEAT_EVENT		= 0x6C,
	HINIC_MGMT_CMD_GET_HW_PF_INFOS		= 0x6D,
	HINIC_MGMT_CMD_GET_SDI_MODE		= 0x6E,

	HINIC_MGMT_CMD_ENABLE_MIGRATE		= 0x6F,
};

/* uCode relates commands */
enum hinic_ucode_cmd {
	HINIC_UCODE_CMD_MODIFY_QUEUE_CONTEXT	= 0,
	HINIC_UCODE_CMD_CLEAN_QUEUE_CONTEXT,
	HINIC_UCODE_CMD_ARM_SQ,
	HINIC_UCODE_CMD_ARM_RQ,
	HINIC_UCODE_CMD_SET_RSS_INDIR_TABLE,
	HINIC_UCODE_CMD_SET_RSS_CONTEXT_TABLE,
	HINIC_UCODE_CMD_GET_RSS_INDIR_TABLE,
	HINIC_UCODE_CMD_GET_RSS_CONTEXT_TABLE,
	HINIC_UCODE_CMD_SET_IQ_ENABLE,
	HINIC_UCODE_CMD_SET_RQ_FLUSH		= 10
};

/* software cmds, vf->pf and multi-host */
enum hinic_sw_funcs_cmd {
	HINIC_SW_CMD_SLAVE_HOST_PPF_REGISTER	= 0x0,
	HINIC_SW_CMD_SLAVE_HOST_PPF_UNREGISTER	= 0x1,
	HINIC_SW_CMD_GET_SLAVE_FUNC_NIC_STATE	= 0x2,
	HINIC_SW_CMD_SET_SLAVE_FUNC_NIC_STATE	= 0x3,
	HINIC_SW_CMD_SEND_MSG_TO_VF             = 0x4,
	HINIC_SW_CMD_MIGRATE_READY		= 0x5,
};

enum sq_l4offload_type {
	OFFLOAD_DISABLE   = 0,
	TCP_OFFLOAD_ENABLE  = 1,
	SCTP_OFFLOAD_ENABLE = 2,
	UDP_OFFLOAD_ENABLE  = 3,
};

enum sq_vlan_offload_flag {
	VLAN_OFFLOAD_DISABLE = 0,
	VLAN_OFFLOAD_ENABLE  = 1,
};

enum sq_pkt_parsed_flag {
	PKT_NOT_PARSED = 0,
	PKT_PARSED     = 1,
};

enum sq_l3_type {
	UNKNOWN_L3TYPE = 0,
	IPV6_PKT = 1,
	IPV4_PKT_NO_CHKSUM_OFFLOAD = 2,
	IPV4_PKT_WITH_CHKSUM_OFFLOAD = 3,
};

enum sq_md_type {
	UNKNOWN_MD_TYPE = 0,
};

enum sq_l2type {
	ETHERNET = 0,
};

enum sq_tunnel_l4_type {
	NOT_TUNNEL,
	TUNNEL_UDP_NO_CSUM,
	TUNNEL_UDP_CSUM,
};

#define NIC_RSS_CMD_TEMP_ALLOC				0x01
#define NIC_RSS_CMD_TEMP_FREE				0x02

#define HINIC_RSS_TYPE_VALID_SHIFT			23
#define HINIC_RSS_TYPE_TCP_IPV6_EXT_SHIFT		24
#define HINIC_RSS_TYPE_IPV6_EXT_SHIFT			25
#define HINIC_RSS_TYPE_TCP_IPV6_SHIFT			26
#define HINIC_RSS_TYPE_IPV6_SHIFT			27
#define HINIC_RSS_TYPE_TCP_IPV4_SHIFT			28
#define HINIC_RSS_TYPE_IPV4_SHIFT			29
#define HINIC_RSS_TYPE_UDP_IPV6_SHIFT			30
#define HINIC_RSS_TYPE_UDP_IPV4_SHIFT			31

#define HINIC_RSS_TYPE_SET(val, member)		\
		(((u32)(val) & 0x1) << HINIC_RSS_TYPE_##member##_SHIFT)

#define HINIC_RSS_TYPE_GET(val, member)		\
		(((u32)(val) >> HINIC_RSS_TYPE_##member##_SHIFT) & 0x1)

enum hinic_speed {
	HINIC_SPEED_10MB_LINK = 0,
	HINIC_SPEED_100MB_LINK,
	HINIC_SPEED_1000MB_LINK,
	HINIC_SPEED_10GB_LINK,
	HINIC_SPEED_25GB_LINK,
	HINIC_SPEED_40GB_LINK,
	HINIC_SPEED_100GB_LINK,
	HINIC_SPEED_UNKNOWN = 0xFF,
};

/* In order to adapt different linux version */
enum {
	HINIC_IFLA_VF_LINK_STATE_AUTO,	/* link state of the uplink */
	HINIC_IFLA_VF_LINK_STATE_ENABLE,	/* link always up */
	HINIC_IFLA_VF_LINK_STATE_DISABLE,	/* link always down */
};

#define HINIC_AF0_FUNC_GLOBAL_IDX_SHIFT		0
#define HINIC_AF0_P2P_IDX_SHIFT			10
#define HINIC_AF0_PCI_INTF_IDX_SHIFT		14
#define HINIC_AF0_VF_IN_PF_SHIFT		16
#define HINIC_AF0_FUNC_TYPE_SHIFT		24

#define HINIC_AF0_FUNC_GLOBAL_IDX_MASK		0x3FF
#define HINIC_AF0_P2P_IDX_MASK			0xF
#define HINIC_AF0_PCI_INTF_IDX_MASK		0x3
#define HINIC_AF0_VF_IN_PF_MASK			0xFF
#define HINIC_AF0_FUNC_TYPE_MASK		0x1

#define HINIC_AF0_GET(val, member)				\
	(((val) >> HINIC_AF0_##member##_SHIFT) & HINIC_AF0_##member##_MASK)

#define HINIC_AF1_PPF_IDX_SHIFT			0
#define HINIC_AF1_AEQS_PER_FUNC_SHIFT		8
#define HINIC_AF1_CEQS_PER_FUNC_SHIFT		12
#define HINIC_AF1_IRQS_PER_FUNC_SHIFT		20
#define HINIC_AF1_DMA_ATTR_PER_FUNC_SHIFT	24
#define HINIC_AF1_MGMT_INIT_STATUS_SHIFT	30
#define HINIC_AF1_PF_INIT_STATUS_SHIFT		31

#define HINIC_AF1_PPF_IDX_MASK			0x1F
#define HINIC_AF1_AEQS_PER_FUNC_MASK		0x3
#define HINIC_AF1_CEQS_PER_FUNC_MASK		0x7
#define HINIC_AF1_IRQS_PER_FUNC_MASK		0xF
#define HINIC_AF1_DMA_ATTR_PER_FUNC_MASK	0x7
#define HINIC_AF1_MGMT_INIT_STATUS_MASK		0x1
#define HINIC_AF1_PF_INIT_STATUS_MASK		0x1

#define HINIC_AF1_GET(val, member)				\
	(((val) >> HINIC_AF1_##member##_SHIFT) & HINIC_AF1_##member##_MASK)

#define HINIC_AF2_GLOBAL_VF_ID_OF_PF_SHIFT	16
#define HINIC_AF2_GLOBAL_VF_ID_OF_PF_MASK	0x3FF

#define HINIC_AF2_GET(val, member)				\
	(((val) >> HINIC_AF2_##member##_SHIFT) & HINIC_AF2_##member##_MASK)

#define HINIC_AF4_OUTBOUND_CTRL_SHIFT		0
#define HINIC_AF4_DOORBELL_CTRL_SHIFT		1
#define HINIC_AF4_OUTBOUND_CTRL_MASK		0x1
#define HINIC_AF4_DOORBELL_CTRL_MASK		0x1

#define HINIC_AF4_GET(val, member)				\
	(((val) >> HINIC_AF4_##member##_SHIFT) & HINIC_AF4_##member##_MASK)

#define HINIC_AF4_SET(val, member)				\
	(((val) & HINIC_AF4_##member##_MASK) << HINIC_AF4_##member##_SHIFT)

#define HINIC_AF4_CLEAR(val, member)				\
	((val) & (~(HINIC_AF4_##member##_MASK <<		\
	HINIC_AF4_##member##_SHIFT)))

#define HINIC_AF5_PF_STATUS_SHIFT		0
#define HINIC_AF5_PF_STATUS_MASK		0xFFFF

#define HINIC_AF5_SET(val, member)				\
	(((val) & HINIC_AF5_##member##_MASK) << HINIC_AF5_##member##_SHIFT)

#define HINIC_AF5_GET(val, member)				\
	(((val) >> HINIC_AF5_##member##_SHIFT) & HINIC_AF5_##member##_MASK)

#define HINIC_AF5_CLEAR(val, member)				\
	((val) & (~(HINIC_AF5_##member##_MASK <<		\
	HINIC_AF5_##member##_SHIFT)))

#define HINIC_PPF_ELECTION_IDX_SHIFT		0

#define HINIC_PPF_ELECTION_IDX_MASK		0x1F

#define HINIC_PPF_ELECTION_SET(val, member)			\
	(((val) & HINIC_PPF_ELECTION_##member##_MASK) <<	\
		HINIC_PPF_ELECTION_##member##_SHIFT)

#define HINIC_PPF_ELECTION_GET(val, member)			\
	(((val) >> HINIC_PPF_ELECTION_##member##_SHIFT) &	\
		HINIC_PPF_ELECTION_##member##_MASK)

#define HINIC_PPF_ELECTION_CLEAR(val, member)			\
	((val) & (~(HINIC_PPF_ELECTION_##member##_MASK	\
		<< HINIC_PPF_ELECTION_##member##_SHIFT)))

#define HINIC_MPF_ELECTION_IDX_SHIFT		0

#define HINIC_MPF_ELECTION_IDX_MASK		0x1F

#define HINIC_MPF_ELECTION_SET(val, member)			\
	(((val) & HINIC_MPF_ELECTION_##member##_MASK) <<	\
		HINIC_MPF_ELECTION_##member##_SHIFT)

#define HINIC_MPF_ELECTION_GET(val, member)			\
	(((val) >> HINIC_MPF_ELECTION_##member##_SHIFT) &	\
		HINIC_MPF_ELECTION_##member##_MASK)

#define HINIC_MPF_ELECTION_CLEAR(val, member)			\
	((val) & (~(HINIC_MPF_ELECTION_##member##_MASK	\
		<< HINIC_MPF_ELECTION_##member##_SHIFT)))

#define HINIC_HWIF_NUM_AEQS(hwif)		((hwif)->attr.num_aeqs)
#define HINIC_HWIF_NUM_CEQS(hwif)		((hwif)->attr.num_ceqs)
#define HINIC_HWIF_NUM_IRQS(hwif)		((hwif)->attr.num_irqs)
#define HINIC_HWIF_GLOBAL_IDX(hwif)		((hwif)->attr.func_global_idx)
#define HINIC_HWIF_GLOBAL_VF_OFFSET(hwif) ((hwif)->attr.global_vf_id_of_pf)
#define HINIC_HWIF_PPF_IDX(hwif)		((hwif)->attr.ppf_idx)
#define HINIC_PCI_INTF_IDX(hwif)		((hwif)->attr.pci_intf_idx)

#define HINIC_FUNC_TYPE(dev)		((dev)->hwif->attr.func_type)
#define HINIC_IS_PF(dev)		(HINIC_FUNC_TYPE(dev) == TYPE_PF)
#define HINIC_IS_VF(dev)		(HINIC_FUNC_TYPE(dev) == TYPE_VF)
#define HINIC_IS_PPF(dev)		(HINIC_FUNC_TYPE(dev) == TYPE_PPF)

#define DB_IDX(db, db_base)	\
	((u32)(((ulong)(db) - (ulong)(db_base)) /	\
	       HINIC_DB_PAGE_SIZE))

enum hinic_pcie_nosnoop {
	HINIC_PCIE_SNOOP = 0,
	HINIC_PCIE_NO_SNOOP = 1,
};

enum hinic_pcie_tph {
	HINIC_PCIE_TPH_DISABLE = 0,
	HINIC_PCIE_TPH_ENABLE = 1,
};

enum hinic_outbound_ctrl {
	ENABLE_OUTBOUND  = 0x0,
	DISABLE_OUTBOUND = 0x1,
};

enum hinic_doorbell_ctrl {
	ENABLE_DOORBELL  = 0x0,
	DISABLE_DOORBELL = 0x1,
};

enum hinic_pf_status {
	HINIC_PF_STATUS_INIT = 0x0,
	HINIC_PF_STATUS_ACTIVE_FLAG = 0x11,
	HINIC_PF_STATUS_FLR_START_FLAG = 0x12,
	HINIC_PF_STATUS_FLR_FINISH_FLAG = 0x13,
};

/* total doorbell or direct wqe size is 512kB, db num: 128, dwqe: 128 */
#define HINIC_DB_DWQE_SIZE      0x00080000
/* BMGW & VMGW VF db size 256k, have no dwqe space */
#define HINIC_GW_VF_DB_SIZE	0x00040000

/* db/dwqe page size: 4K */
#define HINIC_DB_PAGE_SIZE	0x00001000ULL

#define HINIC_DB_MAX_AREAS	(HINIC_DB_DWQE_SIZE / HINIC_DB_PAGE_SIZE)

#define HINIC_PCI_MSIX_ENTRY_SIZE			16
#define HINIC_PCI_MSIX_ENTRY_VECTOR_CTRL		12
#define HINIC_PCI_MSIX_ENTRY_CTRL_MASKBIT		1

#endif /* __HINIC_PORT_CMD_H__ */