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
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t *pte = PGADDR(PDX(UVPT), PDX(addr), PTX(addr) << 2);

	if (~err & FEC_WR)
	{
		panic("pgfault: not write access");
	}
	if (~(*pte) & PTE_COW)
	{
		panic("pgfault: not access to copy-on-write page");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_P | PTE_U | PTE_W)))
	{
		panic("pgfault: %e\n", r);
	}
	memcpy((void *)PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	if ((r = sys_page_unmap(0, ROUNDDOWN(addr, PGSIZE))))
	{
		panic("pgfault: %e\n", r);
	}
	if ((r = sys_page_map(0, (void *)PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U | PTE_W)))
	{
		panic("pgfault: %e\n", r);
	}

	// panic("pgfault not implemented");
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
	int r;

	// LAB 4: Your code here.
	pte_t *pte = PGADDR(PDX(UVPT), pn / (PGSIZE >> 2), (pn % (PGSIZE >> 2)) << 2);
	void *addr = (void *)(pn * PGSIZE);

	if (((*pte) & (PTE_W | PTE_COW)) && (~(*pte) & PTE_SHARE))
	{
		// writable or copy-on-write page
		int perm = (((*pte) & PTE_SYSCALL)  & (~PTE_W)) | PTE_COW;
		if ((r = sys_page_map(0, addr, envid, addr, perm)))
		{
			panic("duppage: %e\n",  r);
		}
		if ((r = sys_page_map(0, addr, 0, addr, perm)))
		{
			panic("duppage: %e\n", r);
		}
	}
	else 
	{
		// read-only page or shared page
		int perm = (*pte) & PTE_SYSCALL;
		if ((r = sys_page_map(0, addr, envid, addr, perm)))
		{
			panic("duppage: %e\n", r);
		}
	}
	// panic("duppage not implemented");
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
	// LAB 4: Your code here.
	envid_t child;
	int r;

	set_pgfault_handler(&pgfault);

	if ((child = sys_exofork()) < 0)
	{
		panic("fork: %e", child);
	}

	// I'm the child
	if (child == 0) 
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return child;
	}

	for (int i = 0; i <= PDX(USTACKTOP); i++)
	{
		pde_t *pde = PGADDR(PDX(UVPT), PDX(UVPT), (i << 2));
		if (~(*pde) & PTE_P) continue;
		for (int j = 0; j < (PGSIZE >> 2); j++)
		{
			if (i == PDX(USTACKTOP) && j >= PTX(USTACKTOP))
			{
				break;
			}
			pte_t *pte = PGADDR(PDX(UVPT), i, j << 2);
			if (~(*pte) & PTE_P) continue;

			duppage(child, i * (PGSIZE >> 2) + j);
		}
	}

	extern void _pgfault_upcall(void);
	if ((r = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P)))
	{
		panic("fork: %e\n", r);
	}
	if ((r = sys_env_set_pgfault_upcall(child, _pgfault_upcall)))
	{
		panic("fork: %e\n", r);
	}

	if ((r = sys_env_set_status(child, ENV_RUNNABLE)))
	{
		panic("fork: %e\n", r);
	}

	return child;

	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
