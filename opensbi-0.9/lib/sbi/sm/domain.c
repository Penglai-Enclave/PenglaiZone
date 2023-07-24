#include <sm/print.h>
#include <sm/domain.h>
#include <sm/sm.h>
#include <sm/pmp.h>
#include <sm/math.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_domain.h>
#include <sbi/riscv_locks.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/riscv_asm.h>
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

// All other domains are stored in the domain_table, sorted increasingly by pre_start_prio.
struct domain_t domain_table[SBI_DOMAIN_MAX_INDEX] = { 0 };

int hartid_to_curr_domainid[MAX_HARTS] = { 0 };

// Check if the domain configuration meets the requirements of PenglaiZone,
// construct domain structures used after.
int domain_info_init(struct sbi_scratch *scratch)
{
	struct sbi_domain *dom;
	struct domain_t *first_domain;
	int found_sys_manager = 0;
	int count	      = 0;
	int i, j;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	// PenglaiZone requires:
	//      All domains run in S-mode, and there is exactly one sys_manager domain,
	// and all harts are assigned to the highest priority pre_start domain (if any) or sys_manager.
	// "possible-harts" and "boot-hart" properties in the device tree are ignored.
	for (i = 0; i < domain_count; ++i) {
		dom = domidx_to_domain_table[i];

		if (dom->next_mode == PRV_U) {
			sbi_printf(
				"SBI Error: DT configuration should has no U-mode domain.\n");
			return -1;
		}

		if (sbi_strcmp(dom->name, "root") == 0)
			continue;

		if (dom->system_manager) {
			found_sys_manager++;
			sys_manager_domain.sbi_domain = dom;
			sys_manager_domain.state      = D_FRESH;
		} else {
			domain_table[count++].sbi_domain = dom;
		}
	}

	if (!found_sys_manager || found_sys_manager > 1) {
		sbi_printf("SBI Error: error config for sys_manager domain.\n");
		return -1;
	}

	// sort domain table increasingly by pre_start_prio
	for (i = 0; i < count - 1; i++) {
		for (j = 0; j < count - i - 1; j++) {
			if (domain_table[j].sbi_domain->pre_start_prio >
			    domain_table[j + 1].sbi_domain->pre_start_prio) {
				dom = domain_table[j].sbi_domain;
				domain_table[j].sbi_domain =
					domain_table[j + 1].sbi_domain;
				domain_table[j + 1].sbi_domain = dom;
			}
		}
	}
	for (i = 0; i < count; i++) {
		// By default, we think that the pre-start domain is preloaded
		if (domain_table[i].sbi_domain->pre_start_prio != INT32_MAX)
			domain_table[i].state = D_FRESH;
		domain_table[i].domain_id = i;
	}
	sys_manager_domain.domain_id = -1;

	first_domain = &sys_manager_domain;
	dom	     = sys_manager_domain.sbi_domain;
	if (domain_table[0].sbi_domain->pre_start_prio != INT32_MAX) {
		first_domain = &domain_table[0];
		dom	     = domain_table[0].sbi_domain;
	}
	// Check that all harts are started in the first pre-start domain (if any).
	for (i = 0; i < MAX_HARTS; i++) {
		if (sbi_platform_hart_invalid(plat, i))
			continue;
		if (!sbi_hartmask_test_hart(i, &dom->assigned_harts)) {
			sbi_printf(
				"SBI Error: error config for domain hart assignment.\n");
			return -1;
		}
	}

	// Maintain current domain_id. At start, all harts in domain0 or sys_manager domain.
	for (i = 0; i < MAX_HARTS; i++) {
		hartid_to_curr_domainid[i] =
			(dom == sys_manager_domain.sbi_domain) ? -1 : 0;
	}

	// Maintain domains' call stack. At start, all domains call stack is null.
	for (i = 0; i < MAX_HARTS; i++) {
		sys_manager_domain.prev_domains[i] = -1;
	}
	for (i = 0; i < count; i++) {
		for (j = 0; j < MAX_HARTS; ++j) {
			domain_table[i].prev_domains[j] = -1;
		}
	}

	// Measure the first pre_start domain
    sbi_printf("SBI Info: %s, %d\n", __func__, __LINE__);
	if (first_domain != &sys_manager_domain){
        sbi_printf("SBI Info: %s, %d, size: %ld\n", __func__, __LINE__, first_domain->sbi_domain->measure_size);
        hash_domain(first_domain);
    }

	return 0;
}

