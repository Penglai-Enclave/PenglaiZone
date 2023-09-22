/*
 * Copyright (c) 2017-2022, IPADS Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_scratch.h>
    .align 3
	.global spm_secure_partition_enter

	/* ---------------------------------------------------------------------
	 * This function is called with SP_EL0 as stack. Here we stash our EL3
	 * callee-saved registers on to the stack as a part of saving the C
	 * runtime and enter the secure payload.
	 * 'x0' contains a pointer to the memory where the address of the C
	 *  runtime context is to be saved.
	 * ---------------------------------------------------------------------
	 */
spm_secure_partition_enter:
	/* Make space for the registers that we're going to save */
    REG_S   sp, 0(a1)
    add	sp, sp, -(SBI_TRAP_REGS_SIZE)

	/* Save callee-saved registers on to the stack */
	REG_L	gp, SBI_TRAP_REGS_OFFSET(gp)(a0)
	REG_L	tp, SBI_TRAP_REGS_OFFSET(tp)(a0)
    REG_S	s0, SBI_TRAP_REGS_OFFSET(s0)(sp)
	REG_S	s1, SBI_TRAP_REGS_OFFSET(s1)(sp)
	REG_S	s2, SBI_TRAP_REGS_OFFSET(s2)(sp)
	REG_S	s3, SBI_TRAP_REGS_OFFSET(s3)(sp)
	REG_S	s4, SBI_TRAP_REGS_OFFSET(s4)(sp)
	REG_S	s5, SBI_TRAP_REGS_OFFSET(s5)(sp)
	REG_S	s6, SBI_TRAP_REGS_OFFSET(s6)(sp)
	REG_S	s7, SBI_TRAP_REGS_OFFSET(s7)(sp)
	REG_S	s8, SBI_TRAP_REGS_OFFSET(s8)(sp)
	REG_S	s9, SBI_TRAP_REGS_OFFSET(s9)(sp)
	REG_S	s10, SBI_TRAP_REGS_OFFSET(s10)(sp)
	REG_S	s11, SBI_TRAP_REGS_OFFSET(s11)(sp)    

    /* Save Return address register on to the stack */
    REG_S	ra, SBI_TRAP_REGS_OFFSET(ra)(sp)

    /* Save original MSCRATCH register on to the stack (use `sp` field) */
    /* Swap TP and MSCRATCH */
	csrrw	tp, CSR_MSCRATCH, tp
    REG_S	tp, SBI_TRAP_REGS_OFFSET(sp)(sp)

    /* Set MSCRATCH register to current runtime stack */
    /* Make space for the struct sbi_scratch that will be used in next _trap_handler */
    li	a5, SBI_SCRATCH_SIZE
    sub	tp, sp, a5
    /* Swap TP and MSCRATCH */
	csrrw	tp, CSR_MSCRATCH, tp

	/* ---------------------------------------------------------------------
	 * Everything is setup now. _trap_exit() will use the secure context to
	 * restore to the general purpose and system registers to MRET into
	 * the secure payload.
	 * ---------------------------------------------------------------------
	 */
	call	_trap_exit

    .align 3
    .global spm_secure_partition_exit
	/* ---------------------------------------------------------------------
	 * This function is called with 'x0' pointing to a C runtime context
	 * saved in spm_secure_partition_enter().
	 * It restores the saved registers and jumps to that runtime with 'x0'
	 * as the new SP register. This destroys the C runtime context that had
	 * been built on the stack below the saved context by the caller. Later
	 * the second parameter 'x1' is passed as a return value to the caller.
	 * ---------------------------------------------------------------------
	 */
spm_secure_partition_exit:
	/* Restore the previous stack */
	add	sp, a0, 0

    /* Restore original MSCRATCH register from the stack (use `sp` field) */
    REG_L	tp, (SBI_TRAP_REGS_OFFSET(sp) - SBI_TRAP_REGS_SIZE)(sp)
    /* Swap TP and MSCRATCH */
	csrrw	tp, CSR_MSCRATCH, tp

	/* Restore callee-saved registers from the stack */
	REG_L	gp, (SBI_TRAP_REGS_OFFSET(gp) - SBI_TRAP_REGS_SIZE)(a0)
	REG_L	tp, (SBI_TRAP_REGS_OFFSET(tp) - SBI_TRAP_REGS_SIZE)(a0)
	REG_L	s0, (SBI_TRAP_REGS_OFFSET(s0) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s1, (SBI_TRAP_REGS_OFFSET(s1) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s2, (SBI_TRAP_REGS_OFFSET(s2) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s3, (SBI_TRAP_REGS_OFFSET(s3) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s4, (SBI_TRAP_REGS_OFFSET(s4) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s5, (SBI_TRAP_REGS_OFFSET(s5) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s6, (SBI_TRAP_REGS_OFFSET(s6) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s7, (SBI_TRAP_REGS_OFFSET(s7) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s8, (SBI_TRAP_REGS_OFFSET(s8) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s9, (SBI_TRAP_REGS_OFFSET(s9) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s10, (SBI_TRAP_REGS_OFFSET(s10) - SBI_TRAP_REGS_SIZE)(sp)
	REG_L	s11, (SBI_TRAP_REGS_OFFSET(s11) - SBI_TRAP_REGS_SIZE)(sp)

	/* Restore Return address registers from the stack */
	REG_L	ra, (SBI_TRAP_REGS_OFFSET(ra) - SBI_TRAP_REGS_SIZE)(sp)

	/* ---------------------------------------------------------------------
	 * This should take us back to the instruction after the call to the
	 * last spm_secure_partition_enter().* Place the second parameter to a0
	 * so that the caller will see it as a return value from the original
	 * entry call.
	 * ---------------------------------------------------------------------
	 */
	add	a0, a1, 0
	ret
