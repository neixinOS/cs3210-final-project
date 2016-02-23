/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

.data
.global idt_beg
idt_beg:
.space 8*256
.global idt_end
idt_end:
.text

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
#define TRAPHANDLER(name, num)                                          \
 .text;\
        .globl name;            /* define global symbol for 'name' */   \
        .type name, @function;  /* symbol type is function */           \
        .align 2;               /* align function definition */         \
        name:                   /* function starts here */              \
        pushl $(num);                                                   \
        jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)                                     \
.text;                                                                  \
        .globl name;                                                    \
        .type name, @function;                                          \
        .align 2;                                                       \
        name:                                                           \
        pushl $0;                                                       \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  

#define SETTG(name, num, sel, off, dpl) \
.text;\
        .global trap_divide_setup;\
        trap_divide_setup:\
        pushal;\
        movl $idt_beg, %ebx;\
        movl $num, %ecx;\
        lea (%ebx, %ecx, 8), %ebx;\
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
        ret;

#define SETIG(name, sel, off, dpl) \
.text;\
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

TRAPHANDLER_NOEC(trap_divide, T_DIVIDE)
TRAPHANDLER_NOEC(trap_debug, T_DEBUG)
TRAPHANDLER_NOEC(trap_nmi, T_NMI)
TRAPHANDLER_NOEC(trap_brkpt, T_BRKPT)
TRAPHANDLER_NOEC(trap_oflow, T_OFLOW)
TRAPHANDLER_NOEC(trap_bound, T_BOUND)
TRAPHANDLER_NOEC(trap_illop, T_ILLOP)
TRAPHANDLER_NOEC(trap_device, T_DEVICE)
TRAPHANDLER(trap_dblflt, T_DBLFLT)
TRAPHANDLER(trap_tss, T_TSS)
TRAPHANDLER(trap_segnp, T_SEGNP)
TRAPHANDLER(trap_stack, T_STACK)
TRAPHANDLER(trap_gpflt, T_GPFLT)
TRAPHANDLER(trap_pgflt, T_PGFLT)
TRAPHANDLER_NOEC(trap_fperr, T_FPERR)
TRAPHANDLER(trap_align, T_ALIGN)
TRAPHANDLER_NOEC(trap_mchk, T_MCHK)
TRAPHANDLER_NOEC(trap_simderr, T_SIMDERR)
//syscall handler
TRAPHANDLER_NOEC(trap_syscall, T_SYSCALL)

/*
 * Lab 3: Your code here for _alltraps
 */
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

.data
.global idt_info_end
idt_info_end:
