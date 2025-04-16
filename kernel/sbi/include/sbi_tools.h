#ifndef __SBI_TOOLS
#define __SBI_TOOLS

#include "sbi_call.h"
#include "sbi_tools_pmu.h"
#include <stdbool.h>


void sbi_info(void) {
    printf("--------------------------- SBI Info ---------------------------\n");
    
    printf("\n");
    
    struct SbiRet spec_version = sbi_get_spec_version();
    printf("    spec-version:       v%d.%d\n", spec_version.value >> 24, spec_version.value & 0b111111111111111111111111);

    struct SbiRet impl_id = sbi_get_impl_id();
    struct SbiRet impl_version = sbi_get_impl_version();
    printf("    implementation:     %d(v%d.%d)\n", impl_id.value, impl_version.value >> 24, impl_version.value & 0b111111111111111111111111);

    // struct SbiRet mvendorid = sbi_get_mvendorid();
    // printf("    mvendorid:          %d\n", mvendorid.value);

    // struct SbiRet marchid = sbi_get_marchid();
    // printf("    marchid:            %d\n", marchid.value);

    // struct SbiRet mimpid = sbi_get_mimpid();
    // printf("    mimpid:             %d\n\n", mimpid.value);
    
    printf("    extensions:\n");
    // BASE
    struct SbiRet proberet = sbi_probe_extension(0x10);
    printf("        [%d]: base\n", proberet.value);

    // TIMER
    proberet = sbi_probe_extension(0x54494D45);
    printf("        [%d]: timer (TIME)\n", proberet.value);

    // IPI
    proberet = sbi_probe_extension(0x735049);
    printf("        [%d]: ipi (sPI: s-mode IPI)\n", proberet.value);

    // RFENCE
    proberet = sbi_probe_extension(0x52464E43);
    printf("        [%d]: rfence (RFNC)\n", proberet.value);

    // Hart State Management
    proberet = sbi_probe_extension(0x48534D);
    printf("        [%d]: hart state management (HSM)\n");

    // System Reset
    proberet = sbi_probe_extension(0x53525354);
    printf("        [%d]: system reset (SRST)\n");

    // Performance Monitoring Unit
    proberet = sbi_probe_extension(0x504D55);
    printf("        [%d]: performance monitoring unit (PMU)\n");

    // Debug Console
    proberet = sbi_probe_extension(0x4442434E);
    printf("        [%d]: debug console (DBCN)\n");

    // System Suspend
    proberet = sbi_probe_extension(0x53555350);
    printf("        [%d]: system suspend (SUSP)\n");

    // CPPC
    proberet = sbi_probe_extension(0x43505043);
    printf("        [%d]: collaborative processor performance control (CPPC)\n");

    // Nested Acceleration
    proberet = sbi_probe_extension(0x4E41434C);
    printf("        [%d]: nested acceleration (NACL)\n");

    // Steal-time Accounting
    proberet = sbi_probe_extension(0x535441);
    printf("        [%d]: steal-time accounting (STA)\n");

    // Supervisor Software Events
    proberet = sbi_probe_extension(0x535345);
    printf("        [%d]: supervisor software events (SSE)\n");

    // SBI Firmware Features
    proberet = sbi_probe_extension(0x46574654);
    printf("        [%d]: sbi firmware features (FWFT)\n");

    // Debug Triggers
    proberet = sbi_probe_extension(0x44425452);
    printf("        [%d]: debug triggers (DBTR)\n");

    // Message Proxy
    proberet = sbi_probe_extension(0x4D505859);
    printf("        [%d]: message proxy (MPXY)\n");

    printf("\n");

    printf("--------------------------- SBI Info ---------------------------\n");

}

