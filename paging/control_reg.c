#include <conf.h>
#include <kernel.h>

unsigned long tmp;

unsigned long read_cr0(void)
{
	STATWORD ps;
	unsigned long local_tmp;

	disable(ps);

	asm("pushl %eax");
	asm("movl %cr0, %eax");
	asm("movl %eax, tmp");
	asm("popl %eax");

	local_tmp = tmp;
	
	restore(ps);

	return local_tmp;
}

unsigned long read_cr2(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr2, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}

unsigned long read_cr3(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr3, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}

unsigned long read_cr4(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr4, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}

void write_cr0(unsigned long n)
{
	STATWORD ps;
	disable(ps);

	tmp = n;
	asm("pushl %eax");
	asm("movl tmp, %eax");
	asm("movl %eax, %cr0");
	asm("popl %eax");
	
	restore(ps);
}

void write_cr3(unsigned long n)
{
	STATWORD ps;
	disable(ps);

	tmp = n;
	asm("pushl %eax");
	asm("movl tmp, %eax");
	asm("movl %eax, %cr3");
	asm("popl %eax");

	restore(ps);
}

void write_cr4(unsigned long n)
{
	STATWORD ps;
	disable(ps);

	tmp = n;
	asm("pushl %eax");
	asm("movl tmp, %eax");
	asm("movl %eax, %cr4");
	asm("popl %eax");

	restore(ps);
}

void enable_paging()
{
	unsigned long temp =  read_cr0();
	temp = temp | ( 0x1 << 31 ) | 0x1;
	write_cr0(temp);
}
