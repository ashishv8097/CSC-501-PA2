#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[BACKING_STORE_ID_MAX];

SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	
	int i,j;
	for(i=0; i<BACKING_STORE_ID_MAX; i++)
	{
		bsm_tab[i].bs_status = BSM_UNMAPPED;
        	bsm_tab[i].bs_npages = 0;
		bsm_tab[i].bs_privacy = NOT_PRIVATE;		
		bsm_tab[i].bs_refcnt = 0;
		
		for(j=0; j<NPROC; j++)
		{
			bsm_tab[i].bs_vpno[j]=0;
			bsm_tab[i].bs_map_list[j]=0;
		
			proctab[j].store[i] = 0;
			proctab[j].vhpno[i] = 0;
			proctab[j].vhpnpages[i] = 0;
		}
	}
	
	restore(ps);
	return OK;
}

SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);

	int i;
	for(i=0; i<BACKING_STORE_ID_MAX; i++)
	{
		if(bsm_tab[i].bs_status==BSM_UNMAPPED)
		{
			*avail = i;
			restore(ps);
			return OK;
		}
	}

	restore(ps);
	return SYSERR;
}

SYSCALL free_bsm(int i)
{
        STATWORD ps;
        disable(ps);

	bsm_tab[i].bs_status = BSM_UNMAPPED;

        restore(ps);
        return OK;
}

SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth)
{
        STATWORD ps;
        disable(ps);

	if(pid==NULLPROC)
	{
		restore(ps);
		return SYSERR;
	}

	int i;
	int lookup_vpno = vaddr>>12;
	int start_vpno_bs, end_vpno_bs;

	for(i=0; i<BACKING_STORE_ID_MAX; i++)
		if(bsm_tab[i].bs_map_list[pid]==1)
		{
			start_vpno_bs = bsm_tab[i].bs_vpno[pid];
			end_vpno_bs = bsm_tab[i].bs_vpno[pid] + bsm_tab[i].bs_npages - 1;
			if(lookup_vpno>=start_vpno_bs && lookup_vpno<=end_vpno_bs)
			{
				*store = i;
				*pageth = lookup_vpno - bsm_tab[i].bs_vpno[pid];
				restore(ps);
				return OK;
			}
		}	

	kprintf("\nBS for PID %d's virtual address %08lx not found.", pid, vaddr);
        restore(ps);
        return SYSERR;
}

SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
        STATWORD ps;
        disable(ps);
	
	/*Don't map if private*/
	if(bsm_tab[source].bs_status == BSM_MAPPED && bsm_tab[source].bs_privacy==PRIVATE)
	{
		kprintf("\nBS %d is already mapped as private heap.", source);
                kprintf("\nCan't map to process %d.", pid);
                restore(ps);
                return SYSERR;
        }

	/*No remap if already mapped*/
	if(proctab[pid].store[source]==1)
	{
		kprintf("\nBS %d is already mapped with process %d", source, pid);
		kprintf("\nTry unmapping and remapping");
		restore(ps);
		return SYSERR;
	}
	
	/*check conflicts*/
	int i, conflict=0;
	int start_pgno, end_pgno, new_start, new_end;
	for(i=0; i<BACKING_STORE_ID_MAX; i++)
		if(proctab[pid].store[i]==1)
		{
			start_pgno = proctab[pid].vhpno[i];
			end_pgno = proctab[pid].vhpno[i] + proctab[pid].vhpnpages[i]-1;

			new_start = vpno;
			new_end = vpno + npages - 1;

			if((new_start>=start_pgno && new_start<=end_pgno)||(new_end>=start_pgno && new_end<=end_pgno))
				conflict++;
		}
	if(conflict>0)
	{
		kprintf("\nSome other BS is mapped in this region.");
                kprintf("\nCan't map.");
                restore(ps);
                return SYSERR;
	}

	if( 1048576 - vpno<npages)
	{
		kprintf("\nCan't map.");
		restore(ps);
		return SYSERR;
	}

	bsm_tab[source].bs_status = BSM_MAPPED;
	bsm_tab[source].bs_refcnt++;
	bsm_tab[source].bs_vpno[pid] = vpno;
	bsm_tab[source].bs_npages = npages;
	bsm_tab[source].bs_map_list[pid] = 1;

	proctab[pid].store[source]=1;
	proctab[pid].vhpno[source]=vpno;
	proctab[pid].vhpnpages[source]=npages;

        restore(ps);
        return OK;
}

SYSCALL bsm_unmap(int pid, int vpno)
{
        STATWORD ps;
        disable(ps);
	
	int i;
	int bs_number=-1;

	for(i=0; i<BACKING_STORE_ID_MAX; i++)
		if(proctab[pid].vhpno[i]==vpno)
		{
			bs_number = i;
			break;
		}

	if(bs_number == -1)
	{
		kprintf("\nNo BS to unmap for PID: %d  VPNO: %d.", pid, vpno);
		restore(ps);
		return SYSERR;
	}

	for(i=0; i<1024; i++)
		if(	frm_tab[i].fr_status==FRM_MAPPED	&&
			frm_tab[i].fr_pid==pid			&& 
			frm_tab[i].fr_type==FR_PAGE		&& 
			frm_tab[i].fr_bsid==bs_number		)
		{
			write_and_free_frm(i, FALSE);
		}

	proctab[pid].store[bs_number] = 0;
	proctab[pid].vhpno[bs_number] = 0;
	proctab[pid].vhpnpages[bs_number] = 0;
	
	bsm_tab[bs_number].bs_map_list[pid]=0;
        bsm_tab[bs_number].bs_vpno[pid]=0;
	bsm_tab[bs_number].bs_refcnt--;

	if(bsm_tab[bs_number].bs_refcnt==0)
		bsm_tab[bs_number].bs_status = BSM_UNMAPPED;

        restore(ps);
        return OK;	
}
