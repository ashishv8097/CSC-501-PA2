#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[];

SYSCALL init_frm()
{
	STATWORD ps;
	disable(ps);

	int i;
	for(i=0; i<NFRAMES; i++)
	{
  		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = 0;
  		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].fr_bsid = -1;
		frm_tab[i].fr_bs_page = -1;
	}

 	restore(ps);
  	return OK;
}

SYSCALL get_frm(int* avail)
{
	STATWORD ps;
	disable(ps);

	int i;
	for(i=0; i<NFRAMES; i++)
  		if(frm_tab[i].fr_status==FRM_UNMAPPED)
		{
			*avail = i;
			restore(ps);
			return OK;
		}

 	restore(ps);
  	return SYSERR;
}

SYSCALL write_and_free_frm(int i, int free)
{
	STATWORD ps;
	disable(ps);
	
	if(frm_tab[i].fr_status == FRM_UNMAPPED)
	{
		kprintf("\nFrame %d unampped.", i);
		restore(ps);
		return SYSERR;
	}
	
	/*writing the page back to BS*/
	if(frm_tab[i].fr_type == FR_PAGE)
	{
		int store, pageth;
		virt_addr_t *addr;
		
		addr->pd_offset = frm_tab[i].fr_vpno >> 10 ;
		addr->pt_offset = frm_tab[i].fr_vpno & 0x003ff ;
		
		pd_t *pd_entry;
		pt_t *pt_entry;
		
		pd_entry = proctab[frm_tab[i].fr_pid].pdbr + addr->pd_offset*sizeof(pd_t);
		pt_entry = pd_entry->pd_base*NBPG + addr->pt_offset*sizeof(pt_t);
		
		if(pt_entry->pt_dirty==1)
		{
			store = frm_tab[i].fr_bsid;
			pageth = frm_tab[i].fr_bs_page;
			write_bs((char *)((FRAME0+i)*NBPG), store, pageth);
			pt_entry->pt_dirty=0;
		}
		
		if(free==TRUE)
		{
			pt_entry->pt_pres=0;						/*make pt_entry as not present*/	
			
			int pt_fno = pd_entry->pd_base-FRAME0;
			frm_tab[pt_fno].fr_refcnt--;					/*reducing reference count for page table*/

			if(frm_tab[pt_fno].fr_refcnt==0)				/*when PT refcount is 0, i.e. when no page mapped to PT*/
			{
				frm_tab[pt_fno].fr_status = FRM_UNMAPPED;		/*remove mapping for PT*/
                		frm_tab[pt_fno].fr_pid = -1;
                		frm_tab[pt_fno].fr_vpno = 0;
                		frm_tab[pt_fno].fr_refcnt = 0;
                		frm_tab[pt_fno].fr_type = FR_PAGE;
                		frm_tab[pt_fno].fr_dirty = 0;
                		frm_tab[pt_fno].fr_bsid = -1;
                		frm_tab[pt_fno].fr_bs_page = -1;
			
				pd_entry->pd_pres=0;					/*mark pd_entry as not present when PT is removed*/
			}
		}
	}

	if(free==TRUE)
	{
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = 0;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].fr_bsid = -1;
		frm_tab[i].fr_bs_page = -1;
	}

	restore(ps);
	return OK;
}

void create_pd(int pid)
{
	int pt_frame;
	pd_t *pd_entry;

	if(get_frm(&pt_frame)==SYSERR)
		replace_page(&pt_frame);

	frm_tab[pt_frame].fr_status = FRM_MAPPED;
	frm_tab[pt_frame].fr_pid = pid;
	frm_tab[pt_frame].fr_type = FR_DIR;

	proctab[pid].pdbr = (FRAME0+pt_frame)*NBPG;
	pd_entry = proctab[pid].pdbr;

	int i;
	for (i=0; i<1024; i++, pd_entry++)
		if(i<4)
		{
			pd_entry->pd_pres = 1;
			pd_entry->pd_write = 0;
			pd_entry->pd_base = FRAME0+i ;
		}
		else
		{
			pd_entry->pd_pres = 0;
			pd_entry->pd_write = 1;
		}
}
