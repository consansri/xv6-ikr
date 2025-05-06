#include "kernel/include/types.h"
// Define MAX_PMU_HANDLES. This should ideally match the kernel's definition.
// If your kernel's MAX_PMU_HANDLES is different, change this value.
#define MAX_PMU_HANDLES 8 // Example value

// PMU Actions (mirroring kernel/include/syspmu.h)
#define PMU_ACTION_START        1
#define PMU_ACTION_STOP         2
#define PMU_ACTION_READ         3
#define PMU_ACTION_STOP_READ    4

// Standard RISC-V PMU Event Codes (examples - refer to RISC-V spec for more)
// These are just common examples; specific hardware might support more/different codes.
// MHPMCOUNTER3: Cycle counter (usually)
#define CSR_CYCLE        0xc00  // Matches mcycle
#define EVENT_CODE_CYCLE 1      // A common mapping for generic cycle event for SBI config
                                // Actual event codes for sbi_pmu_counter_config_matching
                                // can be more complex (e.g., (event_selector << TYPE_SHIFT) | event_code)
                                // For simplicity, we'll assume simple event codes if your SBI impl allows.
                                // The provided kernel code uses event_code directly.

// MHPMCOUNTER4: Instruction retired counter (usually)
#define CSR_INSTRET      0xc02  // Matches minstret
#define EVENT_CODE_INSTRET 2    // A common mapping for generic instruction retired event

// User-level function prototypes for PMU system calls (defined in user.h, implemented via usys.S)
// It's generally better to wrap these in more user-friendly functions if needed,
// but these are the direct syscall wrappers.

// For pmu_setup
// config_mask: bitmask of handles to configure
// event_codes: array of event codes, one for each handle
// flags: array of flags, one for each handle
// Returns success_mask: bitmask of successfully configured handles
uint64 pmu_setup(uint64 config_mask, uint64* event_codes, uint64* flags);

// For pmu_control
// action: PMU_ACTION_START, PMU_ACTION_STOP, PMU_ACTION_READ, PMU_ACTION_STOP_READ
// handle_mask: bitmask of handles to control/read
// values_out: user buffer to store read counter values. Should be large enough
//             to hold values for all set bits in handle_mask.
// Returns 0 on success, -1 on failure.
uint64 pmu_control(int action, uint64 handle_mask, uint64* values_out);
