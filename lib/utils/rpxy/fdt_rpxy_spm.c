/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) IPADS@SJTU 2023. All rights reserved.
 *
 * Authors:
 *   Qingyu Shang <2931013282@sjtu.edu.cn>
 */

#include <sbi/sbi_trap.h>
#include <sbi/riscv_asm.h>

#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_rpxy.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/rpxy/fdt_rpxy.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi/sbi_console.h>

/** Representation of Secure Partition context */
struct sp_context {
    /** secure context for all general registers */
	struct sbi_trap_regs regs;
    /** secure context for S mode CSR registers */
    uint64_t csr_stvec;
    uint64_t csr_sscratch;
    uint64_t csr_sie;
    uint64_t csr_satp;
    /**
     * stack address to restore C runtime context from after
     * returning from a synchronous entry into Secure Partition.
     */
    uintptr_t c_rt_ctx;
};

/*
 * Save current CSR registers context and restore original context.
 */
static void save_restore_csr_context(struct sp_context *ctx)
{
    uint64_t tmp;

    tmp = ctx->csr_stvec;
    ctx->csr_stvec = csr_read(CSR_STVEC);
    csr_write(CSR_STVEC, tmp);

    tmp = ctx->csr_sscratch;
    ctx->csr_sscratch = csr_read(CSR_SSCRATCH);
    csr_write(CSR_SSCRATCH, tmp);

    tmp = ctx->csr_sie;
    ctx->csr_sie = csr_read(CSR_SIE);
    csr_write(CSR_SIE, tmp);

    tmp = ctx->csr_satp;
    ctx->csr_satp = csr_read(CSR_SATP);
    csr_write(CSR_SATP, tmp);
}

/** Assembly helpers */
uint64_t spm_secure_partition_enter(struct sbi_trap_regs *regs, uintptr_t *c_rt_ctx);
void spm_secure_partition_exit(uint64_t c_rt_ctx, uint64_t ret);

/**
 * This function takes an SP context pointer and performs a synchronous entry
 * into it.
 * @param ctx pointer to SP context
 * @return 0 on success
 * @return other values decided by SP if it encounters an exception while running
 */
static uint64_t spm_sp_synchronous_entry(struct sp_context *ctx)
{
	uint64_t rc;

	/* Save current CSR context and setup Secure Partition's CSR context */
	save_restore_csr_context(ctx);

	/* Enter Secure Partition */
	rc = spm_secure_partition_enter(&ctx->regs, &ctx->c_rt_ctx);

	return rc;
}

/**
 * This function returns to the place where spm_sp_synchronous_entry() was
 * called originally.
 * @param ctx pointer to SP context
 * @param rc the return value for the original entry call
 */
static void spm_sp_synchronous_exit(struct sp_context *ctx, uint64_t rc)
{
	/* Save secure state */
	uintptr_t *prev = (uintptr_t *)&ctx->regs;
	uintptr_t *trap_regs = (uintptr_t *)(csr_read(CSR_MSCRATCH) - SBI_TRAP_REGS_SIZE);
	for (int i = 0; i < SBI_TRAP_REGS_SIZE / __SIZEOF_POINTER__; ++i) {
		prev[i] = trap_regs[i];
	}

    /* Set SBI Err and Ret */
    ctx->regs.a0 = SBI_SUCCESS;
    ctx->regs.a1 = 0;

    /* Set MEPC to next instruction */
    ctx->regs.mepc = ctx->regs.mepc + 4;

    /* Save Secure Partition's CSR context and restore original CSR context */
	save_restore_csr_context(ctx);

	/*
	 * The SPM must have initiated the original request through a
	 * synchronous entry into the secure partition. Jump back to the
	 * original C runtime context with the value of rc in a0;
	 */
	spm_secure_partition_exit(ctx->c_rt_ctx, rc);
}

#define MM_VERSION_MAJOR        1
#define MM_VERSION_MAJOR_SHIFT  16
#define MM_VERSION_MAJOR_MASK   0x7FFF
#define MM_VERSION_MINOR        0
#define MM_VERSION_MINOR_SHIFT  0
#define MM_VERSION_MINOR_MASK   0xFFFF
#define MM_VERSION_FORM(major, minor) ((major << MM_VERSION_MAJOR_SHIFT) | \
                                       (minor))
#define MM_VERSION_COMPILED     MM_VERSION_FORM(MM_VERSION_MAJOR, \
                                                MM_VERSION_MINOR)

