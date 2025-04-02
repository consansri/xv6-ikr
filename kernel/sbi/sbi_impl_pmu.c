
#include "include/sbi_impl_pmu.h"


/* Global mirror for event configuration registers for counters 0..31.
   Only indices 3..31 are used.
*/
static uint64 event_config[32] = {0};

/* Shared memory snapshot support:
 *
 * We maintain global variables for the physical shared memory address (split into low and high parts)
 * and the snapshot flags. Additionally, our event_config[] array (indices 3..31) acts as our internal
 * mirror of each counter’s event configuration.
 */
static uint64 snapshot_shmem_phys_lo = 0xFFFFFFFFFFFFFFFFUL;
static uint64 snapshot_shmem_phys_hi = 0xFFFFFFFFFFFFFFFFUL;
static uint64 snapshot_flags = 0;


/* Return the number of PMU counters available */
struct SbiRet sbi_pmu_num_counters_impl(void) {
    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = SBI_PMU_COUNTER_NUM;
    return sbiret;
}


/*
 * Return counter information.
 * Bits [11:0]   = CSR number (assumed to be contiguous starting from SBI_PMU_COUNTER_ADDR_U)
 * Bits [17:12]  = Width (one less than the number of bits in the counter)
 * Bit [63]      = Type (0 = hardware, 1 = firmware)
 */
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_get_info_impl\n");
    #endif

    struct SbiRet sbiret;

    if (counter_idx <= 31 && counter_idx >= 3){
        sbiret.error = SBI_SUCCESS;

        // [11:0]       CSR (12bit user CSR number)
        uint64 csr = SBI_PMU_COUNTER_ADDR_U + counter_idx;
        sbiret.value = sbiret.value | csr;  

        // [17:12]      Width (One less than number of bits in CSR)
        uint64 width = SBI_PMU_COUNTER_WIDTH - 1;
        sbiret.value = sbiret.value | (width << 12);   

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

/*
 * sbi_pmu_counter_config_matching:
 *
 * Find and configure a matching PMU counter.
 *
 * Parameters:
 *   counter_idx_base:  Base index for a set of logical counters.
 *   counter_idx_mask:  Bitmask (relative to the base) specifying which counters to consider.
 *   config_flags:      Configuration and filter flags (bits 0-7 used; reserved bits must be zero).
 *   event_idx:         Encodes the event type (upper 4 bits) and event code (lower 16 bits).
 *   event_data:        Additional event data (typically used for firmware or raw events).
 *
 * Returns:
 *   On success, sbiret.error == SBI_SUCCESS and sbiret.value contains the chosen counter index.
 *   On failure, an appropriate error code is returned.
 */
struct SbiRet sbi_pmu_counter_config_matching_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_config_matching_impl\n");
    #endif

    //dump_hpm_state();

    struct SbiRet ret = { .error = SBI_SUCCESS, .value = 0 };

    // 1. Validation

    /* Validate reserved bits in config_flags: only lower 8 bits are allowed */
    if (config_flags & SBI_PMU_CFG_RESERVED_MASK) {
        ret.error = SBI_ERR_INVALID_PARAM;
        ret.value = 0;
        return ret;
    }

    /* Validate that the base index is within range */
    if (counter_idx_base < 3 || counter_idx_base > 31) {
        ret.error = SBI_ERR_INVALID_PARAM;
        ret.value = 0;
        return ret;
    }

    uint64 selected_idx = 0UL;

    // 2. Selection
    uint64 curr_mcountinhibit = read_mcountinhibit();
    for (int bit = 0; bit < 64; bit++) {
        if (counter_idx_mask & (1UL << bit)) {
            uint64 idx = counter_idx_base + bit;
            if (idx < 3 || idx > 31) {
                ret.error = SBI_ERR_INVALID_PARAM;
                break;
            }

            // Skip Matching and assume availability if flag is set
            if ((config_flags & SBI_PMU_CFG_FLAG_SKIP_MATCH) == 0UL) {
            
                // Check availability of counter  
                if (((curr_mcountinhibit >> idx) & 1UL) == 0UL ) {
                    // Counter is currently in use
                    continue;
                }
            }

            // Select counter
            selected_idx = idx;
            break;
        }
    }

    // 3. Configuration
    if (selected_idx != 0UL) {
        ret.value = selected_idx;

        printf("    old config: ");
        dump_hpm(selected_idx);

        write_hw_event(selected_idx, event_idx);

        if ((config_flags & SBI_PMU_CFG_FLAG_CLEAR_VALUE) != 0UL) {
            write_hw_counter(selected_idx, 0UL);
        }

        if ((config_flags & SBI_PMU_CFG_FLAG_AUTO_START) != 0UL) {
            curr_mcountinhibit &= ~(1UL << selected_idx);
            write_mcountinhibit(curr_mcountinhibit);
        }

        printf("    new config (EVT=%x, FLAGS=%x)\n", event_idx, config_flags);
        printf("    new config: ");
        dump_hpm(selected_idx);

    } else {
        ret.error = SBI_ERR_NOT_SUPPORTED;
    }

    //dump_hpm_state();

    return ret;
}

/* Start (or initialize) one or more counters by writing initial_value into the hardware counter.
 * For each counter indicated in counter_idx_mask (starting at counter_idx_base):
 *   (1) Write the initial_value to the hardware counter CSR.
 *   (2) Clear its corresponding bit in mcountinhibit to enable counting.
 */
struct SbiRet sbi_pmu_counter_start_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_start_impl\n");
    #endif

    struct SbiRet ret = { .error = SBI_SUCCESS, .value = 0 };

    if ((start_flags & SBI_PMU_START_RESERVED_MASK) != 0UL) {
        ret.error = SBI_ERR_INVALID_PARAM;
        return ret;
    }

    //dump_hpm_state();

    uint64 curr_mcountinhibit = read_mcountinhibit();

    for (int bit = 0; bit < 64; bit++) {
        if (counter_idx_mask & (1UL << bit)) {
            uint64 idx = counter_idx_base + bit;
            if (idx < 3 || idx > 31) {
                ret.error = SBI_ERR_INVALID_PARAM;
                return ret;
            }

            // Clear inhibit bits to start counters
            curr_mcountinhibit &= ~(1UL << idx);

            if ((start_flags & SBI_PMU_START_SET_INIT_VALUE) != 0UL) { // SBI_PMU_START_SET_INIT_VALUE: set the value of counters based on the initial_value parameter.
                write_hw_counter(idx, initial_value);
            }
        }
    }

    // Start counters
    write_mcountinhibit(curr_mcountinhibit);

    //dump_hpm_state();

    return ret;
}

/* Stop one or more counters.
 * For each counter indicated in counter_idx_mask (starting at counter_idx_base),
 * set its corresponding bit in mcountinhibit to inhibit counting.
 */
struct SbiRet sbi_pmu_counter_stop_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_stop_impl\n");
    #endif

    //dump_hpm_state();

    struct SbiRet ret = { .error = SBI_SUCCESS, .value = 0 };

    if ((stop_flags & SBI_PMU_STOP_RESERVED_MASK) != 0UL) {
        ret.error = SBI_ERR_INVALID_PARAM;
        return ret;
    }

    uint64 curr_mcountinhibit = read_mcountinhibit();

    for (int bit = 0; bit < 64; bit++) {
        if (counter_idx_mask & (1UL << bit)) {
            uint64 idx = counter_idx_base + bit;
            if (idx < 3 || idx > 31) {
                ret.error = SBI_ERR_INVALID_PARAM;
                continue;
            }

            // Set countinhibit bits to stop counters
            curr_mcountinhibit |= (1UL << idx);

            if ((stop_flags & SBI_PMU_STOP_FLAG_RESET) != 0UL) { // SBI_PMU_STOP_FLAG_RESET: reset the counter to event mapping.
                write_hw_event(idx, 0UL);
            }
        }
    }

    write_mcountinhibit(curr_mcountinhibit);

    //dump_hpm_state();
    
    return ret;
}

/*
 * Read the lower XLEN-bits of a PMU counter.
 * The counter_idx must be between 3 and 31.
 */
struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_fw_read_impl\n");
    #endif

    struct SbiRet ret;

    if (counter_idx < 3 || counter_idx > 31) {
        ret.error = SBI_ERR_INVALID_PARAM;
        ret.value = 0;
        return ret;
    }

    ret.error = SBI_SUCCESS;
    ret.value = read_hw_counter(counter_idx);
    return ret;
}

/*
 * Always return zero for rv64 or higher systems.
 */
struct SbiRet sbi_pmu_counter_fw_read_hi_impl(uint64 counter_idx) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_fw_read_hi_impl\n");
    #endif


    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = 0;
    return sbiret;
}



/*
 * Set the shared memory location for PMU snapshots.
 */
struct SbiRet sbi_pmu_snapshot_set_shmem_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_snapshot_set_shmem_impl\n");
    #endif

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

    snapshot_shmem_phys_lo = shmem_phys_lo;
    snapshot_shmem_phys_hi = shmem_phys_hi;
    snapshot_flags = flags;

    return (struct SbiRet) { .error = SBI_SUCCESS, .value = 0 };
}

// Helper functions

/* --- Inline Assembly Helpers for CSR Access --- */
/*
 * Due to RISC-V requirements, the CSR number must be an immediate.
 * We therefore use a switch-case to select the correct CSR based on the counter index.
 */

uint64 read_hw_counter(uint64 idx) {

    #ifdef SBI_PMU_DEBUG
    printf("read_hw_counter(%d)\n", idx);
    #endif

    uint64 value = 0UL;
    switch(idx) {
      case 3:  asm volatile ("csrr %0, mhpmcounter3" : "=r"(value)); break;
      case 4:  asm volatile ("csrr %0, mhpmcounter4" : "=r"(value)); break;
      case 5:  asm volatile ("csrr %0, mhpmcounter5" : "=r"(value)); break;
      case 6:  asm volatile ("csrr %0, mhpmcounter6" : "=r"(value)); break;
      case 7:  asm volatile ("csrr %0, mhpmcounter7" : "=r"(value)); break;
      case 8:  asm volatile ("csrr %0, mhpmcounter8" : "=r"(value)); break;
      case 9:  asm volatile ("csrr %0, mhpmcounter9" : "=r"(value)); break;
      case 10: asm volatile ("csrr %0, mhpmcounter10" : "=r"(value)); break;
      case 11: asm volatile ("csrr %0, mhpmcounter11" : "=r"(value)); break;
      case 12: asm volatile ("csrr %0, mhpmcounter12" : "=r"(value)); break;
      case 13: asm volatile ("csrr %0, mhpmcounter13" : "=r"(value)); break;
      case 14: asm volatile ("csrr %0, mhpmcounter14" : "=r"(value)); break;
      case 15: asm volatile ("csrr %0, mhpmcounter15" : "=r"(value)); break;
      case 16: asm volatile ("csrr %0, mhpmcounter16" : "=r"(value)); break;
      case 17: asm volatile ("csrr %0, mhpmcounter17" : "=r"(value)); break;
      case 18: asm volatile ("csrr %0, mhpmcounter18" : "=r"(value)); break;
      case 19: asm volatile ("csrr %0, mhpmcounter19" : "=r"(value)); break;
      case 20: asm volatile ("csrr %0, mhpmcounter20" : "=r"(value)); break;
      case 21: asm volatile ("csrr %0, mhpmcounter21" : "=r"(value)); break;
      case 22: asm volatile ("csrr %0, mhpmcounter22" : "=r"(value)); break;
      case 23: asm volatile ("csrr %0, mhpmcounter23" : "=r"(value)); break;
      case 24: asm volatile ("csrr %0, mhpmcounter24" : "=r"(value)); break;
      case 25: asm volatile ("csrr %0, mhpmcounter25" : "=r"(value)); break;
      case 26: asm volatile ("csrr %0, mhpmcounter26" : "=r"(value)); break;
      case 27: asm volatile ("csrr %0, mhpmcounter27" : "=r"(value)); break;
      case 28: asm volatile ("csrr %0, mhpmcounter28" : "=r"(value)); break;
      case 29: asm volatile ("csrr %0, mhpmcounter29" : "=r"(value)); break;
      case 30: asm volatile ("csrr %0, mhpmcounter30" : "=r"(value)); break;
      case 31: asm volatile ("csrr %0, mhpmcounter31" : "=r"(value)); break;
      default: break;
    }
    return value;
}

