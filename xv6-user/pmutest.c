#include "kernel/include/types.h"
#include "xv6-user/user.h"
#include "xv6-user/pmu.h"

// Helper to count set bits (number of counters requested/active)
int count_set_bits(unsigned long mask) {
    int count = 0;
    while (mask > 0) {
        mask &= (mask - 1);
        count++;
    }
    return count;
}

void busy_loop(int iterations) {
    volatile int i, j;
    for (i = 0; i < iterations; ++i) {
        for (j = 0; j < 10000; ++j) {
            // Just consume CPU cycles
        }
    }
}

int main(void) {
    unsigned long event_codes[MAX_PMU_HANDLES];
    unsigned long flags[MAX_PMU_HANDLES];
    unsigned long values[MAX_PMU_HANDLES]; // For reading counter values

    unsigned long config_mask = 0;
    unsigned long success_mask = 0;
    long setup_ret;
    int ctl_ret;

    printf("PMU Test Program Starting...\n");

    // --- Test 1: Configure two counters: Cycles and Instructions Retired ---
    printf("Attempting to configure two counters (Cycles and Instructions)...\n");

    // Configure Handle 0 for Cycles
    config_mask |= (1L << 0); // Request handle 0
    event_codes[0] = SBI_PMU_HW_CPU_CYCLES; // Defined in pmu.h, e.g., as 1
    flags[0] = 0; // Standard flags, kernel might define specifics (e.g., user/kernel mode)

    // Configure Handle 1 for Instructions Retired
    config_mask |= (1L << 1); // Request handle 1
    event_codes[1] = SBI_PMU_HW_INSTRUCTIONS; // Defined in pmu.h, e.g., as 2
    flags[1] = 0; // Standard flags

    setup_ret = pmu_setup(config_mask, event_codes, flags);

    if (setup_ret < 0) {
        printf("pmu_setup failed with error code: %d\n", (int)setup_ret);
        exit(-1);
    }
    success_mask = (unsigned long)setup_ret;

    printf("pmu_setup success_mask: 0x%x\n", success_mask);

    if (success_mask == 0) {
        printf("No PMU counters could be configured. PMU might be unavailable or no counters free.\n");
        // It's possible that the hardware has 0 counters, or SBI reports 0.
        // The kernel's sys_pmu_setup returns 0 if num_physical_counters <= 0.
        // We should exit gracefully if this is the case.
        printf("Exiting pmutest.\n");
        exit(0);
    }
    
    if ((success_mask & (1L << 0)) == 0) {
        printf("Failed to configure Handle 0 (Cycles)\n");
    }
    if ((success_mask & (1L << 1)) == 0) {
        printf("Failed to configure Handle 1 (Instructions)\n");
    }
    if (success_mask != config_mask) {
        printf("Warning: Not all requested counters were configured successfully.\n");
        // For this test, we proceed only if both were successful for simplicity
        if (success_mask != ((1L << 0) | (1L << 1))) {
            printf("Exiting due to partial configuration.\n");
             // Attempt to clear any partially successful configuration
            pmu_setup(0, 0, 0); // Clear all configurations
            exit(-1);
        }
    }


    // --- Test 2: Start, run workload, stop, and read counters ---
    if (success_mask != 0) {
        printf("Starting counters with mask: 0x%x\n", success_mask);
        ctl_ret = pmu_control(PMU_ACTION_START, success_mask, 0); // No values_out for START
        if (ctl_ret != 0) {
            printf("pmu_control START failed!\n");
            pmu_setup(0, 0, 0); // Clear configuration
            exit(-1);
        }

        printf("Running a busy loop...\n");
        busy_loop(100); // Perform some work

        printf("Stopping and reading counters with mask: 0x%x\n", success_mask);
        // Prepare buffer for reading values. The kernel expects values_out to be
        // an array large enough for the number of set bits in success_mask.
        ctl_ret = pmu_control(PMU_ACTION_STOP_READ, success_mask, values);
        if (ctl_ret != 0) {
            printf("pmu_control STOP_READ failed!\n");
            pmu_setup(0, 0, 0); // Clear configuration
            exit(-1);
        }

        printf("Counter values:\n");
        int read_idx = 0;
        if (success_mask & (1L << 0)) {
            printf("  Cycles (Handle 0): %d\n", values[read_idx++]);
        }
        if (success_mask & (1L << 1)) {
            printf("  Instructions (Handle 1): %d\n", values[read_idx++]);
        }
        // Add more handles if configured
    }


    // --- Test 3: Read again (should be same or slightly more if not perfectly stopped) ---
    // Note: Some PMUs might clear on read, or stop might clear.
    // The current kernel sbi_pmu_counter_stop does not reset by default.
    // sbi_pmu_fw_read also does not inherently reset.
    // sbi_pmu_counter_start is called with reset flag.
    if (success_mask != 0) {
        printf("Reading counters again (action PMU_ACTION_READ) with mask: 0x%x\n", success_mask);
        ctl_ret = pmu_control(PMU_ACTION_READ, success_mask, values);
        if (ctl_ret != 0) {
            printf("pmu_control READ failed!\n");
        } else {
            int read_idx = 0;
            if (success_mask & (1L << 0)) {
                printf("  Cycles (Handle 0) after 2nd read: %d\n", values[read_idx++]);
            }
            if (success_mask & (1L << 1)) {
                printf("  Instructions (Handle 1) after 2nd read: %d\n", values[read_idx++]);
            }
        }
    }

    // --- Test 4: Clear PMU configuration ---
    printf("Clearing PMU configuration...\n");
    setup_ret = pmu_setup(0, 0, 0); // config_mask = 0 to clear
    if (setup_ret != 0) { // Expect 0 for success_mask on clear
        printf("pmu_setup (clear) failed or returned non-zero success_mask: %d\n", (int)setup_ret);
    } else {
        printf("PMU configuration cleared.\n");
    }

    printf("PMU Test Program Finished.\n");
    exit(0);
}
