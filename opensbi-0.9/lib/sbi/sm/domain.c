#include <sm/print.h>
#include <sm/domain.h>
#include <sm/sm.h>
#include <sm/math.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_domain.h>
#include <sbi/riscv_locks.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_trap.h>
#include <sm/platform/pmp/platform.h>
#include <sm/utils.h>
#include <sbi/sbi_timer.h>
#include <sm/attest.h>
#include <sm/gm/SM3.h>

extern struct sbi_domain *domidx_to_domain_table[SBI_DOMAIN_MAX_INDEX];
extern u32 domain_count;

// sys_manager is special, no other domain can run it initiative.
// So there is no need to initialize it, when it goes start, its state became D_RUNNING.
// sys_manager's context is always stored in other domain's thread context,
// so thread_context in domain struct of sys_manager doesn't store anything.
struct domain_t sys_manager_domain;

// TODO: sort domain table increasingly by pre_start_prio
struct domain_t domain_table[SBI_DOMAIN_MAX_INDEX] = { 0 };

int hartid_to_curr_domainid[MAX_HARTS] = { 0 };

void domain_info_init()
{
	struct sbi_domain *dom;
	int found_sys_manager = 0;
	int found_pre_start   = 0;

	for (int i = 0; i < domain_count; ++i) {
		dom = domidx_to_domain_table[i];

		// sort domain table increasingly by pre_start_prio
		// TODO: exclude ROOT/u-mode domains
		if (dom->system_manager) {
			found_sys_manager++;
			sys_manager_domain.sbi_domain = dom;
			sys_manager_domain.state      = D_FRESH;
		} else if (dom->pre_start_prio == 1) {
			found_pre_start++;
			domain_table[0].sbi_domain = dom;
			domain_table[0].state	   = D_FRESH;
		} else {
			domain_table[i + 1].sbi_domain = dom;
		}
	}

	if (!found_pre_start) {
		sbi_printf("SBI Error: error config for pre_start domain.\n");
		sbi_hart_hang();
	}

	if (!found_sys_manager || found_sys_manager > 1) {
		sbi_printf("SBI Error: error config for pre_start domain.\n");
		sbi_hart_hang();
	}

	// TODO: measure the first pre-start domain

	// At start, all hart will start domain0
	hartid_to_curr_domainid[current_hartid()] = 0;
}

int swap_between_domains(uintptr_t *host_regs, struct domain_t *dom)
{
    int curr_hartid = current_hartid();
	//save host context
	swap_prev_state(&(dom->thread_context[curr_hartid]), host_regs);

	// clear pending interrupts
	csr_read_clear(CSR_MIP, MIP_MTIP);
	csr_read_clear(CSR_MIP, MIP_STIP);
	csr_read_clear(CSR_MIP, MIP_SSIP);
	csr_read_clear(CSR_MIP, MIP_SEIP);

	// swap the mepc to transfer control to the enclave
	// This will be overwriten by the entry-address in the case of run_enclave
	//swap_prev_mepc(&(enclave->thread_context), csr_read(CSR_MEPC));
	swap_prev_mepc(&(dom->thread_context[curr_hartid]), host_regs[32]);
	host_regs[32] = csr_read(CSR_MEPC); //update the new value to host_regs

	//set mstatus to transfer control to u mode
	uintptr_t mstatus =
		host_regs[33]; //In OpenSBI, we use regs to change mstatus
	mstatus	      = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_S);
	host_regs[33] = mstatus;

	__asm__ __volatile__("sfence.vma" : : : "memory");

	return 0;
}

// Question: two situation, both start from scratch
//      In chained boot stage, should run_domain record thread_context? Yes, store in curr_domain structure.
//      When starting a new sec-linux(normal domain), should? Yes, store in sec-linux domain structure.
// So we use init_domain and run_domain to handle this difference.
uintptr_t init_domain(uintptr_t *regs, struct domain_t *curr_domain,
		      struct domain_t *target_domain)
{
	if (target_domain->state != D_FRESH) {
		sbi_printf(
			"SBI Error: init un-fresh domain in chained boot stage\n");
		sbi_hart_hang();
	}