/* Test function for SBI PMU implementation */
void sbi_pmu_test_suite(void) {
    struct SbiRet ret;

    printf("==========================================================\n");
    printf("                    SBI PMU TEST SUITE                    \n");
    printf("==========================================================\n");

    printf("\n");

    /* 1. Get the total number of PMU counters */
    ret = sbi_pmu_num_counters();
    uint64 num_counters = ret.value;
    if (ret.error == SBI_SUCCESS)
        printf("[NUM COUNTERS] Available PMU counters: %d\n", ret.value);
    else
        printf("[NUM COUNTERS] Error: %d\n", ret.error);

    /* 2. Get counter information for a valid counter (counter 3) */
    ret = sbi_pmu_counter_get_info(3);
    if (ret.error == SBI_SUCCESS)
        printf("[COUNTER INFO] Counter 3 Info: 0x%x\n", ret.value);
    else
        printf("[COUNTER INFO] Error reading counter 3 info: %d\n", ret.error);

    /* 3. Test configuring a counter for a "CPU cycles" event (event code 0x0001)
     * Use base=3 and mask=0x1 (only counter 3) with AUTO_START and CLEAR_VALUE flags.
     */
    uint64 config_flags = SBI_PMU_CFG_FLAG_AUTO_START | SBI_PMU_CFG_FLAG_CLEAR_VALUE;
    uint64 event_idx = 0x0001; // Example: CPU cycles event
    uint64 event_data = 0;     // Not used for this event
    ret = sbi_pmu_counter_config_matching(3, 0x1, config_flags, event_idx, event_data);
    if (ret.error == SBI_SUCCESS)
        printf("[CONFIG MATCH] Counter %d configured for event 0x%x (CPU cycles)\n", ret.value, event_idx);
    else
        printf("[CONFIG MATCH] Failed to configure counter for event 0x%x, error: %d\n", event_idx, ret.error);

    /* 4. Test configuring another event using the SKIP_MATCH flag.
     * For instance, an "Instructions" event (event code 0x0002) using base=3 and mask=0x2 (selects counter 4).
     */
    config_flags = SBI_PMU_CFG_FLAG_SKIP_MATCH | SBI_PMU_CFG_FLAG_AUTO_START;
    event_idx = 0x0002; // Example: Instructions event
    ret = sbi_pmu_counter_config_matching(3, 0x2, config_flags, event_idx, event_data);
    if (ret.error == SBI_SUCCESS)
        printf("[CONFIG MATCH] (Skip Match) Counter %d configured for event 0x%x (Instructions)\n", ret.value, event_idx);
    else
        printf("[CONFIG MATCH] (Skip Match) Failed to configure counter for event 0x%x, error: %d\n", event_idx, ret.error);

    /* 5. Test starting a counter explicitly.
     * Let's assume we want to start counter 5 (base 3 with mask 0x4 selects counter 3+2=5)
     * with an initial value (e.g., 0x100).
     */
    uint64 start_flags = SBI_PMU_START_SET_INIT_VALUE;
    uint64 initial_value = 0x100;
    ret = sbi_pmu_counter_start(3, 0x4, start_flags, initial_value);
    if (ret.error == SBI_SUCCESS)
        printf("[START] Started counters with mask 0x%x and initial value 0x%x\n", 0x4, initial_value);
    else
        printf("[START] Failed to start counters, error: %d\n", ret.error);

    /* 6. Test stopping a counter.
     * Stop the same counter (counter 5) by setting its corresponding bit in mcountinhibit.
     */
    uint64 stop_flags = 0; // No reset flag set in this test.
    ret = sbi_pmu_counter_stop(3, 0x4, stop_flags);
    if (ret.error == SBI_SUCCESS)
        printf("[STOP] Successfully stopped counters with mask 0x%x\n", 0x4);
    else
        printf("[STOP] Failed to stop counters, error: %d\n", ret.error);

    /* 7. Test reading the lower 64 bits (or XLEN bits) of a firmware counter.
     * For example, read counter 3.
     */
    ret = sbi_pmu_counter_fw_read(3);
    if (ret.error == SBI_SUCCESS)
        printf("[READ] Firmware counter 3 value: 0x%x\n", ret.value);
    else
        printf("[READ] Failed to read firmware counter 3, error: %d\n", ret.error);


    printf("\n");

    printf("==========================================================\n");
    printf("                  SBI PMU TEST COMPLETED                  \n");
    printf("==========================================================\n");
}

