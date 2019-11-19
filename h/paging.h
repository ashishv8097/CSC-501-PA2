typedef unsigned int	 bsd_t;

typedef struct 
{
	unsigned int pd_pres	: 1;		/* page table present?		*/
  	unsigned int pd_write 	: 1;		/* page is writable?		*/
  	unsigned int pd_user	: 1;		/* is use level protection?	*/
  	unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
 	unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  	unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  	unsigned int pd_mbz	: 1;		/* must be zero			*/
  	unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  	unsigned int pd_global	: 1;		/* global (ignored)		*/
  	unsigned int pd_avail 	: 3;		/* for programmer's use		*/
  	unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

typedef struct
{
	unsigned int pt_pres	: 1;		/* page is present?		*/
	unsigned int pt_write 	: 1;		/* page is writable?		*/
	unsigned int pt_user	: 1;		/* is use level protection?	*/
	unsigned int pt_pwt	: 1;		/* write through for this page? */
	unsigned int pt_pcd	: 1;		/* cache disable for this page? */
	unsigned int pt_acc	: 1;		/* page was accessed?		*/
	unsigned int pt_dirty 	: 1;		/* page was written?		*/
	unsigned int pt_mbz	: 1;		/* must be zero			*/
	unsigned int pt_global	: 1;		/* should be zero in 586	*/
	unsigned int pt_avail 	: 3;		/* for programmer's use		*/
	unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct
{
	unsigned int pg_offset : 12;		/* page offset			*/
	unsigned int pt_offset : 10;		/* page table offset		*/
	unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct
{
	int bs_status;			/* MAPPED or UNMAPPED		*/
	int bs_refcnt;			/* process id using this slot   */
	int bs_vpno[NPROC];		/* starting virtual page number */
	int bs_npages;			/* number of pages in the store */
	int bs_privacy;
	int bs_map_list[NPROC];
} bs_map_t;

typedef struct
{
	int fr_status;			/* MAPPED or UNMAPPED		*/
	int fr_pid;			/* process id using this frame  */
  	int fr_vpno;			/* corresponding virtual page no*/
  	int fr_refcnt;			/* reference count		*/
  	int fr_type;			/* FR_DIR, FR_TBL, FR_PAGE	*/
  	int fr_dirty;
	int fr_bsid;
	int fr_bs_page;
} fr_map_t;

/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xmunmap(int);

/* given calls for dealing with backing store */
int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);

/*frame's functions*/
SYSCALL free_frame(int i);

/*v.c*/

/*Policy*/
//extern unsigned char page_replace_policy;
//extern char fifo_head, circ_head;
//extern char circ_q_len;
//extern char debug_flag;

//typedef struct
//{
//        int fr_prev;
//	unsigned char fr_age;
//        int fr_next;
//} frame_queue;

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame of free frames	*/
#define NFRAMES 	1024	/* number of free frames	*/
#define VIRTUAL_START	4096	/*all beyond this is virtual	*/

#define BSM_UNMAPPED	0
#define BSM_MAPPED	1
#define NOT_PRIVATE	0
#define	PRIVATE		1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define SC 		3
#define AGING 		4

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000
#define BACKING_STORE_ID_MAX	8
#define BACKING_STORE_PSIZE	256
#define BS_START_PGNO		2048

typedef struct
{
	int fr_prev;
	unsigned char fr_age;
	int fr_next;
} frame_queue;

extern bs_map_t bsm_tab[BACKING_STORE_ID_MAX];
extern fr_map_t frm_tab[NFRAMES];

extern frame_queue fr_q[NFRAMES];
extern int page_replace_policy;
extern int q_head;
extern int circ_q_len;
extern int debug_flag;