// retrieve access to current domain memregion, grant access to target domain
int domain_pmp_configure(struct domain_t *curr_domain,
			 struct domain_t *target_domain)
{
	struct sbi_domain_memregion *reg;
	struct sbi_domain *dom;
	unsigned int curr_pmp_idx = 0, target_pmp_idx = 0, pmp_flags, pmp_bits;
	unsigned long pmp_addr, pmp_addr_max	      = 0;

	pmp_bits     = PMP_ADDR_BITS - 1;
	pmp_addr_max = (1UL << pmp_bits) | ((1UL << pmp_bits) - 1);

	dom = target_domain->sbi_domain;
	sbi_domain_for_each_memregion(dom, reg)
	{
		if (NPMP <= target_pmp_idx)
			break;

		pmp_flags = 0;
		if (reg->flags & SBI_DOMAIN_MEMREGION_READABLE)
			pmp_flags |= PMP_R;
		if (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE)
			pmp_flags |= PMP_W;
		if (reg->flags & SBI_DOMAIN_MEMREGION_EXECUTABLE)
			pmp_flags |= PMP_X;
		if (reg->flags & SBI_DOMAIN_MEMREGION_MMODE)
			pmp_flags |= PMP_L;

		pmp_addr = reg->base >> PMP_SHIFT;
		if (PMP_GRAN_LOG2 <= reg->order && pmp_addr < pmp_addr_max)
			pmp_set(target_pmp_idx++, pmp_flags, reg->base,
				reg->order);
		else {
			sbi_printf("Can not configure pmp for domain %s",
				   dom->name);
			sbi_printf(
				"because memory region address %lx or size %lx is not in range\n",
				reg->base, reg->order);
		}
	}

	dom = curr_domain->sbi_domain;
	sbi_domain_for_each_memregion(dom, reg)
	{
		if (NPMP <= curr_pmp_idx)
			break;
		if (curr_pmp_idx >= target_pmp_idx)
			clear_pmp(curr_pmp_idx);
		curr_pmp_idx++;
	}

	return 0;
}

int swap_between_domains(uintptr_t *host_regs, struct domain_t *dom)
{
	unsigned int this_hart = current_hartid();
	//save host context
	swap_prev_state(&(dom->thread_context[this_hart]), host_regs);

	// clear pending interrupts
	csr_read_clear(CSR_MIP, MIP_MTIP);
	csr_read_clear(CSR_MIP, MIP_STIP);
	csr_read_clear(CSR_MIP, MIP_SSIP);
	csr_read_clear(CSR_MIP, MIP_SEIP);

	// swap the mepc to transfer control to the enclave
	// This will be overwriten by the entry-address in the case of run_enclave
	//swap_prev_mepc(&(enclave->thread_context), csr_read(CSR_MEPC));
	swap_prev_mepc(&(dom->thread_context[this_hart]), host_regs[32]);
	host_regs[32] = csr_read(CSR_MEPC); //update the new value to host_regs

	// swap to target ptbr
	uintptr_t ptbr = dom->domain_ptbr;
	dom->domain_ptbr = csr_read(CSR_SATP);
	csr_write(CSR_SATP, ptbr);

	//set mstatus to transfer control to S-mode
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
		// any error in boot stage will result in halt
		sbi_hart_hang();
	}

	// Check if curr_domain has not initialized yet. At this time,
	// initializing other domain will result in the inability to return
	// to the curr_domain in the future.
	if (curr_domain->state == D_FRESH) {
		sbi_printf("SBI Error: can't support nested fresh domain\n");
		sbi_hart_hang();
	}
	domain_pmp_configure(curr_domain, target_domain);
	swap_between_domains(regs, curr_domain);

	//set initial ptbr
	csr_write(CSR_SATP, 0UL);

	//In OpenSBI, we use regs to change mepc
	regs[32] = (uintptr_t)target_domain->sbi_domain->next_addr;

	//pass parameters
	regs[10] = current_hartid();
	regs[11] = (uintptr_t)target_domain->sbi_domain->next_arg1;
	return 0;
}

