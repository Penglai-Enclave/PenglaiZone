/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 * Copyright (c) 2023, IPADS Lab. All rights reserved.
 */

#include "util.h"

#define SBI_EXT_MMSTUB  0x434F5649
/* SBI function IDs for MMSTUB extension */
#define SBI_COVE_SMM_WAIT_REQ		0x82
#define SBI_COVE_SMM_FINISH_REQ		0x83

static inline void sbi_ecall_wait_req()
{
	SBI_ECALL_0(SBI_EXT_MMSTUB, SBI_COVE_SMM_WAIT_REQ);
}

static inline void sbi_ecall_finish_req()
{
	SBI_ECALL_0(SBI_EXT_MMSTUB, SBI_COVE_SMM_FINISH_REQ);
}

typedef
uint64_t
(*PI_MM_RISCV_CPU_DRIVER_ENTRYPOINT) (
    uint64_t  EventId,
    uint64_t  CpuNumber,
    uint64_t  NsCommBufferAddr
    );

static PI_MM_RISCV_CPU_DRIVER_ENTRYPOINT CpuDriverEntryPoint = NULL;
void sm_smm_init(void *DriverEntryPoint)
{
	CpuDriverEntryPoint =
		(PI_MM_RISCV_CPU_DRIVER_ENTRYPOINT)DriverEntryPoint;
}

uintptr_t sm_smm_communicate(uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
	uintptr_t ret = 0;
	printk("[mmstub] debug line: before CpuDriverEntryPoint, a0: %lx, a1: %lx, a2: %lx\n",
	       a0, a1, a2);

	ret = CpuDriverEntryPoint(a0, a1, a2);

	printk("[mmstub] debug line: after CpuDriverEntryPoint\n");

	return ret;
}

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef struct {
	UINT8 Type;    /* type of the structure */
	UINT8 Version; /* version of this structure */
	UINT16 Size;   /* size of this structure in bytes */
	UINT32 Attr;   /* attributes: unused bits SBZ */
} EFI_PARAM_HEADER;

typedef struct {
	UINT64 Mpidr;
	UINT32 LinearId;
	UINT32 Flags;
} EFI_SECURE_PARTITION_CPU_INFO;

typedef struct {
	EFI_PARAM_HEADER Header;
	UINT64 SpMemBase;
	UINT64 SpMemLimit;
	UINT64 SpImageBase;
	UINT64 SpStackBase;
	UINT64 SpHeapBase;
	UINT64 SpNsCommBufBase;
	UINT64 SpSharedBufBase;
	UINT64 SpImageSize;
	UINT64 SpPcpuStackSize;
	UINT64 SpHeapSize;
	UINT64 SpNsCommBufSize;
	UINT64 SpSharedBufSize;
	UINT32 NumSpMemRegions;
	UINT32 NumCpus;
	EFI_SECURE_PARTITION_CPU_INFO *CpuInfo;
} EFI_SECURE_PARTITION_BOOT_INFO;

void (*_SMM_ModuleInit)(void *SharedBufAddress, int64_t SharedBufSize,
			int64_t SharedCpuEntry, int64_t cookie);
typedef struct {
	EFI_SECURE_PARTITION_BOOT_INFO MmPayloadBootInfo;
	EFI_SECURE_PARTITION_CPU_INFO MmCpuInfo[1];
} EFI_SECURE_SHARED_BUFFER;

EFI_SECURE_SHARED_BUFFER MmSharedBuffer;

typedef struct {
	UINT64 FuncId;
	UINT64 Regs[2];
    UINT64 Return;
} EFI_COMMUNICATE_REG;

// static int num = 0;

void mmstub_main(unsigned long a0, unsigned long a1)
{
	printk("\n[mmstub] running\n");

	_SMM_ModuleInit =
		(void (*)(void *SharedBufAddress, int64_t SharedBufSize,
			  int64_t SharedCpuEntry, int64_t cookie))0x80C00000;

	MmSharedBuffer.MmPayloadBootInfo.Header.Version = 0x01;
	MmSharedBuffer.MmPayloadBootInfo.SpMemBase	= 0x80C00000;
	MmSharedBuffer.MmPayloadBootInfo.SpMemLimit	= 0x82000000;
	MmSharedBuffer.MmPayloadBootInfo.SpImageBase = 0x80C00000; // SpMemBase
	MmSharedBuffer.MmPayloadBootInfo.SpStackBase =
		0x81700000; // SpHeapBase + SpHeapSize
	MmSharedBuffer.MmPayloadBootInfo.SpHeapBase =
		0x80F00000; // SpMemBase + SpImageSize
	MmSharedBuffer.MmPayloadBootInfo.SpNsCommBufBase = 0xFFE00000;
	MmSharedBuffer.MmPayloadBootInfo.SpSharedBufBase = 0x81F80000;
	MmSharedBuffer.MmPayloadBootInfo.SpImageSize	 = 0x300000;
	MmSharedBuffer.MmPayloadBootInfo.SpPcpuStackSize = 0x10000;
	MmSharedBuffer.MmPayloadBootInfo.SpHeapSize	 = 0x800000;
	MmSharedBuffer.MmPayloadBootInfo.SpNsCommBufSize = 0x200000;
	MmSharedBuffer.MmPayloadBootInfo.SpSharedBufSize = 0x80000;
	MmSharedBuffer.MmPayloadBootInfo.NumSpMemRegions = 0x6;
	MmSharedBuffer.MmPayloadBootInfo.NumCpus	 = 1;
	MmSharedBuffer.MmCpuInfo[0].LinearId		 = 0;
	MmSharedBuffer.MmCpuInfo[0].Flags		 = 0;
	MmSharedBuffer.MmPayloadBootInfo.CpuInfo = MmSharedBuffer.MmCpuInfo;
	void *SharedBufAddress			 = &MmSharedBuffer;
	int64_t SharedBufSize			 = sizeof(MmSharedBuffer);
	void *DriverEntryPoint			 = NULL;
	int64_t SharedCpuEntry			 = (int64_t)&DriverEntryPoint;
	int64_t cookie				 = 0;

	printk("[mmstub] debug line: before _SMM_ModuleInit\n");

	_SMM_ModuleInit(SharedBufAddress, SharedBufSize, SharedCpuEntry, cookie);

	printk("[mmstub] ### Secure Monitor StandaloneMm Init ###\n");

	sm_smm_init(DriverEntryPoint);

	while (TRUE) {
        sbi_ecall_console_puts("[mmstub] debug line: before sbi_ecall_wait_req\n");
        sbi_ecall_wait_req();

        // EFI_COMMUNICATE_REG *comm_regs = (EFI_COMMUNICATE_REG *)0x80300000;
        // // printk("    **** [%s time%d] &comm_regs->FuncId: %p, comm_regs->FuncId: %lx\r\n",__func__, num, &comm_regs->FuncId, comm_regs->FuncId);
        // // printk("    **** [%s time%d] &comm_regs->Regs[0]: %p, comm_regs->Regs[0]: %lx\r\n",__func__, num, &comm_regs->Regs[0], comm_regs->Regs[0]);
        // printk("    **** [%s time%d] &comm_regs->Regs[1]: %p, comm_regs->Regs[1]: %lx\r\n",__func__, num++, &comm_regs->Regs[1], comm_regs->Regs[1]);

		// uintptr_t ret = sm_smm_communicate(
		// 	comm_regs->FuncId, comm_regs->Regs[0], comm_regs->Regs[1]);

		// comm_regs->Return = ret;

        // sbi_ecall_finish_req();
        sbi_ecall_console_puts("[mmstub] debug line: after sbi_ecall_finish_req\n");
	}
}
