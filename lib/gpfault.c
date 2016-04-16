#include <inc/lib.h>

extern void _gpfault_upcall(void);

void (*_gpfault_handler)(struct UTrapframe *utf);

void
set_gpfault_handler(void (*handler)(struct UTrapframe *utf))
{
    int r;

    if (_gpfault_handler == 0) {
        envid_t envid = sys_getenvid();
        int r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W);
        if (r < 0) panic("general protection fault handler: %e", r);
        sys_env_set_gpfault_upcall(envid, _gpfault_upcall);
    }

    _gpfault_handler = handler;
}
