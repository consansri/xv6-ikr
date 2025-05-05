#include "include/sbi_trap.h"
#include "include/sbi_impl.h"
#include "include/sbi_call.h"

// Panic Prototype
extern void panic_from_c(void);
#define panic(s) do { printf("PANIC: %s\n", s); panic_from_c(); } while(0)

/*
 * machine_trap_handler(frame, mcause, mtval)
 * 
 * Entry point for all traps into M-mode from S-mode or U-mode.
 * Called by m_trap_vector in entry.S
 * This function deciphers the cause and dispatches to the appropriate handler.
 * It can modify the TrapFrame (e.g., registers, mepc) before returning.
 */
void machine_trap_handler(uint64 mcause, uint64 mtval, struct TrapFrame *frame) {
    // Check if it's an interrupt or an exception
    if (mcause & MCAUSE_INT_BIT) {
        handle_interrupt(mcause, frame);
    } else {
        handle_exception(mcause, mtval, frame);
    }

    // Ensure mepc is properly aligned after potential increment
    frame->mepc = frame->mepc & ~0x1ULL; // Clear lowest bit for potential compressed instructions
}

/* Handles synchronous exceptions */
static void handle_exception(uint64 mcause, uint64 mtval, struct TrapFrame *frame) {
    uint64 cause_code = mcause & 0x7FF; // Extract Exception Code

    switch (cause_code)
    {
    case CAUSE_SUPERVISOR_ECALL:
        struct SbiRet ret = handle_sbi(
            frame->regs[10], // a0
            frame->regs[11], // a1
            frame->regs[12], // a2
            frame->regs[13], // a3
            frame->regs[14], // a4
            frame->regs[15], // a5
            frame->regs[16], // a6
            frame->regs[17]  // a7
        );
        frame->regs[10] = ret.error;
        frame->regs[11] = ret.value;
        frame->mepc += 4; // Skip ecall instruction
        break;

    // --- Firmware Event Exceptions ---
    case CAUSE_ILLEGAL_INSTRUCTION:
        #ifdef SBI_TRAP_DEBUG
        printf("M-Trap: Illegal Instruction @ 0x%lx (mtval=0x%lx)\n", frame->mepc, mtval);
        #endif
        sbi_pmu_fw_illegal_insn(); // Count event
        frame->mepc += 4; // Skip faulting instruction (adjust for compressed if needed)
        break;

    case CAUSE_MISALIGNED_LOAD:
        #ifdef SBI_TRAP_DEBUG
        printf("M-Trap: Misaligned Load @ 0x%lx (mtval=0x%lx)\n", frame->mepc, mtval);
        #endif
        sbi_pmu_fw_misaligned_load();
        frame->mepc += 4; // Skip faulting instruction
        break;

    case CAUSE_LOAD_ACCESS:
        #ifdef SBI_TRAP_DEBUG
        printf("M-Trap: Load Access Fault @ 0x%lx (mtval=0x%lx)\n", frame->mepc, mtval);
        #endif
        sbi_pmu_fw_access_load();
        frame->mepc += 4; // Skip faulting instruction
        break;

    case CAUSE_MISALIGNED_STORE:
        #ifdef SBI_TRAP_DEBUG
        printf("M-Trap: Misaligned Store @ 0x%lx (mtval=0x%lx)\n", frame->mepc, mtval);
        #endif
        sbi_pmu_fw_misaligned_store();
        frame->mepc += 4; // Skip faulting instruction
        break;

    case CAUSE_STORE_ACCESS:
        #ifdef SBI_TRAP_DEBUG
        printf("M-Trap: Store Access Fault @ 0x%lx (mtval=0x%lx)\n", frame->mepc, mtval);
        #endif
        sbi_pmu_fw_access_store();
        frame->mepc += 4; // Skip faulting instruction
        break;
    // --- End Firmware Event Exceptions ---
    
    default:
        printf("Unhandled M-mode Exception:\n");
        printf("  mcause: 0x%lx (Code: %d)\n", mcause, cause_code);
        printf("  mtval:  0x%lx\n", mtval);
        printf("  mepc:   0x%lx\n", frame->mepc);
        printf("  mstatus:0x%lx\n", frame->mstatus);
        panic("Unhandled M-mode exception");
        break;
    }
}

static void handle_interrupt(uint64 mcause, struct TrapFrame *frame) {
    uint64 cause_code = mcause & 0x7FF;

    switch (cause_code) {
        // case IRQ_M_TIMER:
        //     handle_m_timer_interrupt();
        //     break;
        // case IRQ_M_SOFT:
        //     handle_m_soft_interrupt();
        //     break;
        // case IRQ_M_EXT:
        //     handle_m_ext_interrupt();
        //     break;

        default:
            // Interrupt not meant for M-mode or not handled
            // This shouldn't happen if mideleg and mie are set correctly.
             printf("Unexpected M-mode Interrupt:\n");
             printf("  mcause: 0x%lx (Code: %d)\n", mcause, cause_code);
             printf("  mepc:   0x%lx\n", frame->mepc);
            // Ignore or panic? For now, just ignore.
            break;
    }
}
