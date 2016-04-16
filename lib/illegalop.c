#include <inc/lib.h>

extern void _illegalop_upcall(void);

void (*_illegalop_handler)(struct UTrapframe *utf);

void
set_illegalop_handler(void (*handler)(struct UTrapframe *utf))
{
    int r;

    if (_illegalop_handler == 0) {
        envid_t envid = sys_getenvid();
        int r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W);
        if (r < 0) panic("illegal opcode handler: %e", r);
        sys_env_set_illegalop_upcall(envid, _illegalop_upcall);
    }

    _illegalop_handler = handler;
}
