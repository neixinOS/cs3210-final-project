#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _divide0_upcall
_divide0_upcall:

  pushl %esp    
  movl _divide0_handler, %eax
  call *%eax
  addl $4, %esp 
  
  movl 0x28(%esp), %eax
	addl $2, %eax
	subl $4, 0x30(%esp)
	movl 0x30(%esp), %ebx
	movl %eax, (%ebx)

	// Restore the trap-time registers.  After you do this, you
	// can no longer modify any general-purpose registers.
	// LAB 4: Your code here.
	addl $8, %esp
	popal

	// Restore eflags from the stack.  After you do this, you can
	// no longer use arithmetic operations or anything else that
	// modifies eflags.
	// LAB 4: Your code here.
	addl $4, %esp
	popfl

	// Switch back to the adjusted trap-time stack.
	// LAB 4: Your code here.
	popl %esp

	// Return to re-execute the instruction that faulted.
	// LAB 4: Your code here.
	ret
