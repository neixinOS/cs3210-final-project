/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>

.data
.p2align PGSHIFT
.global idt_beg
idt_beg:
.space 8*256
.global idt_end
idt_end:

###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num, dpl)                                   \
.text 2;\
        .globl name;            /* define global symbol for 'name' */   \
        .type name, @function;  /* symbol type is function */           \
        .align 2;               /* align function definition */         \
        name:                   /* function starts here */              \
        pushl $(num);                                                   \
        jmp _alltraps;\
        SETTG(num, GD_KT, name, dpl)

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num, dpl)                             \
.text 2;\
        .globl name;      \
        .type name, @function;  \
        .align 2;                      \
        name:                           \
        pushl $0;                       \
        pushl $(num);                   \
        jmp _alltraps; \
        SETTG(num, GD_KT, name, dpl)

#define SETTG(num, sel, off, dpl) \
.text 1;\
        movl $idt_beg, %ebx;\
        movl $num, %ecx;\
        leal (%ebx, %ecx, 8), %ebx;\
        movl $off, 0(%ebx);\
        movl $off, 4(%ebx);\
        movw $sel, 2(%ebx);\
        movb $0, 4(%ebx);\
        movb $STS_TG32, %al;\
        movb $dpl, %dl;\
        and $0x3, %dl;\
        shl $0x5, %dl;\
        or %dl, %al;\
        or $0b10000000, %al;\
        movb %al, 5(%ebx);\

#define SETIG(name, sel, off, dpl) \
.text 1;\
        .global name\
        name:\
        movl off, 0(idt_beg,num,8);\
        movl off, 4(idt_beg,num,8);\
        movw $sel, 2(idt_beg,num,8);\
        movb $0, 4(idt_beg,num,8);\
        movb $STS_IG32, %al;\
        movb $dpl, %dl;\
        and $0x3, %dl;\
        shl $0x5, %dl;\
        or %dl, %al;\
        or $0b10000000, %al;\
        movb %al, 5(idt_beg,num,8);\
        ret;
/*
#define SETIG(sel, off, dpl) \
.data;\
        .word $off & 0xffff;\
        .word $sel;\
        .space 1;\
        .byte 0b10000000 | (($dpl & 0x3)<<5) | STS_TG32;\
        .word $off >> 16;

.text*/

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.text 1
.global idt_setup
.type idt_setup, @function
idt_setup:
    pushal

TRAPHANDLER_NOEC(trap_divide, T_DIVIDE, 0)
TRAPHANDLER_NOEC(trap_debug, T_DEBUG, 0)
TRAPHANDLER_NOEC(trap_nmi, T_NMI, 0)
TRAPHANDLER_NOEC(trap_brkpt, T_BRKPT, 3)
TRAPHANDLER_NOEC(trap_oflow, T_OFLOW, 0)
TRAPHANDLER_NOEC(trap_bound, T_BOUND, 0)
TRAPHANDLER_NOEC(trap_illop, T_ILLOP, 0)
TRAPHANDLER_NOEC(trap_device, T_DEVICE, 0)
TRAPHANDLER(trap_dblflt, T_DBLFLT, 0)
TRAPHANDLER(trap_tss, T_TSS, 0)
TRAPHANDLER(trap_segnp, T_SEGNP, 0)
TRAPHANDLER(trap_stack, T_STACK, 0)
TRAPHANDLER(trap_gpflt, T_GPFLT, 0)
TRAPHANDLER(trap_pgflt, T_PGFLT, 0)
TRAPHANDLER_NOEC(trap_fperr, T_FPERR, 0)
TRAPHANDLER(trap_align, T_ALIGN, 0)
TRAPHANDLER_NOEC(trap_mchk, T_MCHK, 0)
TRAPHANDLER_NOEC(trap_simderr, T_SIMDERR, 0)
//syscall handler
TRAPHANDLER_NOEC(trap_syscall, T_SYSCALL, 3)

/*
 * Lab 3: Your code here for _alltraps
 */
.text 2
.global _alltraps;
_alltraps:
    pushl %ds;
    pushl %es;
    pushal;
    movl $GD_KD, %eax;
    movl %eax, %ds;
    movl %eax, %es;
    pushl %esp;
    call trap;

.text 1
    popal
    ret
