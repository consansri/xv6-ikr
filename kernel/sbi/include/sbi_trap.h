#ifndef __SBI_TRAP
#define __SBI_TRAP

#include "../../include/types.h"
#include "sbi_riscv.h"

// Define architecture register count (usually 32 for RV32/RV64)
#define NREGS           32

/*
 * Structure to hold the saved context during a machine trap.
 * The order MUST match the order registers are saved/restored
 * in entry.S (`m_trap_entry`).
 * We save CSRs like mepc, mstatus, etc., within this frame as well
 * for convenience in the C handler.
 */
struct TrapFrame {
    // General-purpose registers (x0-x31)
    // x0 (zero) is not explicitly saved but space might be kept for alignment/simplicity
    uint64 regs[NREGS]; // corresponds to x0-x31

    // CSRs saved by the trap handler
    uint64 mstatus;
    uint64 mepc;
    // Add mtval if needed universally, otherwise pass as arg to C handler
    // uint64 mtval;
};

// Function prototype for the main C trap handler
// It receives the cause, trap value, and a pointer to the saved frame.
// It can modify the frame (e.g., mepc, return values in regs[10]/regs[11])
void machine_trap_handler(uint64 mcause, uint64 mtval, struct TrapFrame *frame);


// Forward declarations for handlers within this file or others
static void handle_exception(uint64 mcause, uint64 mtval, struct TrapFrame *frame);
static void handle_interrupt(uint64 mcause, struct TrapFrame *frame);

#endif
