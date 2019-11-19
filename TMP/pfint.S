		.text
pferrcode:	.long	0
		.globl	pfintr
		.globl	pferrcode
pfintr:
	popl	pferrcode
	pushfl
	cli
	pushal
	call	pfint
	popal
	popfl
	iret