// TODO: support modify entry point
// In chained boot flow, we don't maintain call stack between domains and running_harts of each domain.
uintptr_t finish_init_domain(uintptr_t *regs)
{
	struct domain_t *curr_domain, *next_domain;
	unsigned int this_hart = current_hartid();
	int curr_domainid      = hartid_to_curr_domainid[this_hart];
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
	    domain_table[next_domainid].sbi_domain != NULL &&
	    domain_table[next_domainid].sbi_domain->pre_start_prio >
		    curr_domain_prio &&
	    domain_table[next_domainid].sbi_domain->pre_start_prio <
		    INT32_MAX) {

		// Measure next pre_start domain
		next_domain = &domain_table[next_domainid];
		if (next_domain->sbi_domain->measure_size != 0)
			hash_domain(next_domain);

		init_domain(regs, curr_domain, next_domain);
		hartid_to_curr_domainid[this_hart] = next_domainid;
		return 0;
	}

	// No other fresh pre_start domain, boot stage finished.
	init_domain(regs, curr_domain, &sys_manager_domain);
	sys_manager_domain.state = D_RUNNING;
	sbi_hartmask_set_hart(this_hart, &sys_manager_domain.running_harts);
	hartid_to_curr_domainid[this_hart] = -1;

	return 0;
}

// Note(Qingyu): maybe we will need a wrapper function using domain name as parameter
// so we can find domain by name[fixed on each pre_start domain like "smm"], for a universal interface.
uintptr_t run_domain(uintptr_t *regs, unsigned int domain_id)
{
	struct domain_t *curr_domain, *dom;
	unsigned int this_hart = current_hartid();
	int prev_domainid;
	int curr_domainid = hartid_to_curr_domainid[this_hart];
	if (curr_domainid == -1) {
		curr_domain = &sys_manager_domain;
	} else {
		curr_domain = &domain_table[curr_domainid];
	}

	if (domain_id == -1) {
		sbi_printf(
			"SBI Error: sys_manager domain shouldn't be called initiative()\n");
		return -1;
	}
	struct domain_t *target_domain = &domain_table[domain_id];

	if (target_domain->state == D_INVALID) {
		sbi_printf("SBI Error: target domain hasn't be initialized\n");
		return -1;
	}

	if (target_domain->state == D_FRESH) {
		domain_pmp_configure(curr_domain, target_domain);
		swap_between_domains(regs, target_domain);

		//In OpenSBI, we use regs to change mepc
		regs[32] = (uintptr_t)target_domain->sbi_domain->next_addr;

		//pass parameters
		regs[10] = this_hart;
		regs[11] = (uintptr_t)target_domain->sbi_domain->next_arg1;
	} else {
		// check for no call loop between domains using domain->prev_domains
		dom = curr_domain;
		while (TRUE) {
			if (dom->domain_id == domain_id) {
				sbi_printf(
					"SBI Error: domain call stack nested\n");
				return -1;
			}
			prev_domainid = dom->prev_domains[this_hart];
			if (prev_domainid != -1)
				dom = &domain_table[prev_domainid];
			else
				break;
		}
		domain_pmp_configure(curr_domain, target_domain);
		swap_between_domains(regs, target_domain);
	}

	// maintain hartmask 'running_harts'
	sbi_hartmask_clear_hart(this_hart, &curr_domain->running_harts);
	sbi_hartmask_set_hart(this_hart, &target_domain->running_harts);

	target_domain->prev_domains[this_hart] =
		hartid_to_curr_domainid[this_hart];
	hartid_to_curr_domainid[this_hart] = domain_id;
	return 0;
}

uintptr_t exit_domain(uintptr_t *regs)
{
	struct domain_t *curr_domain, *prev_domain;
	unsigned int this_hart = current_hartid();
	int prev_domainid;
	int curr_domainid = hartid_to_curr_domainid[this_hart];
	if (curr_domainid == -1) {
		sbi_printf(
			"SBI Error: sys_manager domain shouldn't call exit_domain()\n");
		return -1;
	}
	curr_domain = &domain_table[curr_domainid];

	prev_domainid = curr_domain->prev_domains[this_hart];
	if (prev_domainid == -1) {
		prev_domain = &sys_manager_domain;
	} else {
		prev_domain = &domain_table[prev_domainid];
	}

	domain_pmp_configure(curr_domain, prev_domain);
	swap_between_domains(regs, curr_domain);

	sbi_hartmask_clear_hart(this_hart, &curr_domain->running_harts);
	sbi_hartmask_set_hart(this_hart, &prev_domain->running_harts);

	hartid_to_curr_domainid[this_hart] = prev_domainid;
	// curr_domain is not running on this hart, so this data doesn't matter,
	// we use -1 to imply call stack end.
	curr_domain->prev_domains[this_hart] = -1;
	return 0;
}
