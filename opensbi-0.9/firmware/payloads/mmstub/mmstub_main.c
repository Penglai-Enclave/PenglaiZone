/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 * Copyright (c) 2023, IPADS Lab. All rights reserved.
 */

#include <sbi/sbi_ecall_interface.h>

#define TRUE			1
#define FALSE			0
#define NULL			((void *)0)

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long		uint64_t;
typedef unsigned long       uintptr_t;
typedef long                int64_t;

#define SBI_ECALL(__ext, __fid, __a0, __a1, __a2)                                    \
	({                                                                    \
		register unsigned long a0 asm("a0") = (unsigned long)(__a0);  \
		register unsigned long a1 asm("a1") = (unsigned long)(__a1);  \
		register unsigned long a2 asm("a2") = (unsigned long)(__a2);  \
        register unsigned long a6 asm("a6") = (unsigned long)(__fid); \
		register unsigned long a7 asm("a7") = (unsigned long)(__ext); \
		asm volatile("ecall"                                          \
			     : "+r"(a0)                                       \
			     : "r"(a1), "r"(a2), "r" (a6), "r"(a7)            \
			     : "memory");                                     \
		a0;                                                           \
	})

#define SBI_ECALL_0(__ext, __fid) SBI_ECALL(__ext, __fid, 0, 0, 0)
#define SBI_ECALL_1(__ext, __fid, __a0) SBI_ECALL(__ext, __fid, __a0, 0, 0)
#define SBI_ECALL_2(__ext, __fid, __a0, __a1) SBI_ECALL(__ext, __fid, __a0, __a1, 0)

#define sbi_ecall_console_putc(c) SBI_ECALL_1(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, (c))

static inline void sbi_ecall_console_puts(const char *str)
{
	while (str && *str)
		sbi_ecall_console_putc(*str++);
}

#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)

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
	sbi_ecall_console_puts(
		"[mmstub] debug line: before CpuDriverEntryPoint\n");

	ret = CpuDriverEntryPoint(a0, a1, a2);

	sbi_ecall_console_puts(
		"[mmstub] debug line: after CpuDriverEntryPoint\n");

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

void mmstub_main(unsigned long a0, unsigned long a1)
{
	sbi_ecall_console_puts("\n[mmstub] running\n");

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

	sbi_ecall_console_puts("[mmstub] debug line: before _SMM_ModuleInit\n");

	_SMM_ModuleInit(SharedBufAddress, SharedBufSize, SharedCpuEntry, cookie);

	sbi_ecall_console_puts(
		"[mmstub] ### Secure Monitor StandaloneMm Init ###\n");

	sm_smm_init(DriverEntryPoint);

	while (TRUE) {
        sbi_ecall_console_puts("[mmstub] debug line: before sbi_ecall_wait_req\n");
        sbi_ecall_wait_req();

        EFI_COMMUNICATE_REG *comm_reg = (EFI_COMMUNICATE_REG *)0x80220000;
		uintptr_t ret = sm_smm_communicate(
			comm_reg->FuncId, comm_reg->Regs[0], comm_reg->Regs[1]);

		comm_reg->Return = ret;

        sbi_ecall_finish_req();
        sbi_ecall_console_puts("[mmstub] debug line: after sbi_ecall_finish_req\n");
	}
}
