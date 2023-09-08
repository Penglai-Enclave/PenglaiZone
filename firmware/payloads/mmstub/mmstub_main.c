/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 * Copyright (c) 2023, IPADS Lab. All rights reserved.
 */

#include "util.h"

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef struct {
	UINT8 Type;	/* type of the structure */
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

void (*_SMM_ModuleInit)(UINT64 CpuId, void *BootInfoAddress);

typedef struct {
	EFI_SECURE_PARTITION_BOOT_INFO MmPayloadBootInfo;
	EFI_SECURE_PARTITION_CPU_INFO MmCpuInfo[1];
} EFI_SECURE_SHARED_BUFFER;

EFI_SECURE_SHARED_BUFFER MmSharedBuffer;

void mmstub_main(unsigned long a0, unsigned long a1)
{
	printk("\n[mmstub] running\n");

	_SMM_ModuleInit =
		(void (*)(UINT64 CpuId, void *BootInfoAddress))0x80C00000;

	MmSharedBuffer.MmPayloadBootInfo.Header.Version = 0x01;
	MmSharedBuffer.MmPayloadBootInfo.SpMemBase	= 0x80C00000;
	MmSharedBuffer.MmPayloadBootInfo.SpMemLimit	= 0x82000000;
	MmSharedBuffer.MmPayloadBootInfo.SpImageBase = 0x80C00000; // SpMemBase
	MmSharedBuffer.MmPayloadBootInfo.SpStackBase =
		0x81FFFFFF; // SpHeapBase + SpHeapSize + SpStackSize
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

	printk("[mmstub] debug line: before _SMM_ModuleInit\n");

	_SMM_ModuleInit(0, &MmSharedBuffer.MmPayloadBootInfo);
}
