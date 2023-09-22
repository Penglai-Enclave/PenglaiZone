/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_trap.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_rpxy.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/rpxy/fdt_rpxy.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>

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

extern const struct sbi_trap_regs *trap_regs; // trap_context_pointer
struct sbi_trap_regs regs;  // secure context
uintptr_t ptbr;
uintptr_t c_rt_ctx;

/* Assembly helpers */
uint64_t spm_secure_partition_enter(struct sbi_trap_regs *regs, uintptr_t *c_rt_ctx);
void spm_secure_partition_exit(uint64_t c_rt_ctx, uint64_t ret);

struct rpxy_spm_data {
	u32 service_group_id;
	int num_services;
	struct sbi_rpxy_service *services;
};

struct rpxy_spm {
	struct sbi_rpxy_service_group group;
	struct spm_chan *chan;
};

void save_secure_context()
{
  int i;

  uintptr_t* prev = (uintptr_t*) &regs;
  uintptr_t* next = (uintptr_t*) trap_regs;
  for(i = 0; i < 33; ++i)
  {
    prev[i] = next[i];
  }

  return;
}

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
	} else if (RPMI_MM_SRV_MM_COMPLETE == srv->id) {
		sbi_printf("####  rpxy_spm_send_message RPMI_MM_SRV_COMPLETE %d %p %d %p %d %p ####\n", srv->id, tx, tx_len, rx, rx_len, ack_len);
        // save secure context
        save_secure_context();

        spm_secure_partition_exit(c_rt_ctx, 0);
	}
	return 0;
}

static int rpxy_spm_init(void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	int rc;
	struct rpxy_spm *rspm;
	const struct rpxy_spm_data *data = match->data;
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
	//rspm->chan = chan;

	/* Register RPXY service group */
	rc = sbi_rpxy_register_service_group(&rspm->group);
	if (rc) {
		sbi_free(rspm);
		return rc;
	}
    sbi_printf("rpxy_spm mm_group registered\n");

    // clear pending interrupts
	csr_read_clear(CSR_MIP, MIP_MTIP);
	csr_read_clear(CSR_MIP, MIP_STIP);
	csr_read_clear(CSR_MIP, MIP_SSIP);
	csr_read_clear(CSR_MIP, MIP_SEIP);

    unsigned long val = csr_read(CSR_MSTATUS);
	val = INSERT_FIELD(val, MSTATUS_MPP, PRV_S);
	val = INSERT_FIELD(val, MSTATUS_MPIE, 0);

    // init secure context
	csr_write(CSR_MSTATUS, val);
    regs.mstatus = val;
	csr_write(CSR_MEPC, 0x80C00000);
    regs.mepc = 0x80C00000;

    //pass parameters
	regs.a0 = current_hartid();
	regs.a1 = 0;

    csr_write(CSR_STVEC, 0x80C00000);
    csr_write(CSR_SSCRATCH, 0);
    csr_write(CSR_SIE, 0);
    csr_write(CSR_SATP, ptbr);

    __asm__ __volatile__("sfence.vma" : : : "memory");

    rc = spm_secure_partition_enter(&regs, &c_rt_ctx);
    // register unsigned long a0 asm("a0") = current_hartid();
	// register unsigned long a1 asm("a1") = 0;
    // __asm__ __volatile__("mret" : : "r"(a0), "r"(a1));

    sbi_printf("rpxy_spm_return init process, retval: %d\n", rc);

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

static struct rpxy_spm_data mm_data = {
	.service_group_id = RPMI_SRVGRP_SPM_MM,
	.num_services = array_size(mm_services),
	.services = mm_services,
};


static const struct fdt_match rpxy_spm_match[] = {
	//Need use the compartible value in the trust-doman in fdt file, use rpmi-mm !! ??
	{ .compatible = "riscv,rpmi-clock", .data = &mm_data }, 
	{},
};

struct fdt_rpxy fdt_rpxy_spm = {
	.match_table = rpxy_spm_match,
	.init = rpxy_spm_init,
};
