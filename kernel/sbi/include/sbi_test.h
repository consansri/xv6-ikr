#ifndef __SBI_TEST__
#define __SBI_TEST__

#include "sbi_call.h"
#include "sbi_impl_pmu.h"

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

void sbi_pmu_test(void) {
    printf("------------------------- SBI PMU Test -------------------------\n");

    /* Test configuration of counter 3:
     * We use counter index base = 3 and a mask with bit 0 set (so only counter 3 is affected).
     * For our test, we use sample values for config_flags, event_idx, and event_data.
     */
    uint64 counter_base = 3;
    uint64 counter_mask = 1ULL; // only bit0 => counter 3
    uint64 config_flags = 0b11;      // Sample flags
    uint64 event_idx    = 0x1;       // Sample event index
    uint64 event_data   = 0x1;    // Sample event data
    struct SbiRet ret;

    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask,
                                          config_flags, event_idx, event_data);
    if(ret.error != SBI_SUCCESS) {
        printf("PMU configuration matching failed: error %d\n", ret.error);
    } else {
        uint64 combined = (event_data << 32) | (config_flags << 16) | (event_idx & 0xFFFF);
        printf("Configured PMU counter 0x%x: event config = 0x%x\n", counter_base, combined);
    }

    /* Start counter 3 with an initial value of 0.
     * This should clear the corresponding inhibit bit in mcountinhibit.
     */
    uint64 initial_value = 0;
    ret = sbi_pmu_counter_start(counter_base, counter_mask, 0, initial_value);
    if(ret.error != SBI_SUCCESS) {
        printf("PMU counter start failed: error %d\n", ret.error);
    } else {
        printf("Started PMU counter 0x%x with initial value %d\n", counter_base, initial_value);
    }

    /* Simulate some workload or delay */
    for (volatile int i = 0; i < 1000000; i++);

    /* Read the counter value (lower 32 bits) for counter 3 */
    ret = sbi_pmu_counter_fw_read(counter_base);
    if(ret.error != SBI_SUCCESS) {
        printf("PMU counter read failed: error %d\n", ret.error);
    } else {
        printf("Read PMU counter 0x%x value (lower 32 bits): %d\n", counter_base, ret.value);
    }

    /* Stop counter 3: This sets the corresponding bit in mcountinhibit */
    ret = sbi_pmu_counter_stop(counter_base, counter_mask, 0);
    if(ret.error != SBI_SUCCESS) {
        printf("PMU counter stop failed: error %d\n", ret.error);
    } else {
        printf("Stopped PMU counter 0x%x\n", counter_base);
    }

    /* Setup shared memory for PMU snapshot.
     * For testing, we use a static local array.
     * In a real system, the physical address of shared memory would be provided by the platform.
     */
    static uint64 snapshot_shmem[32] = {0}; // Enough to hold event info for counters 3..31
    uintptr_t shmem_phys = (uintptr_t) snapshot_shmem; // For simulation, assume physical == virtual

    ret = sbi_pmu_snapshot_set_shmem((uint64)(shmem_phys & 0xffffffffffffffffULL),
                                       (uint64)(shmem_phys >> 64),
                                       SBI_PMU_SNAPSHOT_FLAG_CLEAR);
    if(ret.error != SBI_SUCCESS) {
        printf("Setting PMU snapshot shared memory failed: error %d\n", ret.error);
    } else {
        printf("PMU snapshot shared memory set at address: 0x%x\n", shmem_phys);
    }

    printf("------------------------- SBI PMU Test -------------------------\n");
}

#endif
