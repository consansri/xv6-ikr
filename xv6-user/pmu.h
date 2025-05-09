#include "kernel/include/types.h"
#include "kernel/sbi/include/sbi_impl_pmu.h"
// Define MAX_PMU_HANDLES. This should ideally match the kernel's definition.
// If your kernel's MAX_PMU_HANDLES is different, change this value.
#define MAX_PMU_HANDLES 8 // Example value

// PMU Actions (mirroring kernel/include/syspmu.h)
#define PMU_ACTION_START        1
#define PMU_ACTION_STOP         2
#define PMU_ACTION_READ         3
#define PMU_ACTION_STOP_READ    4

// User-level function prototypes for PMU system calls (defined in user.h, implemented via usys.S)

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
