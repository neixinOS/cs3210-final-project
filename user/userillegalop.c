#include <inc/lib.h>

void 
handler(struct UTrapframe *utf) 
{
	cprintf("Challenge!! Illegal opcode handler in user mode!\n");

}

void
umain(int argc, char **argv)
{
	set_illegalop_handler(handler);
	asm volatile("addpd %xmm2, %xmm1");
	cprintf("Running!\n");
}
