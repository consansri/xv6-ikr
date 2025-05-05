#include "include/memlayout.h"
#include "sbi/include/sbi_trap.h"

    .equ        UART_BASE_ADDR,     0x8000000000000001
    .equ        MIDELEG_VAL,        0b1000001000100000 //  s-timer, s-external
    //.equ        MEDELEG_VAL,        0b1011000100000000 // all page faults and ecall from user mode go to xv6
    //.equ        MEDELEG_VAL,        0b1111111111111111 // all exceptions get handled by xv6
    .equ        MEDELEG_VAL,        0b1111110111111111 // all exceptions get handled by xv6 but Environment calls from S-mode (Allowing SBI)
    .equ        MSTATUS_MPP_S,      (1 << 11) #"Supervisor Mode"

    // --- TrapFrame Offsets (Must match struct TrapFrame in sbi_trap.h) ---
    .equ        TF_REG_OFFSET,      0
    .equ        TF_MSTATUS_OFFSET,  (NREGS * 8)
    .equ        TF_MEPC_OFFSET,     ((NREGS + 1) * 8)
    .equ        TF_SIZE             ((NREGS + 2) * 8) // Total size of the frame
    
// Macro for saving a register to the TrapFrame via current SP
.macro SAVE_REG reg, index
    sd  \reg, ((\index) * 8)(\sp) // Use sp as frame pointer temporarily
.endm

// Macro for loading a register from the TrapFrame via current SP
.macro LOAD_REG reg, index
    ld  \reg, ((\index) * 8)(\sp) // Use sp as frame pointer temporarily
.endm

    .section .text.entry
    .globl _start

_start:

    li      t1, MIDELEG_VAL
    csrw    mideleg, t1

    li      t1, MEDELEG_VAL
    csrw    medeleg, t1

    la      t0, m_trap_vector
    csrw    mtvec, t0

    la      t0, super_entry
    csrw    mepc, t0

    li      t0, MSTATUS_MPP_S
    csrw    mstatus, t0

    # allow both supervisor and user to access all performance counters
    li      t0, -1
    csrw    mcounteren, t0
    csrw    scounteren, t0

    # Disable M-mode interrupts (we delegate S-mode interrupts via mideleg)
    csrw    sie, zero       # S-mode interrupts remain controllable by S-mode
    csrw    mie, zero       # Disable all M-mode interrupts for now

    # clear out .bss segment
    la      t0, bss_start
    la      t1, bss_end
clrloop:
    sd      zero, 0(t0)
    addi    t0, t0, 8
    ble     t0, t1, clrloop

    li      t0, 0x5
    sb      t0, 0(zero)
        
    csrr    a0, mhartid     # Pass hard ID to S-mode
    mret                    # Enter S-mode at super_entry


// --------------------------------------------------------------------
// Machine Mode Trap Handler
// --------------------------------------------------------------------
    .globl m_trap_vector
    .align 2 // Required alignment for trap vectors

