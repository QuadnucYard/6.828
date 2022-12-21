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
#define TRAPHANDLER(name, num)										\
.data;																\
	.long num, name;												\
.text;																\
	.type name, @function;	/* symbol type is function */			\
	.align 2;				/* align function definition */			\
	name:					/* function starts here */				\
	pushl $(num);													\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)									\
.data;																\
	.long num, name;												\
.text;																\
	.type name, @function;											\
	.align 2;														\
	name:															\
	pushl $0;														\
	pushl $(num);													\
	jmp _alltraps

#define __CAT(x, y) x##y
#define __ANONYMOUS(pre, post) __CAT(pre, post)
#define ANONYMOUS(pre) __ANONYMOUS(pre, __COUNTER__)

#define TH(num) \
	TRAPHANDLER(ANONYMOUS(handler_), num)

#define THN(num) \
	TRAPHANDLER_NOEC(ANONYMOUS(handler_), num)

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.data
	.globl trap_table
	trap_table:

THN(T_DIVIDE)
THN(T_DEBUG)
THN(T_NMI)
THN(T_BRKPT)
THN(T_OFLOW)
THN(T_BOUND)
THN(T_ILLOP)
THN(T_DEVICE)
TH(T_DBLFLT)
TH(T_TSS)
TH(T_SEGNP)
TH(T_STACK)
TH(T_GPFLT)
TH(T_PGFLT)
THN(T_FPERR)
TH(T_ALIGN)
THN(T_MCHK)
THN(T_SIMDERR)
THN(T_SYSCALL)
THN(IRQ_OFFSET + IRQ_TIMER)
THN(IRQ_OFFSET + IRQ_KBD)
THN(IRQ_OFFSET + IRQ_SERIAL)
THN(IRQ_OFFSET + IRQ_SPURIOUS)
THN(IRQ_OFFSET + IRQ_IDE)
THN(IRQ_OFFSET + IRQ_ERROR)

/* End of table */
.data
	.long 0, 0

/*
 * Lab 3: Your code here for _alltraps
 */
.text
_alltraps:
    pushl %ds
    pushl %es
    pushal
    movw $GD_KD, %eax
    movw %ax, %ds
    movw %ax, %es
    pushl %esp
    call trap