	// Check if curr_domain has not initialized yet. At this time,
	// initializing other domain will result in the inability to return
	// to the curr_domain in the future.
	if (curr_domain->state == D_FRESH) {
		sbi_printf("SBI Error: can't support nested fresh domain\n");
		sbi_hart_hang();
	}
	swap_between_domains(regs, curr_domain);

	//In OpenSBI, we use regs to change mepc
	regs[32] = (uintptr_t)target_domain->sbi_domain->next_addr;

	//pass parameters
	regs[10] = current_hartid();
	regs[11] = (uintptr_t)target_domain->sbi_domain->next_arg1;
	return 0;
}

// TODO: support modify entry point
// In chained boot flow, we don't maintain call stack between domains.
uintptr_t finish_init_domain(uintptr_t *regs)
{
	struct domain_t *curr_domain;
	int curr_domainid = hartid_to_curr_domainid[current_hartid()];
	if (curr_domainid == -1) {
		sbi_printf(
			"SBI Error: sys_manager domain shouldn't call finish_init()\n");
		sbi_hart_hang();
	}
	curr_domain	   = &domain_table[curr_domainid];
	curr_domain->state = D_RUNNABLE;

	// Chained boot flow
	int next_domainid = curr_domainid + 1;
	int curr_domain_prio =
		domain_table[curr_domainid].sbi_domain->pre_start_prio;
	if (next_domainid < SBI_DOMAIN_MAX_INDEX &&
	    domain_table[next_domainid].sbi_domain->pre_start_prio >
		    curr_domain_prio &&
	    domain_table[next_domainid].sbi_domain->pre_start_prio <
		    INT32_MAX) {

		// TODO: measure next pre-start domain

		init_domain(regs, curr_domain, &domain_table[next_domainid]);
		hartid_to_curr_domainid[current_hartid()] = next_domainid;
		return 0;
	}

	// No other fresh pre-start domain
	init_domain(regs, curr_domain, &sys_manager_domain);
	sys_manager_domain.state		  = D_RUNNING;
	hartid_to_curr_domainid[current_hartid()] = -1;

	return 0;
}

// TODO: find domain by name, using name for universe interface
uintptr_t run_domain(uintptr_t *regs, unsigned int domain_id)
{
	if (domain_id == -1) {
		sbi_printf(
			"SBI Error: sys_manager domain shouldn't be called initiative()\n");
		sbi_hart_hang();
	}
	struct domain_t *target_domain = &domain_table[domain_id];

	// TODO: maintain hartmask 'running_harts'
	if (target_domain->state == D_FRESH) {
		swap_between_domains(regs, target_domain);

		//In OpenSBI, we use regs to change mepc
		regs[32] = (uintptr_t)target_domain->sbi_domain->next_addr;

		//pass parameters
		regs[10] = current_hartid();
		regs[11] = (uintptr_t)target_domain->sbi_domain->next_arg1;
	} else {
		// TODO: check for no call loop between domains
		// use domain->prev_domains
		swap_between_domains(regs, target_domain);
	}

	target_domain->prev_domains[current_hartid()] =
		hartid_to_curr_domainid[current_hartid()];
	hartid_to_curr_domainid[current_hartid()] = domain_id;
	return 0;
}

uintptr_t exit_domain(uintptr_t *regs)
{
	struct domain_t *curr_domain;
	int curr_domainid = hartid_to_curr_domainid[current_hartid()];
	if (curr_domainid == -1) {
		sbi_printf(
			"SBI Error: sys_manager domain shouldn't call exit_domain()\n");
		sbi_hart_hang();
	}
	curr_domain = &domain_table[curr_domainid];

	swap_between_domains(regs, curr_domain);

	hartid_to_curr_domainid[current_hartid()] =
		curr_domain->prev_domains[current_hartid()];
	// curr_domain is not running on this hart, so this data doesn't matter
	curr_domain->prev_domains[current_hartid()] = -1;
	return 0;
}
