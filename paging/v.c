#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/* Declaration */
LOCAL   newpid();
struct pentry proctab[];

/*----------------------------------------------------------------------n
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
        disable(ps);

	if(hsize<0 || hsize>BACKING_STORE_PSIZE)
	{
		kprintf("\nPage size out of bounds.");
		restore(ps);
		return SYSERR;
	}
	
	int bs_id;
	if( get_bsm(&bs_id) == SYSERR)
	{
		kprintf("\nNo free BS found. Process can't be created.");
		restore(ps);
		return SYSERR;
	}

	int pid = create(procaddr, ssize, priority, name, nargs, args);
	bsm_tab[bs_id].bs_status = BSM_MAPPED;
	bsm_tab[bs_id].bs_vpno[pid] = VIRTUAL_START ;
	bsm_tab[bs_id].bs_npages = hsize;
	bsm_tab[bs_id].bs_privacy = PRIVATE;
	bsm_tab[bs_id].bs_map_list[pid] = 1;
	
	proctab[pid].store[bs_id] = 1;
	proctab[pid].vhpno[bs_id] = VIRTUAL_START;
	proctab[pid].vhpnpages[bs_id] = hsize;

	proctab[pid].vmemlist->mnext = VIRTUAL_START*NBPG;
	struct mblock *base;
	base = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE;
	base->mlen = hsize*NBPG;
	base->mnext = NULL;

	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}

/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD *vgetmem(nbytes)
	unsigned nbytes;
{
        STATWORD ps;
	kprintf("\nBe");
        struct mblock *p, *q, *leftover ;

        disable(ps);
        if( nbytes==0 || proctab[currpid].vmemlist->mnext==(struct mblock *)NULL)
	{
                restore(ps);
                return( (WORD *)SYSERR);
        }

        nbytes = (unsigned int) roundmb(nbytes);
        for( q=&(proctab[currpid].vmemlist), p=proctab[currpid].vmemlist->mnext; p != (struct mblock *) NULL; q=p, p=p->mnext)
        	if ( p->mlen == nbytes)
		{
                        q->mnext = p->mnext;
                        restore(ps);
                        return( (WORD *)p );
                }
		else if ( p->mlen > nbytes ) 
		{
                        leftover = (struct mblock *)( (unsigned)p + nbytes );
                        q->mnext = leftover;
                        leftover->mnext = p->mnext;
                        leftover->mlen = p->mlen - nbytes;
                        restore(ps);
                        return( (WORD *)p );
                }

        restore(ps);
        return( (WORD *)SYSERR );
}

/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;
        struct  mblock  *p, *q;
        unsigned top;

        if (size==0 || (unsigned)block>(unsigned)maxaddr
            || ((unsigned)block)<((unsigned) &end))
                return(SYSERR);
        size = (unsigned)roundmb(size);
        disable(ps);
        for( p=proctab[currpid].vmemlist->mnext,q= &(proctab[currpid].vmemlist);
             p != (struct mblock *) NULL && p < block ;
             q=p,p=p->mnext )
                ;
        if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= &proctab[currpid].vmemlist) ||
            (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
                restore(ps);
                return(SYSERR);
        }
        if ( q!= &proctab[currpid].vmemlist && top == (unsigned)block )
                        q->mlen += size;
        else {
                block->mlen = size;
                block->mnext = p;
                q->mnext = block;
                q = block;
        }
        if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
                q->mlen += p->mlen;
                q->mnext = p->mnext;
        }
        restore(ps);
        return(OK);
}
