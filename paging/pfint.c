#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

SYSCALL pfint()
{
	STATWORD ps;
	disable(ps);
	unsigned long ul_faulted_addr = read_cr2();
	virt_addr_t *faulted_addr;
	
	faulted_addr->pd_offset = ul_faulted_addr >> 22 ;
	faulted_addr->pt_offset = (ul_faulted_addr >> 12) & 0x003ff ;
	faulted_addr->pg_offset = ul_faulted_addr & 0x00000fff ;

	pd_t *pd_entry = proctab[currpid].pdbr + faulted_addr->pd_offset*sizeof(pd_t);
	if(pd_entry->pd_pres != 1)
	{
		int pt_frame;
		if(get_frm(&pt_frame)==SYSERR)
		{
			replace_page(&pt_frame);
			restore(ps);
			return OK;
		}
		
		pd_entry->pd_pres = 1;
		pd_entry->pd_write = 1;
		pd_entry->pd_base = FRAME0+pt_frame;

		frm_tab[pt_frame].fr_status = FRM_MAPPED;
		frm_tab[pt_frame].fr_pid = currpid;
		frm_tab[pt_frame].fr_type = FR_TBL;
		frm_tab[pt_frame].fr_refcnt = 0;
	}
	
	pt_t *pt_entry = pd_entry->pd_base*NBPG + faulted_addr->pt_offset*sizeof(pt_t);
	if(pt_entry->pt_pres != 1)
        {	
		int store, pageth;
		int pg_frame;
		
		if(get_frm(&pg_frame)==SYSERR)
		{
			replace_page(&pg_frame);
			restore(ps);
			return OK;
		}

		pt_entry->pt_pres = 1;
		pt_entry->pt_write = 1;
		pt_entry->pt_dirty = 0;
		pt_entry->pt_base = FRAME0+pg_frame;
		
		frm_tab[pd_entry->pd_base-FRAME0].fr_refcnt++;		/*when a new page is created, increase the refcount for the page table frame*/

		frm_tab[pg_frame].fr_status = FRM_MAPPED;
		frm_tab[pg_frame].fr_pid = currpid;
		frm_tab[pg_frame].fr_type = FR_PAGE;
		frm_tab[pg_frame].fr_vpno = ul_faulted_addr/NBPG;
		insert_frame(pg_frame);
			
		if(bsm_lookup(currpid, ul_faulted_addr, &store, &pageth)!=SYSERR)
		{
			read_bs((char *)(pt_entry->pt_base*NBPG), store, pageth);
			frm_tab[pg_frame].fr_bsid = store;
			frm_tab[pg_frame].fr_bs_page = pageth;
		}	
		else
		{
			if(currpid!=0)
			{
				kprintf("\nBSM not found.");
				kill(currpid);
			}
		}
		
        }

	write_cr3(proctab[currpid].pdbr);
	
	restore(ps);
	return OK;
}
