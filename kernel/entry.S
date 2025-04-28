    #include "include/memlayout.h"

   .equ        UART_BASE_ADDR,     0x8000000000000001
   .equ        MIDELEG_VAL,        0b1000001000100000 //  s-timer, s-external
   //.equ        MEDELEG_VAL,        0b1011000100000000 // all page faults and ecall from user mode go to xv6
   //.equ        MEDELEG_VAL,        0b1111111111111111 // all exceptions get handled by xv6
   .equ        MEDELEG_VAL,        0b1111110111111111 // all exceptions get handled by xv6 but Environment calls from S-mode (Allowing SBI)
   
   .equ        MSTATUS_MPP_S,      (1 << 11) #"Supervisor Mode"


.macro push reg
	addi sp, sp, -8
	sd \reg, 0(sp)
.endm

.macro pop reg
	ld \reg, 0(sp)
	addi sp, sp, 8
.endm


    .section .text.entry
    .globl _start

    
_start:

    li      t1, MIDELEG_VAL
    csrw    mideleg, t1

    li      t1, MEDELEG_VAL
    csrw    medeleg, t1

    la      t0, mexc_handle
    csrw    mtvec, t0

    la      t0, super_entry
    csrw    mepc, t0

    li      t0, MSTATUS_MPP_S
    csrw    mstatus, t0

    # allow both supervisor and user to access all performance counters
    li      t0, -1
    csrw    mcounteren, t0
    csrw    scounteren, t0

    csrw    sie, zero
    csrw    mie, zero

    # clear out .bss segment
    la      t0, bss_start
    la      t1, bss_end
clrloop:
    sd      zero, 0(t0)
    addi    t0, t0, 8
    ble     t0, t1, clrloop

    li      t0, 0x5
    sb      t0, 0(zero)
    
    
    csrr    a0, mhartid
    mret


mexc_handle:
    csrr    t0, mcause
    andi    t0, t0, 255
    addi    t0, t0, -11
    beq     t0, zero, restret

    # Skips SBI if cause is not Environment call from s-mode
    csrr    t0, mcause
    andi    t0, t0, (1 << 9)    // Check if Exception 9 (Environment call from S-mode)
    beq     t1, zero, .skip_sbi // Only handle calls from supervisor with "handle_sbi" not everything!

    # Uses mscratch for stack switch
.sbi_create_env:

    ##################################################### switch from SUPERVISOR-STACK to SBI-STACK

    # Switch Supervisor to SBI Stack
    csrw    mscratch, sp
    la      sp, sbi_stack_top

    # Save Callee Save Registers
    sd      ra, -8(sp)
    sd      s0, -16(sp)
    sd      s1, -24(sp)
    sd      s2, -32(sp)
    sd      s3, -40(sp)
    sd      s4, -48(sp)
    sd      s5, -56(sp)
    sd      s6, -64(sp)
    sd      s7, -72(sp)
    sd      s8, -80(sp)
    sd      s9, -88(sp)
    sd      s10, -96(sp)
    sd      s11, -104(sp)

    # Save Supervisor Stack pointer
    csrr    s0, mscratch
    sd      s0, -112(sp)

    addi    sp, sp, -112

    #####################################################

    jal     handle_sbi  # Handle SBI Calls

    ##################################################### switch from SBI-STACK to SUPERVISOR-STACK

    addi    sp, sp, 112

    ld      ra, -8(sp)
    ld      s0, -16(sp)
    ld      s1, -24(sp)
    ld      s2, -32(sp)
    ld      s3, -40(sp)
    ld      s4, -48(sp)
    ld      s5, -56(sp)
    ld      s6, -64(sp)
    ld      s7, -72(sp)
    ld      s8, -80(sp)
    ld      s9, -88(sp)
    ld      s10, -96(sp)
    ld      s11, -104(sp)
 
    # Switch SBI Stack to Supervisor Stack
    ld      sp, -112(sp)

    #####################################################

    # Increment mepc return address by 4 (won't be done automatically for ecalls from supervisor) (uses mscratch)
    csrrw   t0, mscratch, t0
    csrr    t0, mepc
    addi    t0, t0, 4
    csrw    mepc, t0
    csrrw   t0, mscratch, t0

    # Return to supervisor
    mret

.skip_sbi:

    mv      s0, sp
    la      sp, boot_stack_top
    
    li      a0, 10
    jal     uart.out
    li      a0, 13
    jal     uart.out
    li      a0, 'e'
    jal     uart.out
    li      a0, 'x'
    jal     uart.out
    li      a0, 'c'
    jal     uart.out
    li      a0, ':'
    jal     uart.out

    csrr    a0, mcause
    li      a1, 8
    jal uart.printReg

    li      a0, '@'
    jal     uart.out

    csrr    a0, mepc
    li      a1, 64
    jal uart.printReg

    li      a0, 10
    jal     uart.out
    li      a0, 13
    jal     uart.out

    li      a0, 's'
    jal     uart.out
    li      a0, 'p'
    jal     uart.out

    li      a0, '@'
    jal     uart.out

    mv      a0, s0
    li      a1, 64
    jal uart.printReg

doom: 
    j       doom


restret:
    csrr    t0, mscratch
    mret

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

	la sp, boot_stack_top

    // add by retrhelo, write tp reg 
    // csrr t1, mhartid 
    // mv tp, t1

    // lui t0, %hi(main)
    // addi t0, t0, %lo(main)
    // jr t0
    call main

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



