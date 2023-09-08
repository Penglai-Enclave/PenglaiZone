/*
 * Authors:
 *   Dong Du <Dd_nirvana@sjtu.edu.cn>
 *   Erhu Feng <2748250768@qq.com>
 *   Qingyu Shang <2931013282@qq.com>
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

struct sbi_ecall_extension ecall_smm_host;

static int sbi_ecall_smm_host_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_smm_host);
}

struct sbi_ecall_extension ecall_smm_host = {
	.extid_start = SBI_EXT_COVE,
	.extid_end = SBI_EXT_COVE,
	.register_extensions	= sbi_ecall_smm_host_register_extensions,
	.handle = sbi_ecall_smm_host_handler,
};
