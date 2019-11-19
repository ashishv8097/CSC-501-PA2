#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
	STATWORD ps;
	disable(ps);

	/* Checking if all parameters are okay. */
	if(virtpage<VIRTUAL_START || virtpage>0xFFFFF)
	{
		kprintf("\nVPN is in physical memory range, mapping not needed.");
		restore(ps);
		return SYSERR;
	}
	else if(source<0 || source>=BACKING_STORE_ID_MAX)
	{
		kprintf("\nCan't locate BS=%d.",source);
		kprintf("\nBS ID should be in range 0-%d.", BACKING_STORE_ID_MAX-1);
		restore(ps);
		return SYSERR;
	}
	else if( npages<1 || npages>BACKING_STORE_PSIZE )
	{
		kprintf("\nCan't allocate %d pages.", npages);
		kprintf("\nPages should be in range 1-%d.", BACKING_STORE_PSIZE);
		restore(ps);
		return SYSERR;
	}
	else
	{
		bsm_map(currpid, virtpage, source, npages);
	}

	restore(ps);
	return OK;
}

SYSCALL xmunmap(int virtpage)
{
	STATWORD ps;
	disable(ps);

	if(virtpage<VIRTUAL_START)
	{
		kprintf("\nNo such mapping.");
		restore(ps);
		return SYSERR;
	}
	else
		bsm_unmap(currpid, virtpage);

	restore(ps);
	return OK;
}
