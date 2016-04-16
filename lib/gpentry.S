#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _gpfault_upcall
_gpfault_upcall:

	pushl %esp			
	movl _gpfault_handler, %eax
	call *%eax
	addl $4, %esp			
	
	movl 0x28(%esp), %eax;
	addl $2, %eax;
	movl 0x30(%esp), %ebx;
	subl $4, %ebx;
	movl %ebx, 0x30(%esp);
	movl %eax, (%ebx);

	addl $8, %esp;
	popal;

	addl $4, %esp;
	popfl;

	pop %esp;

	ret;