static struct sp_context mm_context;

struct efi_param_header {
	uint8_t type;	/* type of the structure */
	uint8_t version; /* version of this structure */
	uint16_t size;   /* size of this structure in bytes */
	uint32_t attr;   /* attributes: unused bits SBZ */
};

struct efi_secure_partition_cpu_info {
	uint64_t mpidr;
	uint32_t linear_id;
	uint32_t flags;
};

struct efi_secure_partition_boot_info {
	struct efi_param_header header;
	uint64_t sp_mem_base;
	uint64_t sp_mem_limit;
	uint64_t sp_image_base;
	uint64_t sp_stack_base;
	uint64_t sp_heap_base;
	uint64_t sp_ns_comm_buf_base;
	uint64_t sp_shared_buf_base;
	uint64_t sp_image_size;
	uint64_t sp_pcpu_stack_size;
	uint64_t sp_heap_size;
	uint64_t sp_ns_comm_buf_size;
	uint64_t sp_shared_buf_size;
	uint32_t num_sp_mem_region;
	uint32_t num_cpus;
	struct efi_secure_partition_cpu_info *cpu_info;
};

struct efi_secure_shared_buffer {
	struct efi_secure_partition_boot_info mm_payload_boot_info;
	struct efi_secure_partition_cpu_info mm_cpu_info[1];
};

void set_mm_boot_args(struct sbi_trap_regs *regs)
{
    /* Fix me: where to setup boot_info for secure partition? */
	struct efi_secure_shared_buffer *mm_shared_buffer = (struct efi_secure_shared_buffer *)0x80B00000;

	mm_shared_buffer->mm_payload_boot_info.header.version = 0x01;
	mm_shared_buffer->mm_payload_boot_info.sp_mem_base	= 0x80C00000;
	mm_shared_buffer->mm_payload_boot_info.sp_mem_limit	= 0x82000000;
	mm_shared_buffer->mm_payload_boot_info.sp_image_base = 0x80C00000; // sp_mem_base
	mm_shared_buffer->mm_payload_boot_info.sp_stack_base =
		0x81FFFFFF; // sp_heap_base + sp_heap_size + SpStackSize
	mm_shared_buffer->mm_payload_boot_info.sp_heap_base =
		0x80F00000; // sp_mem_base + sp_image_size
	mm_shared_buffer->mm_payload_boot_info.sp_ns_comm_buf_base = 0xFFE00000;
	mm_shared_buffer->mm_payload_boot_info.sp_shared_buf_base = 0x81F80000;
	mm_shared_buffer->mm_payload_boot_info.sp_image_size	 = 0x300000;
	mm_shared_buffer->mm_payload_boot_info.sp_pcpu_stack_size = 0x10000;
	mm_shared_buffer->mm_payload_boot_info.sp_heap_size	 = 0x800000;
	mm_shared_buffer->mm_payload_boot_info.sp_ns_comm_buf_size = 0x200000;
	mm_shared_buffer->mm_payload_boot_info.sp_shared_buf_size = 0x80000;
	mm_shared_buffer->mm_payload_boot_info.num_sp_mem_region = 0x6;
	mm_shared_buffer->mm_payload_boot_info.num_cpus	 = 1;
	mm_shared_buffer->mm_cpu_info[0].linear_id		 = 0;
	mm_shared_buffer->mm_cpu_info[0].flags		 = 0;
	mm_shared_buffer->mm_payload_boot_info.cpu_info = mm_shared_buffer->mm_cpu_info;

    regs->a0 = current_hartid();
	regs->a1 = (uintptr_t)&mm_shared_buffer->mm_payload_boot_info;
}

/*
 * StandaloneMm early initialization.
 */
int spm_mm_init()
{
    int rc;

    /* clear pending interrupts */
	csr_read_clear(CSR_MIP, MIP_MTIP);
	csr_read_clear(CSR_MIP, MIP_STIP);
	csr_read_clear(CSR_MIP, MIP_SSIP);
	csr_read_clear(CSR_MIP, MIP_SEIP);

    unsigned long val = csr_read(CSR_MSTATUS);
	val = INSERT_FIELD(val, MSTATUS_MPP, PRV_S);
	val = INSERT_FIELD(val, MSTATUS_MPIE, 0);

    /* init secure context */
    mm_context.regs.mstatus = val;
    /* Fix me: entry point value in domain information of spm_mm */
    mm_context.regs.mepc = 0x80C00000;

    /* set boot arguments */
    set_mm_boot_args(&mm_context.regs);

    /* init secure CSR context */
    mm_context.csr_stvec = 0x80C00000;
    mm_context.csr_sscratch = 0;
    mm_context.csr_sie = 0;
    mm_context.csr_satp = 0;

    __asm__ __volatile__("sfence.vma" : : : "memory");

    rc = spm_sp_synchronous_entry(&mm_context);

    sbi_printf("spm_mm_init finish, retval: %d\n", rc);
    return rc;
}

