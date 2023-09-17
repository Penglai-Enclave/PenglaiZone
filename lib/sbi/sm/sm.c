#include <sbi/riscv_atomic.h>
#include <sm/sm.h>
#include <sm/domain.h>

#include <sbi/sbi_console.h>

int sm_domain_init(struct sbi_scratch *scratch)
{
  pmp_info_init(scratch);
  return domain_info_init(scratch);
}

#define MMSTUB_SHARE_MEM       (void*)((unsigned long)0x80300000)

typedef struct {
	unsigned long  FuncId;
	unsigned long  Regs[2];
    unsigned long  Return;
} EFI_COMMUNICATE_REG;

EFI_COMMUNICATE_REG *CommRegsPointer[MAX_HARTS] = {0};

uintptr_t sm_smm_communicate(uintptr_t *regs, uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret = 0;
//   sbi_printf("[Penglai Monitor] %s invoked\r\n",__func__);
//   sbi_printf("    **** [%s time%d] a0: %lx, a1: %lx, a2: %lx\r\n",__func__, num, a0, a1, a2);

  EFI_COMMUNICATE_REG *comm_regs = CommRegsPointer[current_hartid()];
  comm_regs->FuncId = a0;
//   sbi_printf("    **** [%s time%d] &comm_regs->FuncId: %p, comm_regs->FuncId: %lx\r\n",__func__, num, &comm_regs->FuncId, comm_regs->FuncId);
  comm_regs->Regs[0] = a1;
//   sbi_printf("    **** [%s time%d] &comm_regs->Regs[0]: %p, comm_regs->Regs[0]: %lx\r\n",__func__, num, &comm_regs->Regs[0], comm_regs->Regs[0]);
  comm_regs->Regs[1] = a2;
//   sbi_printf("    **** [%s time%d] &comm_regs->Regs[1]: %p, comm_regs->Regs[1]: %lx\r\n",__func__, num, &comm_regs->Regs[1], comm_regs->Regs[1]);

  ret = run_domain(regs, 0);

//   sbi_printf("[Penglai Monitor] %s return: %ld\r\n",__func__, ret);

  return ret;
}

uintptr_t sm_smm_version(uintptr_t *regs, uintptr_t a1)
{
  uintptr_t ret = 0;
  uintptr_t *mm_version = (uintptr_t *)a1;
  sbi_printf("[Penglai Monitor] %s invoked\r\n",__func__);
  *mm_version = MM_VERSION_COMPILED;

  sbi_printf("[Penglai Monitor] %s return: %ld\r\n",__func__, ret);
  return ret;
}

uintptr_t sm_smm_exit(uintptr_t *regs, uintptr_t CommRegPointer)
{
  uintptr_t ret = 0;
  static int init_stat[MAX_HARTS] = {0};
//   sbi_printf("[Penglai Monitor] %s invoked\r\n",__func__);
  unsigned int this_hart = current_hartid();

  if(init_stat[this_hart] == 0){
    CommRegsPointer[this_hart] = (EFI_COMMUNICATE_REG *)CommRegPointer;
    ret = finish_init_domain(regs);
    init_stat[this_hart] = 1;
  } else {
    ret = exit_domain(regs);
  }

//   sbi_printf("[Penglai Monitor] %s return: %ld\r\n",__func__, ret);
  return ret;
}
