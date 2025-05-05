#ifndef __SBI_RISCV_H
#define __SBI_RISCV_H

// Standard RISC-V Machine Cause Codes (mcause)
// Exceptions
#define CAUSE_MISALIGNED_FETCH          0
#define CAUSE_FETCH_ACCESS              1
#define CAUSE_ILLEGAL_INSTRUCTION       2
#define CAUSE_BREAKPOINT                3
#define CAUSE_MISALIGNED_LOAD           4
#define CAUSE_LOAD_ACCESS               5
#define CAUSE_MISALIGNED_STORE          6
#define CAUSE_STORE_ACCESS              7 // Also Store/AMO access fault
#define CAUSE_USER_ECALL                8
#define CAUSE_SUPERVISOR_ECALL          9
#define CAUSE_VIRTUAL_SUPERVISOR_ECALL  10
#define CAUSE_MACHINE_ECALL             11
#define CAUSE_FETCH_PAGE_FAULT          12
#define CAUSE_LOAD_PAGE_FAULT           13
// Reserved                             14
#define CAUSE_STORE_PAGE_FAULT          15 // Also Store/AMO page fault
// Reserved                             16-23
#define CAUSE_FETCH_GUEST_PAGE_FAULT    24
#define CAUSE_LOAD_GUEST_PAGE_FAULT     25
#define CAUSE_VIRTUAL_INSTRUCTION       26
#define CAUSE_STORE_GUEST_PAGE_FAULT    27

// Add Interrupts if needed later (usually defined with interrupt bit set)
// #define IRQ_S_SOFT                      (1 | (1UL << 63))
// #define IRQ_M_SOFT                      (3 | (1UL << 63))
// #define IRQ_S_TIMER                     (5 | (1UL << 63))
// #define IRQ_M_TIMER                     (7 | (1UL << 63))
// #define IRQ_S_EXT                       (9 | (1UL << 63))
// #define IRQ_M_EXT                       (11 | (1UL << 63))

// MCAUSE Interrupt Bit (for RV64)
#define MCAUSE_INT_BIT                  (1UL << 63)

// Other useful definitions (example)
// MSTATUS register fields
#define MSTATUS_MPP_MASK  (3L << 11) // Previous privilege mode mask
#define MSTATUS_MPP_M     (3L << 11)
#define MSTATUS_MPP_S     (1L << 11)
#define MSTATUS_MPP_U     (0L << 11)
#define MSTATUS_MIE       (1L << 3)  // Machine Interrupt Enable
#define MSTATUS_SIE       (1L << 1)  // Supervisor Interrupt Enable
#define MSTATUS_UIE       (1L << 0)  // User Interrupt Enable

// Add other CSR definitions as needed (mepc, mtvec, medeleg etc.)

#endif // __RISCV_H