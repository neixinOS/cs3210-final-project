#include <inc/lib.h>

extern void _divide0_upcall(void);

void (*_divide0_handler)(struct UTrapframe *utf);

void
set_divide0_handler(void (*handler)(struct UTrapframe *utf))
{
    int r;

    if (_divide0_handler == 0) {
        envid_t envid = sys_getenvid();
        int r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W);
        if (r < 0) panic("divide by zero handler: %e", r);
        sys_env_set_divide0_upcall(envid, _divide0_upcall);
    }

    _divide0_handler = handler;
}