void sbi_pmu_test(void) {
    printf("------------------------- SBI PMU Test -------------------------\n");

    printf("\n");

    /* Test configuration of counter 3:
     * We use counter index base = 3 and a mask with bit 0 set (so only counter 3 is affected).
     * For our test, we use sample values for config_flags, event_idx, and event_data.
     */
    uint64 counter_base = 3;
    uint64 counter_mask = 1UL;      // only bit0 => counter 3
    uint64 config_flags = 0b11;     // Sample flags
    uint64 event_idx    = 0x1;      // Sample event index
    uint64 event_data   = 0x0;      // Sample event data
    struct SbiRet ret;

    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask,
                                          config_flags, event_idx, event_data);
    if (ret.error != SBI_SUCCESS) {
        printf("    PMU configuration matching failed: error %d\n", ret.error);
    } else {
        uint64 combined = (event_data << 32) | (config_flags << 16) | (event_idx & 0xFFFF);
        printf("    Configured PMU counter 0x%x: event config = 0x%x\n", counter_base, combined);
    }

    /* Start counter 3 with an initial value of 0.
     * This should clear the corresponding inhibit bit in mcountinhibit.
     */
    uint64 initial_value = 0;
    ret = sbi_pmu_counter_start(counter_base, counter_mask, 0, initial_value);
    if(ret.error != SBI_SUCCESS) {
        printf("    PMU counter start failed: error %d\n", ret.error);
    } else {
        printf("    Started PMU counter 0x%x with initial value %d\n", counter_base, initial_value);
    }

    /* Simulate some workload or delay */
    for (volatile int i = 0; i < 1000000; i++);

    /* Read the counter value (lower 32 bits) for counter 3 */
    ret = sbi_pmu_counter_fw_read(counter_base);
    if (ret.error != SBI_SUCCESS) {
        printf("    PMU counter read failed: error %d\n", ret.error);
    } else {
        printf("    Read PMU counter 0x%x value: %d\n", counter_base, ret.value);
    }

    /* Stop counter 3: This sets the corresponding bit in mcountinhibit */
    ret = sbi_pmu_counter_stop(counter_base, counter_mask, 0);
    if(ret.error != SBI_SUCCESS) {
        printf("    PMU counter stop failed: error %d\n", ret.error);
    } else {
        printf("    Stopped PMU counter 0x%x\n", counter_base);
    }

    /* Setup shared memory for PMU snapshot.
     * For testing, we use a static local array.
     * In a real system, the physical address of shared memory would be provided by the platform.
     */
    static uint64 snapshot_shmem[32] = {0}; // Enough to hold event info for counters 3..31
    uintptr_t shmem_phys = (uintptr_t) snapshot_shmem; // For simulation, assume physical == virtual

    ret = sbi_pmu_snapshot_set_shmem((uint64)(shmem_phys & 0xffffffffffffffffULL),
                                       (uint64)(shmem_phys >> 64),
                                       0UL);
    if(ret.error != SBI_SUCCESS) {
        printf("    Setting PMU snapshot shared memory failed: error %d\n", ret.error);
    } else {
        printf("    PMU snapshot shared memory set at address: 0x%x\n", shmem_phys);
    }

    printf("\n");

    printf("------------------------- SBI PMU Test -------------------------\n");
}


void sbi_test_pmu(void) {
    // Stop all counters
    sbi_pmu_counter_stop(0UL, 0x1FFFFFFFFFFFFFFFUL, SBI_PMU_STOP_FLAG_RESET);
    
    // Start PMU Counter Test
    struct CounterSet set = sbi_pmu_test_start_counter(SBI_PMU_HW_CPU_CYCLES, SBI_PMU_HW_CACHE_MISSES, SBI_PMU_HW_BRANCH_INSTRUCTIONS, SBI_PMU_HW_BRANCH_MISSES);
    
    if (!set.valid) {
        return;
    }

    // Execute some programs to test
    sbi_pmu_test_suite();
    //sbi_pmu_test();

    // Stop PMU Counter test
    sbi_pmu_test_stop_counter(set);
}


#endif
