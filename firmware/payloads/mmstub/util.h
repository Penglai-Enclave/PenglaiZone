#include <sbi/sbi_ecall_interface.h>

#define TRUE			1
#define FALSE			0
#define true			1
#define false			0
#define NULL			((void *)0)

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long		uint64_t;
typedef unsigned long       uintptr_t;
typedef long                int64_t;
typedef unsigned short int  bool;
typedef unsigned long       size_t;

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

extern void sbi_ecall_console_puts(const char *str);
extern void printk(const char *str, ...);

#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)
