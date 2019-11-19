/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h> 

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;   
//	kprintf("\nKill called for %d", pid); 
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();
	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	send(pptr->pnxtkin, pid);
	freestk(pptr->pbase, pptr->pstklen);

	/*Garbage collection*/
        //kprintf("\n%d - Garbage", pid);
        int i;
        for(i=5; i<NFRAMES; i++)
        {
                if(frm_tab[i].fr_pid==pid && frm_tab[i].fr_status==FRM_MAPPED)
                {
                        write_and_free_frm(i, TRUE);
                }
//		kprintf("\nfine till %d",i);
        }
//	kprintf("\nFrames freed.");
        for(i=0; i<BACKING_STORE_ID_MAX; i++)
        {
		if(proctab[pid].store[i]==1)
                        bsm_unmap(pid, bsm_tab[i].bs_vpno[pid]);
        }
//	kprintf("\n%d killed", pid);
        /*Garbage collection ends*/

	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	
	/*Garbage collection
	kprintf("\n%d - Garbage", pid);
	int i;
	for(i=4; i<NFRAMES; i++)
	{
		if(frm_tab[i].fr_pid==pid)
		{
			free_frm(i);
		}
	}
	
	for(i=0; i<BACKING_STORE_ID_MAX; i++)
	{
		if(bsm_tab[i].bs_pid==pid)
			bsm_unmap(pid, bsm_tab[i].bs_vpno[pid]);
	}
	Garbage collection ends*/

	restore(ps);
	return(OK);
}