struct rpxy_spm_mm_data {
	u32 service_group_id;
	int num_services;
	struct sbi_rpxy_service *services;
};

struct rpxy_spm {
	struct sbi_rpxy_service_group group;
};

static int rpxy_spm_send_message(struct sbi_rpxy_service_group *grp,
				  struct sbi_rpxy_service *srv,
				  void *tx, u32 tx_len,
				  void *rx, u32 rx_len,
				  unsigned long *ack_len)
{
    sbi_printf("#### rpxy_spm_send_message %d %p %d %p %d %p ####\n", srv->id, tx, tx_len, rx, rx_len, ack_len);
	if (RPMI_MM_SRV_MM_VERSION == srv->id) {
		*((u32 *)rx) = MM_VERSION_COMPILED;
	} else if (RPMI_MM_SRV_MM_COMMUNICATE == srv->id) {
		sbi_printf("#### rpxy_spm_send_message RPMI_MM_SRV_COMMUNICATE %d %p %d %p %d %p ####\n", srv->id, tx, tx_len, rx, rx_len, ack_len);
        spm_sp_synchronous_entry(&mm_context);
	} else if (RPMI_MM_SRV_MM_COMPLETE == srv->id) {
		sbi_printf("####  rpxy_spm_send_message RPMI_MM_SRV_COMPLETE %d %p %d %p %d %p ####\n", srv->id, tx, tx_len, rx, rx_len, ack_len);
        spm_sp_synchronous_exit(&mm_context, 0);
	}
	return 0;
}

static int rpxy_spm_mm_init(void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	int rc;
	struct rpxy_spm *rspm;
	const struct rpxy_spm_mm_data *data = match->data;
    sbi_printf("rpxy_spm_init\n");

	/* Allocate context for RPXY mbox client */
	rspm = sbi_zalloc(sizeof(*rspm));
	if (!rspm)
		return SBI_ENOMEM;

	/* Setup RPXY spm client */
	rspm->group.transport_id = 0;
	rspm->group.service_group_id = data->service_group_id;
	rspm->group.max_message_data_len = -1;
	rspm->group.num_services = data->num_services;
	rspm->group.services = data->services;
	rspm->group.send_message = rpxy_spm_send_message;

	/* Register RPXY service group */
	rc = sbi_rpxy_register_service_group(&rspm->group);
	if (rc) {
		sbi_free(rspm);
		return rc;
	}
    sbi_printf("rpxy_spm mm_group registered\n");

    spm_mm_init();

	return 0;
}

static struct sbi_rpxy_service mm_services[] = {
{
	.id = RPMI_MM_SRV_MM_VERSION,
	.min_tx_len = 0,
	.max_tx_len = 0,
	.min_rx_len = sizeof(u32),
	.max_rx_len = sizeof(u32),
},
{
	.id = RPMI_MM_SRV_MM_COMMUNICATE,
	.min_tx_len = 0,
	.max_tx_len = 0x1000,
	.min_rx_len = 0,
	.max_rx_len = 0,
},
{
	.id = RPMI_MM_SRV_MM_COMPLETE,
	.min_tx_len = 0,
	.max_tx_len = 0x1000,
	.min_rx_len = 0,
	.max_rx_len = 0,
},
};

static struct rpxy_spm_mm_data mm_data = {
	.service_group_id = RPMI_SRVGRP_SPM_MM,
	.num_services = array_size(mm_services),
	.services = mm_services,
};

static const struct fdt_match rpxy_spm_mm_match[] = {
	{ .compatible = "riscv,rpmi-spm-mm", .data = &mm_data }, 
	{},
};

struct fdt_rpxy fdt_rpxy_spm = {
	.match_table = rpxy_spm_mm_match,
	.init = rpxy_spm_mm_init,
};
