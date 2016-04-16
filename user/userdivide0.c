// divide by zero

#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
  cprintf("Challenge!! Divide by zero handler in user mode! The wrong answer generated is: ");
}

void
umain(int argc, char **argv)
{
  set_divide0_handler(handler);

  int x, y;
  x = 3;
  y = 0;
  cprintf("%d\n", x/y);
  cprintf("Running...\n");
}
