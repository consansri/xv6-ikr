
#include "include/sbi_impl.h"

/*
 * sbi_init()
 * Main C initialization function for the SBI/M-mode firmware.
 * Called once from entry.S after basic M-mode setup.
 */
void sbi_init() {
    // Initialize SBI subsystems here.

    // 1. Initialize the PMU extension (probes hardware counters)
    sbi_pmu_init();

    printf("SBI Initialization Complete.\n");    
}

/*
 * The handle_sbi function dispatches SBI calls based on the extension id (eid)
 * and function id (fid). Here we support both the BASE and PMU extensions.
 */
struct SbiRet handle_sbi(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 fid, uint64 eid) {

    struct SbiRet sbiret; 
    sbiret.error = -1; // SBI Error: Failed

    #ifdef SBI_DISPATCHER_DEBUG 
        printf("SBI Dispatcher -> handle_sbi(eid = 0x%x, fid = %d)\n", eid, fid);
    #endif

    switch(eid){
        case 0x10: // BASE
            switch(fid) {
                case 0:
                    sbiret = sbi_get_spec_version_impl();
                    break;

                case 1:
                    sbiret = sbi_get_impl_id_impl();
                    break;

                case 2:
                    sbiret = sbi_get_impl_version_impl();
                    break;

                case 3:
                    sbiret = sbi_probe_extension_impl(a0);
                    break;

                case 4:
                    sbiret = sbi_get_mvendorid_impl();
                    break;

                case 5:
                    sbiret = sbi_get_marchid_impl();
                    break;

                case 6:
                    sbiret = sbi_get_mimpid_impl();
                    break;

                default: 
                    #ifdef SBI_DISPATCHER_DEBUG
                        printf("Invalid SBI BASE Call (eid = 0x%x, fid = %d)\n", eid, fid);
                    #endif
                    return sbiret;
            }
            break;

        case 0x54494D45: // TIMER
            switch(fid) {


                default:
                    #ifdef SBI_DISPATCHER_DEBUG
                        printf("Invalid SBI TIMER Call (eid = 0x%x, fid = %d)\n", eid, fid);
                    #endif
                    return sbiret;
            }
            break;

        case 0x504D55: // PMU
            switch(fid) {
                case 0: 
                    sbiret = sbi_pmu_num_counters_impl();
                    break;

                case 1: 
                    sbiret = sbi_pmu_counter_get_info_impl(a0);
                    break;

                case 2:
                    sbiret = sbi_pmu_counter_config_matching_impl(a0, a1, a2, a3, a4);
                    break;

                case 3:
                    sbiret = sbi_pmu_counter_start_impl(a0, a1, a2, a3);
                    break;

                case 4: 
                    sbiret = sbi_pmu_counter_stop_impl(a0, a1, a2);
                    break;

                case 5:
                    sbiret = sbi_pmu_counter_fw_read_impl(a0);
                    break;

                case 6:
                    sbiret = sbi_pmu_counter_fw_read_hi_impl(a0);
                    break;

                case 7:
                    sbiret = sbi_pmu_snapshot_set_shmem_impl(a0, a1, a2);
                    break;

                default: 
                    #ifdef SBI_DISPATCHER_DEBUG
                        printf("Invalid SBI PMU Call (eid = 0x%x, fid = %d)\n", eid, fid);
                    #endif
                    return sbiret;
            }
            break;


        default:
            #ifdef SBI_DISPATCHER_DEBUG
                printf("Invalid SBI Call (eid = 0x%x, fid = %d)\n", eid, fid);
            #endif
            return sbiret;
    }

    #ifdef SBI_DISPATCHER_DEBUG
        printf("SBI Dispatcher -> Send SbiRet(error = %d, value = 0x%x)\n", sbiret.error, sbiret.value);
    #endif

    return sbiret;
}

void print_sbi_info(void){

    uint64 a0_reg, a1_reg, a2_reg, a3_reg, a4_reg, a5_reg, fid_reg, eid_reg;

    asm volatile(
        "mv %0, a0\n"
        "mv %1, a1\n"
        "mv %2, a2\n"
        "mv %3, a3\n"
        "mv %4, a4\n"
        "mv %5, a5\n"
        "mv %6, a6\n"
        "mv %7, a7\n"
        : "=r" (a0_reg), "=r" (a1_reg), "=r" (a2_reg), "=r" (a3_reg), "=r" (a4_reg), "=r" (a5_reg), "=r" (fid_reg), "=r" (eid_reg)
        : 
        : "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
    );

    printf(
        "SBI Dump (HEX)\n"
        "   EID: %x\n"
        "   FID: %x\n"
        "    a0: %x\n"
        "    a1: %x\n"
        "    a2: %x\n"
        "    a3: %x\n"
        "    a4: %x\n"
        "    a5: %x\n"
        , eid_reg, fid_reg, a0_reg, a1_reg, a2_reg, a3_reg, a4_reg, a5_reg
    );

}




