// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 9: Your code here.

	//pgfault() проверяет, что ошибка является записью (проверка на FEC_WR в коде ошибки) и что PTE для страницы помечен PTE_COW. Если нет, паникует.
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	int r;
	pte_t pte = uvpt[PGNUM(addr)];
    if (!(err & FEC_WR) || !(pte & PTE_COW))
    {
    	//cprintf("\nva = %08x\nerr = %i\n", (uint32_t)addr, err);
		panic("pgfault!!!");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 9: Your code here.
	// Allocate a new page, map it at a temporary location (PFTEMP),
	r = sys_page_alloc(sys_getenvid(), PFTEMP, PTE_W | PTE_U | PTE_P);
	if (r)
	{
       	panic("sys_page_alloc: %i", r);
	}
	addr = ROUNDDOWN(addr, PGSIZE);
	// copy the data from the old page to the new page, then move the new page to the old page's address.
    memmove(PFTEMP, addr, PGSIZE);

    r = sys_page_map(sys_getenvid(), PFTEMP, sys_getenvid(), addr, PTE_W | PTE_U | PTE_P);
	if (r)
	{
    	panic("sys_page_map: %i", r);
	}
	r = sys_page_unmap(sys_getenvid(), PFTEMP);
	if (r)
	{
    	panic("sys_page_unmap: %i", r);
	}
	return;

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 9: Your code here.
	//panic("duppage not implemented");
	pte_t pte = uvpt[pn];
	void *va = (void *)(pn << PGSHIFT);

	int err;
	
	if (pte & PTE_SHARE) 
	{
		// cprintf("HAHAHAHAHAHHAHAHAHAHA for %d\n", envid);
		// cprintf("page address [%08x]\n", uvpt[PGNUM(va)]);
		if (sys_page_map(0, va, envid, va, pte & PTE_SYSCALL))
		{	
			panic("duppage share");
		}
	}
	else if ((pte & PTE_W) || (pte & PTE_COW)) 
	{
		err = sys_page_map(sys_getenvid(), va, envid, va, PTE_COW | PTE_U | PTE_P);
        if (err)
        {	
        	panic("duppage cow: %i", err);
        }
        err = sys_page_map(sys_getenvid(), va, sys_getenvid(), va, PTE_COW | PTE_U | PTE_P);
        if (err)
        {	
        	panic("duppage perm: %i", err);
 		}
 	}
	else if (sys_page_map(sys_getenvid(), va, envid, va, PTE_U | PTE_P))
	{
		panic("duppage map");
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 9: Your code here.
	int err;
	// Set up our page fault handler appropriately.
	set_pgfault_handler(pgfault);
	// Create a child.
	envid_t envid = sys_exofork();
	if (envid < 0) 
	{
		panic("exofork: %i",envid);
	}
	else if (envid == 0) 
	{
		//Remember to fix "thisenv" in the child process.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	int ipd, ipt;
	for (ipd = 0; ipd != PDX(UTOP); ipd++) 
	{
		if (!(uvpd[ipd] & PTE_P))
			continue;
		for (ipt = 0; ipt != NPTENTRIES; ipt++) 
		{
			unsigned pn = (ipd << 10) | ipt;
			if (pn == PGNUM(UXSTACKTOP - PGSIZE)) 
			{
				//   so you must allocate a new page for the child's user exception stack.
				err = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P);
				if (err)
				{
					panic("fork xstack: %i", err);
				}
				continue;
			}
			if (uvpt[pn] & PTE_P)
			{	
				duppage(envid, pn);
			}
		}
	}
	err = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
	if (err)
	{	
		panic("sys_env_set_pgfault_upcall: %i", err);
	}
	err = sys_env_set_status(envid, ENV_RUNNABLE);
	if (err)
	{
		panic("sys_env_set_status: %i", err);
	}
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
