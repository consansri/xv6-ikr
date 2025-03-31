#include "include/sbi_dispatcher.h"



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
                    break;
            }
            break;

        case 0x54494D45: // TIMER
            switch(fid) {


                default:
                    #ifdef SBI_DISPATCHER_DEBUG
                    printf("Invalid SBI TIMER Call (eid = 0x%x, fid = %d)\n", eid, fid);
                    #endif
                    break;
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

                default: 
                    #ifdef SBI_DISPATCHER_DEBUG
                    rintf("Invalid SBI PMU Call (eid = 0x%x, fid = %d)\n", eid, fid);
                    #endif
                    break;
            }
            break;


        default:
            #ifdef SBI_DISPATCHER_DEBUG
            printf("Invalid SBI Call (eid = 0x%x, fid = %d)\n", eid, fid);
            #endif
            return sbiret;
    }

    #ifdef SBI_DISPATCHER_DEBUG
    printf("SBI Dispatcher -> Send SbiRet(error = 0x%x, value = 0x%x)\n", sbiret.error, sbiret.value);
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

// ----------------------------------------------------------------- EXTENSIONS

// BASE

struct SbiRet sbi_get_spec_version_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = (SBI_SPEC_VERSION_MAJ << 24) | SBI_SPEC_VERSION_MIN;
    return sbiret;
}

struct SbiRet sbi_get_impl_id_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = SBI_IMPL_ID;
    return sbiret;
}

struct SbiRet sbi_get_impl_version_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = (SBI_IMPL_VERSION_MAJ << 24) | SBI_IMPL_VERSION_MIN;
    return sbiret;
}

struct SbiRet sbi_probe_extension_impl(uint64 extension_id) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    switch(extension_id) {
        case 0x10: // BASE
            sbiret.value = 1;
            return sbiret;

        case 0x504D55: // PMU
            sbiret.value = 1;
            return sbiret;

        default:
            sbiret.value = 0;
            return sbiret;
    }
}

struct SbiRet sbi_get_mvendorid_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    asm volatile(
        "csrr %0, mvendorid"
        :
        : "r" (sbiret.value)
    );

    return sbiret;
}

struct SbiRet sbi_get_marchid_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    asm volatile(
        "csrr %0, marchid"
        :
        : "r" (sbiret.value)
        : "memory"
    );

    return sbiret;
}

struct SbiRet sbi_get_mimpid_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    asm volatile(
        "csrr %0, mimpid"
        :
        : "r" (sbiret.value)
        : 
    );

    return sbiret;
}

// PMU

struct SbiRet sbi_pmu_num_counters_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = SBI_PMU_COUNTER_NUM;
    return sbiret;
}

/*
@return 
    SbiRet.error = if (counter_idx points to an invalid counter) SBI_ERR_INVALID_PARAM else SBI_SUCCESS
    SbiRet.value = counter_info {
        [11:0]      = CSR (12bit CSR number)
        [17:12]     = Width (One less than number of bits in CSR)
        [XLEN-2:18] = Reserved for future use
        [XLEN-1]    = Type (0 = hardware, 1 = firmware)
    }
*/
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx) {
    struct SbiRet sbiret;

    if (counter_idx < SBI_PMU_COUNTER_NUM && counter_idx >= 0){
        sbiret.error = SBI_SUCCESS;

        // [11:0]       CSR (12bit user CSR number)
        sbiret.value = sbiret.value | (SBI_PMU_COUNTER_ADDR_U + counter_idx);  

        // [17:12]      Width (One less than number of bits in CSR)
        sbiret.value = sbiret.value | ((SBI_PMU_COUNTER_WIDTH - 1) << 12);   

        // [XLEN-2:18]  Reserved for future use  
        //sbiret.value = sbiret.value           

        // [XLEN-1]     Type (0 = hardware, 1 = firmware)                                        
        sbiret.value = sbiret.value | (0 << 63); // CURRENTLY ONLY HARDWARE COUNTERS
    } else {
        sbiret.error = SBI_ERR_INVALID_PARAM;
        sbiret.value = 0;
    }
    
    return sbiret;
}

struct SbiRet sbi_pmu_counter_config_matching_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data) {

}

struct SbiRet sbi_pmu_counter_start_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value) {


    if (start_flags & 0b1 == 1) { // SBI_PMU_START_SET_INIT_VALUE: set the value of counters based on the initial_value parameter.

    }

