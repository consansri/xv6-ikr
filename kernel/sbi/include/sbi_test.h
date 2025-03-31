#ifndef __SBI_TEST__
#define __SBI_TEST__

#include "sbi_call.h"

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

#endif
