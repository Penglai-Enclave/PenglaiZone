/*
 * Authors:
 *   Dong Du <Dd_nirvana@sjtu.edu.cn>
 *   Erhu Feng <2748250768@qq.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sm/sm.h>


/* SBI function IDs for COVE extension */
#define SBI_COVE_SMM_VERSION		0x80
#define SBI_COVE_SMM_COMMUNICATE	0x81
#define SBI_COVE_SMM_EVENT_COMPLETE 0x82

static int sbi_ecall_smm_host_handler(unsigned long extid, unsigned long funcid,
		const struct sbi_trap_regs *regs, unsigned long *out_val,
		struct sbi_trap_info *out_trap)
{
	uintptr_t ret = 0;

	((struct sbi_trap_regs *)regs)->mepc += 4;

	switch (funcid) {
		case SBI_COVE_SMM_VERSION:
			ret = sm_smm_version((uintptr_t *)regs, regs->a1);
			break;
		case SBI_COVE_SMM_COMMUNICATE:
		    ret = sm_smm_communicate((uintptr_t *)regs, funcid, regs->a0, regs->a1);
		    sbi_printf("[Penglai@Monitor] host interface SBI_SMM_COMMUNICATE (funcid:%ld) \n", funcid);
		    break;
        case SBI_COVE_SMM_EVENT_COMPLETE:
            ret = sm_smm_exit((uintptr_t *)regs, regs->a1);
		    sbi_printf("[Penglai@Monitor] mm interface SBI_COVE_SMM_EVENT_COMPLETE (funcid:%ld) \n", funcid);
			break;
		default:
			sbi_printf("[Penglai@Monitor] host interface(funcid:%ld) not supported yet\n", funcid);
			ret = SBI_ENOTSUPP;
	}
	*out_val = ret;
	return ret;
}

struct sbi_ecall_extension ecall_smm_host = {
	.extid_start = SBI_EXT_COVE,
	.extid_end = SBI_EXT_COVE,
	.handle = sbi_ecall_smm_host_handler,
};

/* SBI function IDs for MMSTUB extension */
#define SBI_COVE_SMM_INIT_COMPLETE		0x82
#define SBI_COVE_SMM_EXIT				0x83

static int sbi_ecall_smm_stub_handler(unsigned long extid, unsigned long funcid,
		const struct sbi_trap_regs *regs, unsigned long *out_val,
		struct sbi_trap_info *out_trap)
{
	uintptr_t ret = 0;

	((struct sbi_trap_regs *)regs)->mepc += 4;

	switch (funcid) {
		case SBI_COVE_SMM_INIT_COMPLETE:
			ret = sm_smm_init_complete((uintptr_t *)regs);
		    sbi_printf("[Penglai@Monitor] mmstub interface SBI_COVE_SMM_INIT_COMPLETE (funcid:%ld) \n", funcid);
			break;
		case SBI_COVE_SMM_EXIT:
			ret = sm_smm_exit((uintptr_t *)regs, regs->a1);
		    // sbi_printf("[Penglai@Monitor] mmstub interface SBI_COVE_SMM_EXIT (funcid:%ld) \n", funcid);
			break;
		default:
			sbi_printf("[Penglai@Monitor] mmstub interface(funcid:%ld) not supported yet\n", funcid);
			ret = SBI_ENOTSUPP;
	}
	*out_val = ret;
	return ret;
}

struct sbi_ecall_extension ecall_smm_stub = {
	.extid_start = SBI_EXT_MMSTUB,
	.extid_end = SBI_EXT_MMSTUB,
	.handle = sbi_ecall_smm_stub_handler,
};

static int sbi_ecall_penglai_host_handler(unsigned long extid, unsigned long funcid,
		const struct sbi_trap_regs *regs, unsigned long *out_val,
		struct sbi_trap_info *out_trap)
{
	uintptr_t ret = 0;

