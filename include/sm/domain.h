#ifndef _DOMAIN_H
#define _DOMAIN_H

#include <sbi/riscv_asm.h>
#include <sbi/sbi_domain.h>
#include <sm/sm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_atomic.h>
#include <sm/thread.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
	D_INVALID = 0,
	D_FRESH	= 1,
	D_RUNNABLE,
	D_RUNNING,
} domain_state_t;

struct domain_t {
    // domain_id: -1 represents sys_manager domain,
    // other represents domain in domain_table which indexed by domain_id.
	unsigned int domain_id;
	domain_state_t state;

	struct sbi_domain *sbi_domain;

	// entry point of domain
	unsigned long entry_point;

	unsigned long domain_ptbr;

	// current running harts in domain, for multi-hart management(ipi and device irq)
	struct sbi_hartmask running_harts;

	// hart to previous domains(caller), for tracking call stack and detect loop.
    int prev_domains[MAX_HARTS];

	// domain thread context
	struct thread_state_t thread_context[MAX_HARTS];
};

int domain_info_init(struct sbi_scratch *scratch);
int pmp_info_init(struct sbi_scratch *scratch);
uintptr_t finish_init_domain(uintptr_t *regs);
uintptr_t run_domain(uintptr_t *regs, unsigned int domain_id);
uintptr_t exit_domain(uintptr_t *regs);

#endif /* _DOMAIN_H */
