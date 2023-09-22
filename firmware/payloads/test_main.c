/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_ecall_interface.h>

#define SBI_ECALL(__eid, __fid, __a0, __a1, __a2)                             \
	({                                                                    \
		register unsigned long a0 asm("a0") = (unsigned long)(__a0);  \
		register unsigned long a1 asm("a1") = (unsigned long)(__a1);  \
		register unsigned long a2 asm("a2") = (unsigned long)(__a2);  \
        register unsigned long a3 asm("a3") = (unsigned long)(0);  \
        register unsigned long a4 asm("a4") = (unsigned long)(0);  \
        register unsigned long a5 asm("a5") = (unsigned long)(0);  \
		register unsigned long a6 asm("a6") = (unsigned long)(__fid); \
		register unsigned long a7 asm("a7") = (unsigned long)(__eid); \
		asm volatile("ecall"                                          \
			     : "+r"(a0)                                       \
			     : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)             \
			     : "memory");                                     \
		a0;                                                           \
	})

#define SBI_ECALL_0(__eid, __fid) SBI_ECALL(__eid, __fid, 0, 0, 0)
#define SBI_ECALL_1(__eid, __fid, __a0) SBI_ECALL(__eid, __fid, __a0, 0, 0)
#define SBI_ECALL_2(__eid, __fid, __a0, __a1) SBI_ECALL(__eid, __fid, __a0, __a1, 0)

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

#define SBI_EXT_RPXY                 0x52505859

/* SBI function IDs for RPXY extension */
#define SBI_RPXY_SETUP_SHMEM        0x1
#define SBI_RPXY_SEND_NORMAL_MSG    0x2

/**
  Set the RPXY Shared Memory.

  @param    ShmemPhys   Shaed Memory Physical Address.
  @retval   EFI_SUCCESS    Success is returned from the functin in SBI.
**/
int
SbiRpxySetShmem(
  int PageSize,
  int ShmemPhys
  )
{
  SBI_ECALL_2(SBI_EXT_RPXY, SBI_RPXY_SETUP_SHMEM, PageSize, ShmemPhys);
  return 0;
}

/**
  Send the MM data through OpenSBI FW RPXY Extension SBI.

  @param    Transportd
  @param    SrvGrpId
  @param    SrvId
  @retval   EFI_SUCCESS    Success is returned from the functin in SBI.
**/
int
SbiRpxySendNormalMessage(
  int TransportId, 
  int SrvGrpId,
  int SrvId
  )
{
  SBI_ECALL(SBI_EXT_RPXY, SBI_RPXY_SEND_NORMAL_MSG, TransportId, SrvGrpId, SrvId);
  return 0;
}

#define RPMI_MM_TRANSPORT_ID  0x00
#define RPMI_MM_SRV_GROUP     0x08
#define RPMI_MM_SRV_COMPLETE  0x03

void test_main(unsigned long a0, unsigned long a1)
{
	sbi_ecall_console_puts("\nTest payload running\n");

    SbiRpxySetShmem(0x1000, 0xFFE00000);
    SbiRpxySendNormalMessage(RPMI_MM_TRANSPORT_ID, RPMI_MM_SRV_GROUP, RPMI_MM_SRV_COMPLETE);

	while (1)
		wfi();
}