	//csr_write(CSR_MEPC, regs->mepc + 4);
	((struct sbi_trap_regs *)regs)->mepc += 4;

	switch (funcid) {
		// The following is the Penglai's Handler
		case SBI_MM_INIT:
			ret = sm_mm_init(regs->a0, regs->a1);
			break;
		case SBI_MEMORY_EXTEND:
			ret = sm_mm_extend(regs->a0, regs->a1);
			break;
		case SBI_ALLOC_ENCLAVE_MM:
			ret = sm_alloc_enclave_mem(regs->a0);
			break;
		case SBI_CREATE_ENCLAVE:
			ret = sm_create_enclave(regs->a0);
			break;
		case SBI_RUN_ENCLAVE:
			ret = sm_run_enclave((uintptr_t *)regs, regs->a0);
			break;
		case SBI_ATTEST_ENCLAVE:
			ret = sm_attest_enclave(regs->a0, regs->a1, regs->a2);
			break;
		case SBI_STOP_ENCLAVE:
			ret = sm_stop_enclave((uintptr_t *)regs, regs->a0);
			break;
		case SBI_RESUME_ENCLAVE:
			ret = sm_resume_enclave((uintptr_t *)regs, regs->a0);
			break;
		case SBI_DESTROY_ENCLAVE:
			ret = sm_destroy_enclave((uintptr_t *)regs, regs->a0);
			break;
		case SBI_SMM_VERSION:
			ret = sm_smm_version((uintptr_t *)regs, regs->a1);
			break;
		case SBI_SMM_COMMUNICATE:
		    ret = sm_smm_communicate((uintptr_t *)regs, funcid, regs->a0, regs->a1);
		    sbi_printf("[Penglai@Monitor] host interface SBI_SMM_COMMUNICATE (funcid:%ld) \n", funcid);
		    break;
		default:
			sbi_printf("[Penglai@Monitor] host interface(funcid:%ld) not supported yet\n", funcid);
			ret = SBI_ENOTSUPP;
	}
	//((struct sbi_trap_regs *)regs)->mepc = csr_read(CSR_MEPC);
	//((struct sbi_trap_regs *)regs)->mstatus = csr_read(CSR_MSTATUS);
	*out_val = ret;
	return ret;
}

struct sbi_ecall_extension ecall_penglai_host = {
	.extid_start = SBI_EXT_PENGLAI_HOST,
	.extid_end = SBI_EXT_PENGLAI_HOST,
	.handle = sbi_ecall_penglai_host_handler,
};

static int sbi_ecall_penglai_enclave_handler(unsigned long extid, unsigned long funcid,
		const struct sbi_trap_regs *regs, unsigned long *out_val,
		struct sbi_trap_info *out_trap)
{
	uintptr_t ret = 0;

	//csr_write(CSR_MEPC, regs->mepc + 4);
	((struct sbi_trap_regs *)regs)->mepc += 4;

	switch (funcid) {
		// The following is the Penglai's Handler
		case SBI_EXIT_ENCLAVE:
			ret = sm_exit_enclave((uintptr_t *)regs, regs->a0);
			break;
		case SBI_ENCLAVE_OCALL:
			ret = sm_enclave_ocall((uintptr_t *)regs, regs->a0, regs->a1, regs->a2);
			break;
		case SBI_GET_KEY:
			ret = sm_enclave_get_key((uintptr_t *)regs, regs->a0, regs->a1, regs->a2, regs->a3);
			break;
		default:
			sbi_printf("[Penglai@Monitor] enclave interface(funcid:%ld) not supported yet\n", funcid);
			ret = SBI_ENOTSUPP;
	}
	*out_val = ret;
	return ret;
}

struct sbi_ecall_extension ecall_penglai_enclave = {
	.extid_start = SBI_EXT_PENGLAI_ENCLAVE,
	.extid_end = SBI_EXT_PENGLAI_ENCLAVE,
	.handle = sbi_ecall_penglai_enclave_handler,
};