m_trap_vector:
    // 1.   Atomically Swap SP with M-mode stack pointer stored in mscratch
    //      Saves original SP (S/U mode) into mscratch for later saving.
    //      SP now points to the dedicated M-mode stack (sbi_stack_top)
    csrrw   sp, mscratch, sp

    // 2.   Allocate space for TrapFrame on the M-mode stack
    addi    sp, sp, -TF_SIZE

    // 3.   Save GPRs (except, x0, x2) and CSRs into the TrapFrame
    //      Save original SP (from mscratch) into frame->regs[2]
    //      Save other registers relative to sp (frame base)
    SAVE_REG    t6, 31  // x31 (t6)
    SAVE_REG    t5, 30  // x30 (t5)
    SAVE_REG    t4, 29  // x29 (t4)
    SAVE_REG    t3, 28  // x28 (t3)
    SAVE_REG    s11, 27 // x27 (s11)
    SAVE_REG    s10, 26 // x26 (s10)
    SAVE_REG    s9, 25  // x25 (s9)
    SAVE_REG    s8, 24  // x24 (s8)
    SAVE_REG    s7, 23  // x23 (s7)
    SAVE_REG    s6, 22  // x22 (s6)
    SAVE_REG    s5, 21  // x21 (s5)
    SAVE_REG    s4, 20  // x20 (s4)
    SAVE_REG    s3, 19  // x19 (s3)
    SAVE_REG    s2, 18  // x18 (s2)
    SAVE_REG    a7, 17  // x17 (a7)
    SAVE_REG    a6, 16  // x16 (a6)
    SAVE_REG    a5, 15  // x15 (a5)
    SAVE_REG    a4, 14  // x14 (a4)
    SAVE_REG    a3, 13  // x13 (a3)
    SAVE_REG    a2, 12  // x12 (a2)
    SAVE_REG    a1, 11  // x11 (a1)
    SAVE_REG    a0, 10  // x10 (a0)
    SAVE_REG    s1, 9   // x9 (s1)
    SAVE_REG    s0, 8   // x8 (s0/fp)
    SAVE_REG    t2, 7   // x7 (t2)
    SAVE_REG    t1, 6   // x6 (t1)
    SAVE_REG    t0, 5   // x5 (t0)
    SAVE_REG    tp, 4   // x4 (tp)
    SAVE_REG    gp, 3   // x3 (gp)
    csrr        t0, mscratch // Get original SP (x2)
    SAVE_REG    t0, 2   // Save original SP (x2)
    SAVE_REG    ra, 1   // x1 (ra)

    // Save relevant CSRs into the frame
    csrr t0, mstatus
    sd   t0, TF_MSTATUS_OFFSET(sp)
    csrr t0, mepc
    sd   t0, TF_MEPC_OFFSET(sp)

    // 4.   Prepare arguments for the C handler
    csrr    a0, mcause  // Arg 0: mcause
    csrr    a1, mtval   // Arg 1: mtval
    mv      a2, sp      // Arg 2: Pointer to TrapFrame

    // 5.   Call the main C trap handler function
    call    machine_trap_handler

    // 6. C handler returns. Restore context from TrapFrame.

    // Restore CSRs first (mepc might have been modified by C handler)
    ld      t0, TF_MSTATUS_OFFSET(sp)
    csrw    mstatus, t0
    ld      t0, TF_MEPC_OFFSET(sp)
    csrw    mepc, t0

    // Restore GPRs
    LOAD_REG    ra, 1   // x1 (ra)
    // Skip sp for now, restore original SP last
    LOAD_REG    gp, 3   // x3 (gp)
    LOAD_REG    tp, 4   // x4 (tp)
    LOAD_REG    t0, 5   // x5 (t0)
    LOAD_REG    t1, 6   // x6 (t1)
    LOAD_REG    t2, 7   // x7 (t2)
    LOAD_REG    s0, 8   // x8 (s0/fp)
    LOAD_REG    s1, 9   // x9 (s1)
    LOAD_REG    a0, 10  // x10 (a0) - SBI return error
    LOAD_REG    a1, 11  // x11 (a1) - SBI return value
    LOAD_REG    a2, 12  // x12 (a2)
    LOAD_REG    a3, 13  // x13 (a3)
    LOAD_REG    a4, 14  // x14 (a4)
    LOAD_REG    a5, 15  // x15 (a5)
    LOAD_REG    a6, 16  // x16 (a6)
    LOAD_REG    a7, 17  // x17 (a7)
    LOAD_REG    s2, 18  // x18 (s2)
    LOAD_REG    s3, 19  // x19 (s3)
    LOAD_REG    s4, 20  // x20 (s4)
    LOAD_REG    s5, 21  // x21 (s5)
    LOAD_REG    s6, 22  // x22 (s6)
    LOAD_REG    s7, 23  // x23 (s7)
    LOAD_REG    s8, 24  // x24 (s8)
    LOAD_REG    s9, 25  // x25 (s9)
    LOAD_REG    s10, 26 // x26 (s10)
    LOAD_REG    s11, 27 // x27 (s11)
    LOAD_REG    t3, 28  // x28 (t3)
    LOAD_REG    t4, 29  // x29 (t4)
    LOAD_REG    t5, 30  // x30 (t5)
    LOAD_REG    t6, 31  // x31 (t6)
    
    // 7.   Deallocate TrapFrame from M-mode stack
    addi    sp, sp, TF_SIZE

    // 8.   Atomically Swap SP back to original S/U mode SP from mscratch
    csrrw   sp, mscratch, sp

    // 9.   Return from trap
    mret

// --------------------------------------------------------------------
// C-Callable Panic Function
// --------------------------------------------------------------------
    .globl panic_from_c
    panic_from_c:
        // just enter an infinite loop
    .panic_loop:
        j   .panic_loop


    # send character in a0
uart.out:	
	push 	s0
	push	s1
uart.out.wait:
	li		s1,	UART_BASE_ADDR
	lb		s0, 2(s1)					# check if ty uart is busy
	bnez 	s0, uart.out.wait
	sb	a0, 0(s1)					# write data in a0 to uart

	pop		s1
	pop		s0
	ret

#a0: reg
#a1: length
uart.printReg:
            addi    sp, sp,-32
            sd      ra, 0(sp)
            sd      s0, 8(sp)
            sd      s1, 16(sp)
            sd      s2, 24(sp)
            mv	    s0, a0
            mv	    s1, a1
            li	    s2, 9
uart.printReg.next:
            addi	s1, s1,-4
            srl	    a0, s0,s1
            andi	a0, a0,0xF
            ble	    a0,s2,uart.printReg.DD
            addi	a0, a0,7
uart.printReg.DD:
            addi	a0, a0,48
            jal	        uart.out
            bnez	s1, uart.printReg.next
            ld      s2, 24(sp)
            ld      s1, 16(sp)
            ld      s0, 8(sp)
            ld      ra, 0(sp)
            addi    sp, sp,32
            ret

super_entry:

	la sp, boot_stack_top   // Initialize S-mode stack pointer

    // Optional: Set thread pointer (tp) if needed by S-mode
    // add by retrhelo, write tp reg 
    // csrr t1, mhartid 
    // mv tp, t1

    // lui t0, %hi(main)
    // addi t0, t0, %lo(main)
    // jr t0
    call main               // Call the main C kernel function

loop:
    j loop

    .section .bss.stack
    .align 12
    .globl boot_stack
boot_stack:
    .space 4096 * 2
    .globl boot_stack_top
boot_stack_top:
    .align 8
    .globl sbi_stack
sbi_stack:
    .space 4096
    .globl sbi_stack_top
sbi_stack_top:



