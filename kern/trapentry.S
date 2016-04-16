/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


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
        .globl name;                                                    \
        .type name, @function;                                          \
        .align 2;                                                       \
        name:                                                           \
        pushl $0;                                                       \
        pushl $(num);                                                   \
        jmp _alltraps



#define trap_handler(name, num)                                         \
        .data;                                                          \
          .long name;                                                   \
        .text;                                                          \
          .globl name;                                                  \
          .type name, @function;                                        \
          .align 2;                                                     \
        name:                                                           \
          pushl $(num);                                                 \
          jmp _alltraps

#define trap_handler_noec(name, num)                                    \
        .data;                                                          \
          .long name;                                                   \
        .text;                                                          \
          .globl name;                                                  \
          .type name, @function;\
          .align 2;                                                     \
        name:                                                           \
          pushl $0;                                                     \
          pushl $(num);                                                 \
          jmp _alltraps

#define undefined()                                                     \
        .data;                                                          \
          .long 0

.data
  .p2align 2
  .globl table
table:
.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
    /*TRAPHANDLER_NOEC(divide_error, T_DIVIDE)                                  # Divide error
    TRAPHANDLER_NOEC(debug, T_DEBUG)                                                    # Debug
    TRAPHANDLER_NOEC(nmi, T_NMI)                                                            # Non-Maskable Interrupt
  TRAPHANDLER_NOEC(breakpoint, T_BRKPT)                                         # Breakpoint
    TRAPHANDLER_NOEC(overflow, T_OFLOW)                                             # Overflow
    TRAPHANDLER_NOEC(bound, T_BOUND)                                                    # BOUND Range Exceeded
    TRAPHANDLER_NOEC(illegal_op, T_ILLOP)                                           # Invalid Opcode
    TRAPHANDLER_NOEC(device_not_available, T_DEVICE)                    # Device Not Available
    TRAPHANDLER(double_fault, T_DBLFLT)                                             # Double Fault
    //TRAPHANDLER_NOEC(coprocessor_segment_overrun, T_COPROC)       # Coprocessor Segment Overrun
    TRAPHANDLER(invalid_TSS, T_TSS)                                                     # Invalid TSS
    TRAPHANDLER(segment_not_present, T_SEGNP)                                   # Segment Not Present
    TRAPHANDLER(stack_segment, T_STACK)                                             # Stack Fault
    TRAPHANDLER(general_protection, T_GPFLT)                                    # General Protection
    TRAPHANDLER(page_fault, T_PGFLT)                                                    # Page Fault
    //TRAPHANDLER_NOEC(reserved, T_RES)                                                 # (unknown trap)
    TRAPHANDLER_NOEC(float_point_error, T_FPERR)                            # x87 FPU Floating-Point Error
    TRAPHANDLER(alignment_check, T_ALIGN)                                           # Alignment Check
    TRAPHANDLER_NOEC(machine_check, T_MCHK)                                     # Machine-Check
    TRAPHANDLER_NOEC(SIMD_float_point_error, T_SIMDERR)             # SIMD Floating-Point Exception*/
/*
 * challenge
 */
  trap_handler_noec(divide_error, T_DIVIDE)                  # Divide error
  trap_handler_noec(debug, T_DEBUG)                          # Debug
  trap_handler_noec(nmi, T_NMI)                              # Non-Maskable Interrupt
  trap_handler_noec(breakpoint, T_BRKPT)                     # Breakpoint
  trap_handler_noec(overflow, T_OFLOW)                       # Overflow
  trap_handler_noec(bound, T_BOUND)                          # BOUND Range Exceeded
  trap_handler_noec(illegal_op, T_ILLOP)                     # Invalid Opcode
  trap_handler_noec(device_not_available, T_DEVICE)          # Device Not Available
  trap_handler(double_fault, T_DBLFLT)                       # Double Fault
  undefined()                                                # Coprocessor Segment Overrun
  trap_handler(invalid_TSS, T_TSS)                           # Invalid TSS
  trap_handler(segment_not_present, T_SEGNP)                 # Segment Not Present
  trap_handler(stack_segment, T_STACK)                       # Stack Fault
  trap_handler(general_protection, T_GPFLT)                  # General Protection
  trap_handler(page_fault, T_PGFLT)                          # Page Fault
  undefined()                                                # (unknown trap)
  trap_handler_noec(float_point_error, T_FPERR)              # x87 FPU Floating-Point Error
  trap_handler(alignment_check, T_ALIGN)                     # Alignment Check
  trap_handler_noec(machine_check, T_MCHK)                   # Machine-Check
  trap_handler_noec(SIMD_float_point_error, T_SIMDERR)       # SIMD Floating-Point Exception
    trap_handler_noec(irq_0, (IRQ_OFFSET + IRQ_TIMER))         # 20---32, Hardware IRQ numbers, IRQ_OFFSET+IRQ_WHATEVER
  trap_handler_noec(irq_1, (IRQ_OFFSET + IRQ_KBD))           # 33
  trap_handler_noec(irq_2, (IRQ_OFFSET + 2))                 # 34
  trap_handler_noec(irq_3, (IRQ_OFFSET + 3))                 # 35
  trap_handler_noec(irq_4, (IRQ_OFFSET + IRQ_SERIAL))        # 36
  trap_handler_noec(irq_5, (IRQ_OFFSET + 5))                 # 37
  trap_handler_noec(irq_6, (IRQ_OFFSET + 6))                 # 38
  trap_handler_noec(irq_7, (IRQ_OFFSET + IRQ_SPURIOUS))      # 39   
  trap_handler_noec(irq_8, (IRQ_OFFSET + 8))                 # 40
  trap_handler_noec(irq_9, (IRQ_OFFSET + 9))                 # 41
  trap_handler_noec(irq_10, (IRQ_OFFSET + 10))               # 42
  trap_handler_noec(irq_11, (IRQ_OFFSET + 11))               # 43
  trap_handler_noec(irq_12, (IRQ_OFFSET + 12))               # 44
  trap_handler_noec(irq_13, (IRQ_OFFSET + 13))               # 45
  trap_handler_noec(irq_14, (IRQ_OFFSET + IRQ_IDE))          # 46
  trap_handler_noec(irq_15, (IRQ_OFFSET + 15))               # 47
  trap_handler_noec(system_call, T_SYSCALL)                  # system call

/*
 * Lab 3: Your code here for _alltraps
 */
.text
  .globl _alltraps
_alltraps:
  # push values to make the stack look like a struct Trapframe
  pushl %ds
  pushl %es
  pushal
  # load GD_KD into %ds and %es
  movl $GD_KD, %eax
  movw %ax,%ds
  movw %ax,%es
  # pushl %esp to pass a pointer to the Trapframe as an argument to trap()
  pushl %esp
  # call trap (can trap ever return?)
  movl $0, %ebp
  call trap
  # iret not return