uint64 read_hw_event(uint64 idx) {

    #ifdef SBI_PMU_DEBUG
    printf("read_hw_event(%d)\n", idx);
    #endif

    uint64 value = 0UL;
    switch(idx) {
      case 3:  asm volatile ("csrr %0, mhpmevent3" : "=r"(value)); break;
      case 4:  asm volatile ("csrr %0, mhpmevent4" : "=r"(value)); break;
      case 5:  asm volatile ("csrr %0, mhpmevent5" : "=r"(value)); break;
      case 6:  asm volatile ("csrr %0, mhpmevent6" : "=r"(value)); break;
      case 7:  asm volatile ("csrr %0, mhpmevent7" : "=r"(value)); break;
      case 8:  asm volatile ("csrr %0, mhpmevent8" : "=r"(value)); break;
      case 9:  asm volatile ("csrr %0, mhpmevent9" : "=r"(value)); break;
      case 10: asm volatile ("csrr %0, mhpmevent10" : "=r"(value)); break;
      case 11: asm volatile ("csrr %0, mhpmevent11" : "=r"(value)); break;
      case 12: asm volatile ("csrr %0, mhpmevent12" : "=r"(value)); break;
      case 13: asm volatile ("csrr %0, mhpmevent13" : "=r"(value)); break;
      case 14: asm volatile ("csrr %0, mhpmevent14" : "=r"(value)); break;
      case 15: asm volatile ("csrr %0, mhpmevent15" : "=r"(value)); break;
      case 16: asm volatile ("csrr %0, mhpmevent16" : "=r"(value)); break;
      case 17: asm volatile ("csrr %0, mhpmevent17" : "=r"(value)); break;
      case 18: asm volatile ("csrr %0, mhpmevent18" : "=r"(value)); break;
      case 19: asm volatile ("csrr %0, mhpmevent19" : "=r"(value)); break;
      case 20: asm volatile ("csrr %0, mhpmevent20" : "=r"(value)); break;
      case 21: asm volatile ("csrr %0, mhpmevent21" : "=r"(value)); break;
      case 22: asm volatile ("csrr %0, mhpmevent22" : "=r"(value)); break;
      case 23: asm volatile ("csrr %0, mhpmevent23" : "=r"(value)); break;
      case 24: asm volatile ("csrr %0, mhpmevent24" : "=r"(value)); break;
      case 25: asm volatile ("csrr %0, mhpmevent25" : "=r"(value)); break;
      case 26: asm volatile ("csrr %0, mhpmevent26" : "=r"(value)); break;
      case 27: asm volatile ("csrr %0, mhpmevent27" : "=r"(value)); break;
      case 28: asm volatile ("csrr %0, mhpmevent28" : "=r"(value)); break;
      case 29: asm volatile ("csrr %0, mhpmevent29" : "=r"(value)); break;
      case 30: asm volatile ("csrr %0, mhpmevent30" : "=r"(value)); break;
      case 31: asm volatile ("csrr %0, mhpmevent31" : "=r"(value)); break;
      default: break;
    }
    return value;
}