    if ((start_flags >> 1) & 0b1 == 1) { // SBI_PMU_START_FLAG_INIT_SNAPSHOT: Initialize the given counters from shared memory if available.

    }


    for (uint64 counter_index = 0; counter_index < 32; counter_index++) {
        if ((counter_idx_mask >> counter_index) & 1 == 1) {
            switch((counter_idx_base + counter_index) % 32) {
                case 0:
                    asm volatile(
                        "csrw mhpmcounter3, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 1:
                    asm volatile(
                        "csrw mhpmcounter4, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 2:
                    asm volatile(
                        "csrw mhpmcounter5, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 3:
                    asm volatile(
                        "csrw mhpmcounter6, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 4:
                    asm volatile(
                        "csrw mhpmcounter7, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 5:
                    asm volatile(
                        "csrw mhpmcounter8, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 6:
                    asm volatile(
                        "csrw mhpmcounter9, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 7:
                    asm volatile(
                        "csrw mhpmcounter10, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 8:
                    asm volatile(
                        "csrw mhpmcounter11, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 9:
                    asm volatile(
                        "csrw mhpmcounter12, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 10:
                    asm volatile(
                        "csrw mhpmcounter13, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 11:
                    asm volatile(
                        "csrw mhpmcounter14, %0"
                        : "=r" (initial_value)
                    );
                    break;
                
                case 12:
                    asm volatile(
                        "csrw mhpmcounter15, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 13:
                    asm volatile(
                        "csrw mhpmcounter16, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 14:
                    asm volatile(
                        "csrw mhpmcounter17, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 15:
                    asm volatile(
                        "csrw mhpmcounter18, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 16:
                    asm volatile(
                        "csrw mhpmcounter19, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 17:
                    asm volatile(
                        "csrw mhpmcounter20, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 18:
                    asm volatile(
                        "csrw mhpmcounter21, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 19:
                    asm volatile(
                        "csrw mhpmcounter22, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 20:
                    asm volatile(
                        "csrw mhpmcounter23, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 21:
                    asm volatile(
                        "csrw mhpmcounter24, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 22:
                    asm volatile(
                        "csrw mhpmcounter25, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 23:
                    asm volatile(
                        "csrw mhpmcounter26, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 24:
                    asm volatile(
                        "csrw mhpmcounter27, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 25:
                    asm volatile(
                        "csrw mhpmcounter28, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 26:
                    asm volatile(
                        "csrw mhpmcounter29, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 27:    
                    asm volatile(
                        "csrw mhpmcounter30, %0"
                        : "=r" (initial_value)
                    );
                    break;

                case 28:
                    asm volatile(
                        "csrw mhpmcounter31, %0"
                        : "=r" (initial_value)
                    );
                    break;

                default:
                    break;
            }

        }
    }


}

struct SbiRet sbi_pmu_counter_stop_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags) {

    if (stop_flags & 0b1 == 1) { // SBI_PMU_STOP_FLAG_RESET: reset the counter to event mapping.

    }

    if ((stop_flags >> 1) & 0b1 == 1) { // SBI_PMU_STOP_FLAG_TAKE_SNAPSHOT: save a snapshot of the given counter's value in the shared memory if available.

    }

}

struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx) {

}

/*
    @return always zero for rv64 or higher systems.
*/
struct SbiRet sbi_pmu_counter_fw_read_hi_impl(uint64 counter_idx) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = 0;
    return sbiret;
}

struct SbiRet sbi_pmu_snapshot_set_shmem_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    // flags must be zero and shmem_phys_lo must be 4kb aligned in this version!
    if (flags != 0 || (shmem_phys_lo % 4096) != 0) {
        sbiret.error = SBI_ERR_INVALID_PARAM;
        sbiret.value = 0;
        return sbiret;
    }

    // shmem_phys_lo and shmem_phys_hi parameters must be writable or doesn't satisfy other requirements
    if (/* TODO */ 1) {
        sbiret.error = SBI_ERR_INVALID_ADDRESS;
        sbiret.value = 0;
        return sbiret;
    }

}

/*

*/
struct SbiRet sbi_pmu_event_get_info_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 num_entries, uint64 flags) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    // flags must be zero and shmem_phys_lo must be 16-bytes aligned and event_idx doesn't conform with the encodings defined in the specification.
    if (flags != 0 || (shmem_phys_lo % 16) != 0) {
        sbiret.error = SBI_ERR_INVALID_PARAM;
        sbiret.value = 0;
        return sbiret;
    }


}



