#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
	cprintf("Challenge!! general protection fault handler in user mode\n");
}

void
umain(int argc, char **argv)
{

	set_gpfault_handler(handler);

	asm volatile("int $14");
	cprintf("Running...\n");
}