uint64 read_mcountinhibit(void) {

    #ifdef SBI_PMU_DEBUG
    printf("read_mcountinhibit()\n");
    #endif

    uint64 value;
    asm volatile ("csrr %0, mcountinhibit" : "=r"(value));
    return value;
}

void write_hw_counter(uint64 idx, uint64 value) {

    #ifdef SBI_PMU_DEBUG
    printf("write_hw_counter(%d, %d)\n", idx, value);
    #endif

    switch(idx) {
      case 3:  asm volatile ("csrw mhpmcounter3, %0" :: "r"(value)); break;
      case 4:  asm volatile ("csrw mhpmcounter4, %0" :: "r"(value)); break;
      case 5:  asm volatile ("csrw mhpmcounter5, %0" :: "r"(value)); break;
      case 6:  asm volatile ("csrw mhpmcounter6, %0" :: "r"(value)); break;
      case 7:  asm volatile ("csrw mhpmcounter7, %0" :: "r"(value)); break;
      case 8:  asm volatile ("csrw mhpmcounter8, %0" :: "r"(value)); break;
      case 9:  asm volatile ("csrw mhpmcounter9, %0" :: "r"(value)); break;
      case 10: asm volatile ("csrw mhpmcounter10, %0" :: "r"(value)); break;
      case 11: asm volatile ("csrw mhpmcounter11, %0" :: "r"(value)); break;
      case 12: asm volatile ("csrw mhpmcounter12, %0" :: "r"(value)); break;
      case 13: asm volatile ("csrw mhpmcounter13, %0" :: "r"(value)); break;
      case 14: asm volatile ("csrw mhpmcounter14, %0" :: "r"(value)); break;
      case 15: asm volatile ("csrw mhpmcounter15, %0" :: "r"(value)); break;
      case 16: asm volatile ("csrw mhpmcounter16, %0" :: "r"(value)); break;
      case 17: asm volatile ("csrw mhpmcounter17, %0" :: "r"(value)); break;
      case 18: asm volatile ("csrw mhpmcounter18, %0" :: "r"(value)); break;
      case 19: asm volatile ("csrw mhpmcounter19, %0" :: "r"(value)); break;
      case 20: asm volatile ("csrw mhpmcounter20, %0" :: "r"(value)); break;
      case 21: asm volatile ("csrw mhpmcounter21, %0" :: "r"(value)); break;
      case 22: asm volatile ("csrw mhpmcounter22, %0" :: "r"(value)); break;
      case 23: asm volatile ("csrw mhpmcounter23, %0" :: "r"(value)); break;
      case 24: asm volatile ("csrw mhpmcounter24, %0" :: "r"(value)); break;
      case 25: asm volatile ("csrw mhpmcounter25, %0" :: "r"(value)); break;
      case 26: asm volatile ("csrw mhpmcounter26, %0" :: "r"(value)); break;
      case 27: asm volatile ("csrw mhpmcounter27, %0" :: "r"(value)); break;
      case 28: asm volatile ("csrw mhpmcounter28, %0" :: "r"(value)); break;
      case 29: asm volatile ("csrw mhpmcounter29, %0" :: "r"(value)); break;
      case 30: asm volatile ("csrw mhpmcounter30, %0" :: "r"(value)); break;
      case 31: asm volatile ("csrw mhpmcounter31, %0" :: "r"(value)); break;
      default: break;
    }
}

