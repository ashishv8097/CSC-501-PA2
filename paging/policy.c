#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

frame_queue fr_q[];

int debug_flag;
int q_head;
int circ_q_len;

SYSCALL srpolicy(int policy)
{
	STATWORD ps;
	disable(ps);

	if( policy == SC )
		page_replace_policy = SC;
	else if( policy == AGING )
        {
		page_replace_policy = AGING;

		q_head = 0;
		fr_q[q_head].fr_prev = q_head;
		fr_q[q_head].fr_age = -1;
		fr_q[q_head].fr_next = q_head;
	}
	else
        {
                restore(ps);
                return SYSERR;
        }

	debug_flag = 1;
	
	restore(ps);
	return OK;
}

SYSCALL grpolicy()
{
	return page_replace_policy;
}

SYSCALL insert_frame(int fr_no)
{
	STATWORD ps;
	disable(ps);

	int next, prev;

	if(page_replace_policy==SC)
	{
		if(circ_q_len==0)
		{
			q_head = fr_no;
			fr_q[q_head].fr_prev = q_head;
			fr_q[q_head].fr_next = q_head;
		}
		else
		{
			fr_q[fr_q[q_head].fr_prev].fr_next = fr_no;
			fr_q[fr_no].fr_prev = fr_q[q_head].fr_prev;
			fr_q[fr_no].fr_next = q_head;
			fr_q[q_head].fr_prev = fr_no;
		}
			
		circ_q_len++;
	}
	else
	{
		fr_q[fr_q[q_head].fr_prev].fr_next = fr_no;

                fr_q[fr_no].fr_prev = fr_q[q_head].fr_prev;
		fr_q[fr_no].fr_age = 0;
                fr_q[fr_no].fr_next = q_head;

                fr_q[q_head].fr_prev = fr_no;
	}
	restore(ps);
	return OK;
}

SYSCALL	extract_frame(int policy, int *frame_to_remove)
{
	STATWORD ps;
        disable(ps);
	int prev, select, next;
        char pid;
        pd_t *pd_entry;
        pt_t *pt_entry;
	virt_addr_t *splits;

        if(page_replace_policy==SC)
        {
                /* SC policy */
		if(circ_q_len==0)
		{
			kprintf("\nNo frames in queue.");
			kprintf("\nPossibly low memory. Shutting down...\n");
			shutdown();
			restore(ps);
			return SYSERR;
		}
		
		while(1)
		{
			splits->pd_offset = frm_tab[q_head].fr_vpno >> 10;
			splits->pt_offset = frm_tab[q_head].fr_vpno & 0x003ff;
			
			pid = frm_tab[q_head].fr_pid;

			pd_entry = proctab[pid].pdbr + splits->pd_offset*sizeof(pd_t);
			pt_entry = pd_entry->pd_base*NBPG + splits->pt_offset*sizeof(pt_t);
			
			if(pt_entry->pt_acc!=0)
                        {
			        pt_entry->pt_acc=0;
				q_head = fr_q[q_head].fr_next;
			}
                        else
                                break;
		}

		*frame_to_remove = q_head;
		circ_q_len--;

		if(circ_q_len==0)
		{
			fr_q[q_head].fr_prev = -1;
                	fr_q[q_head].fr_next = -1;
		}
		else
		{
			prev = fr_q[q_head].fr_prev;
               		next = fr_q[q_head].fr_next;

                	fr_q[next].fr_prev = prev;
                	fr_q[prev].fr_next = next;

			fr_q[q_head].fr_next = -1;
			fr_q[q_head].fr_prev = -1;

			q_head = next;
		}
		
                restore(ps);
                return OK;
	}
        else
        {
		/*Traverse 1*/
		select = fr_q[q_head].fr_next;
		
		if(select == q_head) /*checking if queue is empty*/
		{
			kprintf("\nNo frames in queue.");
			kprintf("\nPossibly low memory. Shutting down...\n");
                        shutdown();
			restore(ps);
			return SYSERR;
		}

		while( select != q_head )
                {
                        splits->pd_offset = frm_tab[select].fr_vpno >> 10 ;
                        splits->pt_offset = frm_tab[select].fr_vpno & 0x003ff ;
                        pid = frm_tab[select].fr_pid;

                        pd_entry = proctab[pid].pdbr + splits->pd_offset*sizeof(pd_t);
                        pt_entry = pd_entry->pd_base*NBPG + splits->pt_offset*sizeof(pt_t);
			fr_q[select].fr_age = fr_q[select].fr_age/2;
			if(pt_entry->pt_acc!=0)
			{
				if(fr_q[select].fr_age+128 > 255)
					fr_q[select].fr_age=255;
				else
					fr_q[select].fr_age=fr_q[select].fr_age+128;

				pt_entry->pt_acc=0;
			}
			select = fr_q[select].fr_next;
		}

		select = fr_q[q_head].fr_next;
		char min_age=255;
		int to_extract = select;

		while( select != q_head )
                {
			if( fr_q[select].fr_age < min_age )
			{
				min_age = fr_q[select].fr_age;
				to_extract = select;
			}
			
			if(min_age==0)
			{
				prev = fr_q[to_extract].fr_prev;
                		next = fr_q[to_extract].fr_next;

                		fr_q[to_extract].fr_prev = prev;
                		fr_q[to_extract].fr_next = next;

				fr_q[to_extract].fr_next = -1;
                		fr_q[to_extract].fr_age = -1;
                		fr_q[to_extract].fr_prev = -1;

                		*frame_to_remove = to_extract;
                		restore(ps);
                		return OK;
			}

			select = fr_q[select].fr_next;
		}
		select = to_extract;
	
		prev = fr_q[select].fr_prev;
                next = fr_q[select].fr_next;

                fr_q[next].fr_prev = prev;
                fr_q[prev].fr_next = next;

		fr_q[select].fr_next = -1;
		fr_q[select].fr_age = -1;
		fr_q[select].fr_prev = -1;

                *frame_to_remove = select;

                restore(ps);
                return OK;
        }
}

SYSCALL replace_page(int *frame_to_remove)
{
	STATWORD ps;
        disable(ps);

	extract_frame(page_replace_policy, &frame_to_remove);
	write_and_free_frm(frame_to_remove, TRUE);

	if(debug_flag==1)
		kprintf("Replaced frame: %d\n", frame_to_remove);

        restore(ps);
        return OK;
}
