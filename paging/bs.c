#include <bufpool.h>
#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <paging.h>
#include <proc.h>

int get_bs(bsd_t bs_id, unsigned int npages)
{
	STATWORD ps;
	disable(ps);

	if(npages<1 || npages>BACKING_STORE_PSIZE)
	{	
		restore(ps);
		return SYSERR;
	}
	else if(bs_id<0 || bs_id>=BACKING_STORE_ID_MAX)
	{
		restore(ps);
		return SYSERR;
	}	
	else if(bsm_tab[bs_id].bs_privacy==PRIVATE)
	{
		restore(ps);
		return SYSERR;
	}

	if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED)
	{
		restore(ps);
		return npages;
	}
	else
	{
		restore(ps);
		return bsm_tab[bs_id].bs_npages;
	}
}

SYSCALL read_bs(char *dst, bsd_t bs_id, int page)
{
        STATWORD ps;
        disable(ps);

        void * phy_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + page*NBPG;
        bcopy(phy_addr, (void*)dst, NBPG);
        restore(ps);
        return OK;
}

int write_bs(char *src, bsd_t bs_id, int page)
{
        STATWORD ps;
        disable(ps);

        char * phy_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + page*NBPG;
        bcopy((void*)src, phy_addr, NBPG);

        restore(ps);
        return OK;
}

SYSCALL release_bs(bsd_t bs_id)
{
        STATWORD ps;
        disable(ps);

	int i;
	unsigned long *bs_address;

	for(i=0; i<BACKING_STORE_PSIZE; i++)
	{	
		bs_address = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + i*NBPG;
		for( ; bs_address< BACKING_STORE_BASE+(bs_id+1)*BACKING_STORE_UNIT_SIZE; bs_address++)
			*bs_address = 0;
        }

	bsm_tab[i].bs_status=BSM_UNMAPPED;
        bsm_tab[i].bs_npages=0;
        bsm_tab[i].bs_privacy=NOT_PRIVATE;
        
	for(i=0; i<NPROC; i++)
	{	
		bsm_tab[bs_id].bs_vpno[i]=0;
		bsm_tab[bs_id].bs_map_list[i]=0;
		
		proctab[i].store[bs_id]=0;
		proctab[i].vhpno[bs_id]=0;
		proctab[i].vhpnpages[bs_id]=0;
	}

	restore(ps);
        return OK;
}