void write_hw_event(uint64 idx, uint64 value) {

    #ifdef SBI_PMU_DEBUG
    printf("write_hw_event(%d, %x)\n", idx, value);
    #endif

    switch(idx) {
      case 3:  asm volatile ("csrw mhpmevent3, %0" :: "r"(value)); break;
      case 4:  asm volatile ("csrw mhpmevent4, %0" :: "r"(value)); break;
      case 5:  asm volatile ("csrw mhpmevent5, %0" :: "r"(value)); break;
      case 6:  asm volatile ("csrw mhpmevent6, %0" :: "r"(value)); break;
      case 7:  asm volatile ("csrw mhpmevent7, %0" :: "r"(value)); break;
      case 8:  asm volatile ("csrw mhpmevent8, %0" :: "r"(value)); break;
      case 9:  asm volatile ("csrw mhpmevent9, %0" :: "r"(value)); break;
      case 10: asm volatile ("csrw mhpmevent10, %0" :: "r"(value)); break;
      case 11: asm volatile ("csrw mhpmevent11, %0" :: "r"(value)); break;
      case 12: asm volatile ("csrw mhpmevent12, %0" :: "r"(value)); break;
      case 13: asm volatile ("csrw mhpmevent13, %0" :: "r"(value)); break;
      case 14: asm volatile ("csrw mhpmevent14, %0" :: "r"(value)); break;
      case 15: asm volatile ("csrw mhpmevent15, %0" :: "r"(value)); break;
      case 16: asm volatile ("csrw mhpmevent16, %0" :: "r"(value)); break;
      case 17: asm volatile ("csrw mhpmevent17, %0" :: "r"(value)); break;
      case 18: asm volatile ("csrw mhpmevent18, %0" :: "r"(value)); break;
      case 19: asm volatile ("csrw mhpmevent19, %0" :: "r"(value)); break;
      case 20: asm volatile ("csrw mhpmevent20, %0" :: "r"(value)); break;
      case 21: asm volatile ("csrw mhpmevent21, %0" :: "r"(value)); break;
      case 22: asm volatile ("csrw mhpmevent22, %0" :: "r"(value)); break;
      case 23: asm volatile ("csrw mhpmevent23, %0" :: "r"(value)); break;
      case 24: asm volatile ("csrw mhpmevent24, %0" :: "r"(value)); break;
      case 25: asm volatile ("csrw mhpmevent25, %0" :: "r"(value)); break;
      case 26: asm volatile ("csrw mhpmevent26, %0" :: "r"(value)); break;
      case 27: asm volatile ("csrw mhpmevent27, %0" :: "r"(value)); break;
      case 28: asm volatile ("csrw mhpmevent28, %0" :: "r"(value)); break;
      case 29: asm volatile ("csrw mhpmevent29, %0" :: "r"(value)); break;
      case 30: asm volatile ("csrw mhpmevent30, %0" :: "r"(value)); break;
      case 31: asm volatile ("csrw mhpmevent31, %0" :: "r"(value)); break;
      default: break;
    }
}

void write_mcountinhibit(uint64 value) {

    #ifdef SBI_PMU_DEBUG
    printf("write_mcountinhibit(%x)\n", value);
    #endif

    asm volatile ("csrw mcountinhibit, %0" :: "r"(value));
}

void dump_hpm(uint64 counter_idx) {
    #ifdef SBI_PMU_DEBUG
    printf("dump_hpm\n");
    #endif

    uint64 mcountinhibit = read_mcountinhibit();
    uint64 counter_state = read_hw_counter(counter_idx);
    uint64 event_state = read_hw_event(counter_idx);
    
    if (((mcountinhibit >> counter_idx) & 1UL) == 0UL)
        printf("[DUMP] counting %d: [EVT: %x]: %d\n", counter_idx, event_state, counter_state);
    else
        printf("[DUMP]          %d: [EVT: %x]: %d\n", counter_idx, event_state, counter_state);


}

void dump_hpm_state(void) {

    #ifdef SBI_PMU_DEBUG
    printf("dump_hpm_state\n");
    #endif

    uint64 mcountinhibit = read_mcountinhibit();
    uint64 counter_state, event_state;
    for(uint64 i = 3; i < 32; i++) {
        counter_state = read_hw_counter(i);
        event_state = read_hw_event(i);
        if (((mcountinhibit >> i) & 1UL) == 0UL)
            printf("[DUMP] counting %d: [EVT: %x]: %d\n", i, event_state, counter_state);
        else
            printf("[DUMP]          %d: [EVT: %x]: %d\n", i, event_state, counter_state);
    }
}



