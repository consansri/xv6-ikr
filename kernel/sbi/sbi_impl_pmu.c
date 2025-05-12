
#include "include/sbi_impl_pmu.h"

// --- Global State for PMU ---

// Number of *actually implemented* hardware counters (mhpmcounter3 onwards).
// Determined by sbi_pmu_init() at boot time.
static volatile uint64 actual_num_hw_counters = 0;

// Array to hold the state of all firmware counters.
static volatile struct FirmwareCounterState firmware_counters[SBI_PMU_COUNTER_NUM_FW];

// Shared memory snapshot support state variables.
static volatile uint64 snapshot_shmem_phys_lo = SBI_PMU_NO_COUNTER_IDX;
static volatile uint64 snapshot_shmem_phys_hi = SBI_PMU_NO_COUNTER_IDX;
static volatile uint64 snapshot_flags = 0;

// --- SBI Initialization ---

/**
 * sbi_pmu_init()
 * Probes hardware counters by attempting to write/read mhpmevent CSRs.
 * Sets the global 'actual_num_hw_counters'.
 * MUST be called once during SBI initialization.
 */
void sbi_pmu_init(void) {
    uint64 probed_hw_count = 0;
    uint64 saved_event;

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_init: Probing hardware counters...\n");
    #endif

    // Iterate through the *maximum possible* hardware counters
    for (uint64 hw_idx = 0; hw_idx < SBI_PMU_MAX_HW_COUNTERS; hw_idx++) {
        uint64 csr_idx = hw_idx + SBI_PMU_HW_COUNTER_IDX_BASE; // CSR index (3, 4, ...)

        // Read current event config
        saved_event = read_hw_event_csr(csr_idx);

        // Try to configure a test event (CPU Cycles)
        write_hw_event_csr(csr_idx, SBI_PMU_HW_CPU_CYCLES);

        // Check if write was successful (CSR exists and is writable)
        if (read_hw_event_csr(csr_idx) != 0) {
            // Success! This counter exists. Increment count.
            probed_hw_count += 1;
            // Restore original event config (might have been non-zero)
            write_hw_event_csr(csr_idx, saved_event);
        } else {
            // Failure. Assume this and subsequent counters don't exist.
            #ifdef SBI_PMU_DEBUG
            printf("  Probe failed at HW index %d (CSR %d). Assuming %d HW counters.\n",
                   hw_idx, csr_idx, probed_hw_count);
            #endif
            break; // Stop probing
        }
    }
    
    actual_num_hw_counters = probed_hw_count;

    // Initialize firmware counter states (already zeroed in .bss, but set event/active explicitly)
    for (uint64 i = 0; i < SBI_PMU_COUNTER_NUM_FW; i++) {
        firmware_counters[i].counter = 0;
        firmware_counters[i].event = SBI_PMU_HW_NO_EVENT; // No event initially
        firmware_counters[i].active = false;
    }

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_init: Found %d hardware counters. Total counters = %d.\n",
           actual_num_hw_counters, actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW);
    #endif
}

// --- SBI Function Implementations ---


/**
 * FID #0: Get Number of Counters
 * Returns the total number of detected hardware counters plus firmware counters.
 */
struct SbiRet sbi_pmu_num_counters_impl(void) {
    // Assumes sbi_pmu_init() has already run.
    return (struct SbiRet){
        .error = SBI_SUCCESS,
        .value = actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW
    };
}


/**
 * FID #1: Get Counter Info
 * Provides information about a specific counter based on its SBI index.
 * Bits [11:0]   = CSR number (assumed to be contiguous starting from SBI_PMU_COUNTER_ADDR_U)
 * Bits [17:12]  = Width (one less than the number of bits in the counter)
 * Bit [63]      = Type (0 = hardware, 1 = firmware)
 */
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_get_info_impl(SBI Idx=%d)\n", counter_idx);
    #endif

    if (!isValidCounterIdx(counter_idx)) {
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    struct SbiRet ret = { .error = SBI_SUCCESS, .value = 0 };
    uint64 width_field = (SBI_PMU_COUNTER_WIDTH - 1) << 12; // [17:12] Width

    if (isHardwareCounterIdx(counter_idx)) {
        // --- Hardware Counter ---
        uint64 csr_num = SBI_PMU_HW_COUNTER_CSR_BASE + counter_idx; // CSR number (e.g., 0xC03)
        uint64 type_field = 0; // Type 0 for hardware

        ret.value = (csr_num & 0xFFF) // [11:0] CSR Number (lower 12 bits)
                  | width_field
                  | type_field;
    } else {
        // --- Firmware Counter ---
        // No CSR number for firmware counters.
        uint64 type_field = (1UL << 63); // Type 1 for firmware

        ret.value = width_field | type_field;
    }

    return ret;
}

/**
 * FID #2: Configure Matching Counter
 * Finds an available counter matching the type (HW/FW) implied by event_idx
 * within the specified range (using SBI indices) and configures it.
 * 
 * Parameters:
 *   counter_idx_base:  Base index for a set of logical counters.
 *   counter_idx_mask:  Bitmask (relative to the base) specifying which counters to consider.
 *   config_flags:      Configuration and filter flags (bits 0-7 used; reserved bits must be zero).
 *   event_idx:         Encodes the event type (upper 4 bits) and event code (lower 16 bits).
 *   event_data:        Additional event data (typically used for firmware or raw events).
 *
 * Returns:
 *   On success, ret.error == SBI_SUCCESS and ret.value contains the chosen counter index.
 *   On failure, an appropriate error code is returned.
 */
struct SbiRet sbi_pmu_counter_config_matching_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_config_matching_impl(base SBI Idx=%d, mask=0x%x, flags=0x%x, event=0x%x) - HW Counters: %d\n",
           counter_idx_base, counter_idx_mask, config_flags, event_idx, actual_num_hw_counters);
    #endif

    // --- Validation ---

    bool isHardware = isHardwareEvent(event_idx);

    // Validate reserved bits in config_flags: only lower 8 bits are allowed
    if (config_flags & SBI_PMU_CFG_RESERVED_MASK) {
        #ifdef SBI_PMU_DEBUG
        printf("  Error: Reserved config flags set (0x%x)\n", config_flags & SBI_PMU_CFG_RESERVED_MASK);
        #endif
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    // Base index must be a valid *starting* SBI index
    if (!isValidCounterIdx(counter_idx_base)) {
        #ifdef SBI_PMU_DEBUG
        printf("  Error: Invalid base SBI index (%d)\n", counter_idx_base);
        #endif
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    bool target_is_hw = isHardwareEvent(event_idx);
    uint64 selected_idx = SBI_PMU_NO_COUNTER_IDX; // Stores the selected SBI index
    uint64 current_mcountinhibit = read_mcountinhibit();

    // --- Selection Loop (Iterating through SBI indices based on mask) ---
    uint64 curr_mcountinhibit = read_mcountinhibit();
    for (int bit = 0; bit < 64; bit++) {
        if (!(counter_idx_mask & (1UL << bit))) {
            #ifdef SBI_PMU_DEBUG
            //printf("    SBI Idx %d isn't contained by mask\n", counter_idx_base + bit);
            #endif
            continue; // Skip if not in mask
        }

        uint64 current_sbi_idx = counter_idx_base + bit;
        if (!isValidCounterIdx(current_sbi_idx)) {
            #ifdef SBI_PMU_DEBUG
            //printf("    SBI Idx %d isn't valid\n", current_sbi_idx);
            #endif
            continue; // Skip if combined SBI index is out of range
        }

        // --- Check if counter type matches target event type ---
        if(target_is_hw) {
            #ifdef SBI_PMU_DEBUG
            printf("    Testing HW idx %d\n", current_sbi_idx);
            #endif

            if (!isHardwareCounterIdx(current_sbi_idx)) continue; // Skip FW indices if target is HW

            // --- Hardware Counter Availability Check ---
            uint64 hw_csr_idx = current_sbi_idx + SBI_PMU_HW_COUNTER_IDX_BASE; // CSR index (3+)
            bool is_inhibited = (current_mcountinhibit >> hw_csr_idx) & 1UL;
            uint64 current_event = read_hw_event_csr(hw_csr_idx);

            if (!(config_flags & SBI_PMU_CFG_FLAG_SKIP_MATCH)) {
                if (!is_inhibited || (current_event != SBI_PMU_HW_NO_EVENT)) {
                    #ifdef SBI_PMU_DEBUG
                    printf("    HW SBI Idx %d (CSR %d) unavailable (inhibited=%d, event=0x%x)\n", current_sbi_idx, hw_csr_idx, !is_inhibited, current_event);
                    #endif
                    continue;
                }
            }
            #ifdef SBI_PMU_DEBUG
            printf("    HW SBI Idx %d is valid\n", current_sbi_idx);
            #endif
            selected_idx = current_sbi_idx; // Select this SBI index
            break;

        } else { // Target is Firmware
            #ifdef SBI_PMU_DEBUG
            printf("    Testing FW idx %d\n", current_sbi_idx);
            #endif

            if (!isFirmwareCounterIdx(current_sbi_idx)) continue; // Skip HW indices if target is FW

            // --- Firmware Counter Availability Check ---
            uint64 fw_struct_idx = current_sbi_idx - actual_num_hw_counters; // Index into firmware_counters array
            bool is_active = firmware_counters[fw_struct_idx].active;
            uint64 current_event = firmware_counters[fw_struct_idx].event;

            if (!(config_flags & SBI_PMU_CFG_FLAG_SKIP_MATCH)) {
                if (is_active || (current_event != SBI_PMU_HW_NO_EVENT)) {
                    #ifdef SBI_PMU_DEBUG
                    printf("    FW SBI Idx %d unavailable (active=%d, event=0x%x)\n", current_sbi_idx, is_active, current_event);
                    #endif
                    continue;
                }
            }

            #ifdef SBI_PMU_DEBUG
            printf("    FW SBI Idx %d is valid\n", current_sbi_idx);
            #endif
            selected_idx = current_sbi_idx; // Select this SBI index
            break;
        }
    }

    // --- Configuration ---
    if (selected_idx == SBI_PMU_NO_COUNTER_IDX) {
        #ifdef SBI_PMU_DEBUG
        printf("  Error: No suitable counter found in mask 0x%x (base SBI Idx %d)\n", counter_idx_mask, counter_idx_base);
        #endif
        if (config_flags & SBI_PMU_CFG_FLAG_SKIP_MATCH) {
             return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
        } else {
             return (struct SbiRet){ .error = SBI_ERR_NOT_SUPPORTED, .value = 0 };
        }
    }

    // --- Apply Configuration to selected_idx ---
    if (isHardwareCounterIdx(selected_idx)) { // Check type of selected index
        // --- Configure Hardware Counter ---
        uint64 hw_csr_idx = selected_idx + SBI_PMU_HW_COUNTER_IDX_BASE;

        if (isHardwareEvent(event_idx)) {
             write_hw_event_csr(hw_csr_idx, event_idx);
        } else {
             #ifdef SBI_PMU_DEBUG
             printf("  Error: Attempting to configure HW counter (SBI Idx %d) with FW event (0x%x)\n", selected_idx, event_idx);
             #endif
             return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 }; // Mismatched event type
        }

        if (config_flags & SBI_PMU_CFG_FLAG_CLEAR_VALUE) {
            write_hw_counter(hw_csr_idx, 0UL);
        }
        if (config_flags & SBI_PMU_CFG_FLAG_AUTO_START) {
            current_mcountinhibit &= ~(1UL << hw_csr_idx);
            write_mcountinhibit(current_mcountinhibit);
        }

    } else { // Selected index is Firmware
        // --- Configure Firmware Counter ---
        uint64 fw_struct_idx = selected_idx - actual_num_hw_counters;

        if (isFirmwareEvent(event_idx)) {
            firmware_counters[fw_struct_idx].event = event_idx;
        } else {
             #ifdef SBI_PMU_DEBUG
             printf("  Error: Attempting to configure FW counter (SBI Idx %d) with HW event (0x%x)\n", selected_idx, event_idx);
             #endif
             return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 }; // Mismatched event type
        }

        if (config_flags & SBI_PMU_CFG_FLAG_CLEAR_VALUE) {
            firmware_counters[fw_struct_idx].counter = 0UL;
        }
        if (config_flags & SBI_PMU_CFG_FLAG_AUTO_START) {
            firmware_counters[fw_struct_idx].active = true;
        } else {
             firmware_counters[fw_struct_idx].active = false;
        }
    }

    #ifdef SBI_PMU_DEBUG
    printf("  Configuration successful for SBI index %d\n", selected_idx);
    #endif

    return (struct SbiRet){ .error = SBI_SUCCESS, .value = selected_idx };
}

/**
 * FID #3: Start Counters
 * Enables counting for selected counters (using SBI indices).
 * 
 * For each counter indicated in counter_idx_mask (starting at counter_idx_base):
 *   (1) Write the initial_value to the hardware counter CSR.
 *   (2) Clear its corresponding bit in mcountinhibit to enable counting.
 */
struct SbiRet sbi_pmu_counter_start_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_start_impl(base SBI Idx=%d, mask=0x%x, flags=0x%x, init_val=%d)\n",
           counter_idx_base, counter_idx_mask, start_flags, initial_value);
    #endif

    if (start_flags & SBI_PMU_START_RESERVED_MASK) {
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    bool set_initial = (start_flags & SBI_PMU_START_SET_INIT_VALUE);
    uint64 mcountinhibit_to_clear = 0;

    for (int bit = 0; bit < 64; bit++) {
        if (!(counter_idx_mask & (1UL << bit))) continue;

        uint64 current_sbi_idx = counter_idx_base + bit;
        if (!isValidCounterIdx(current_sbi_idx)) {
            #ifdef SBI_PMU_DEBUG
            printf("  Warning: Invalid SBI index %d in start mask ignored\n", current_sbi_idx);
            #endif
            continue;
        }

        if (isHardwareCounterIdx(current_sbi_idx)) {
            uint64 hw_csr_idx = current_sbi_idx + SBI_PMU_HW_COUNTER_IDX_BASE;
            mcountinhibit_to_clear |= (1UL << hw_csr_idx);
            if (set_initial) {
                write_hw_counter(hw_csr_idx, initial_value);
            }
        } else { // Firmware
            uint64 fw_struct_idx = current_sbi_idx - actual_num_hw_counters;
            if (set_initial) {
                firmware_counters[fw_struct_idx].counter = initial_value;
            }
            firmware_counters[fw_struct_idx].active = true;
        }
    }

    if (mcountinhibit_to_clear != 0) {
        uint64 current_mcountinhibit = read_mcountinhibit();
        write_mcountinhibit(current_mcountinhibit & ~mcountinhibit_to_clear);
    }

    #ifdef SBI_PMU_DEBUG
    printf("  mcountinhibit after start: 0x%x\n", read_mcountinhibit());
    #endif

    return (struct SbiRet){ .error = SBI_SUCCESS, .value = 0 };
}

/**
 * FID #4: Stop Counters
 * Disables counting for selected counters (using SBI indices).
 * 
 * For each counter indicated in counter_idx_mask (starting at counter_idx_base),
 * set its corresponding bit in mcountinhibit to inhibit counting.
 */
struct SbiRet sbi_pmu_counter_stop_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_counter_stop_impl(base SBI Idx=%d, mask=0x%x, flags=0x%x)\n",
           counter_idx_base, counter_idx_mask, stop_flags);
    #endif
    
    if (stop_flags & SBI_PMU_STOP_RESERVED_MASK) {
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    bool reset_event = (stop_flags & SBI_PMU_STOP_FLAG_RESET);
    // bool take_snapshot = (stop_flags & SBI_PMU_STOP_FLAG_TAKE_SNAPSHOT); // TODO
    uint64 mcountinhibit_to_set = 0;

    for (int bit = 0; bit < 64; bit++) {
        if (!(counter_idx_mask & (1UL << bit))) continue;

        uint64 current_sbi_idx = counter_idx_base + bit;
        if (!isValidCounterIdx(current_sbi_idx)) {
             #ifdef SBI_PMU_DEBUG
             printf("  Warning: Invalid SBI index %d in stop mask ignored\n", current_sbi_idx);
             #endif
            continue;
        }

        if (isHardwareCounterIdx(current_sbi_idx)) {
            uint64 hw_csr_idx = current_sbi_idx + SBI_PMU_HW_COUNTER_IDX_BASE;
            mcountinhibit_to_set |= (1UL << hw_csr_idx);
            if (reset_event) {
                write_hw_event_csr(hw_csr_idx, SBI_PMU_HW_NO_EVENT);
            }
        } else { // Firmware
            uint64 fw_struct_idx = current_sbi_idx - actual_num_hw_counters;
            firmware_counters[fw_struct_idx].active = false;
            if (reset_event) {
                firmware_counters[fw_struct_idx].event = SBI_PMU_HW_NO_EVENT;
            }
        }
        // TODO: Handle snapshot logic if take_snapshot is set
    }

    if (mcountinhibit_to_set != 0) {
        uint64 current_mcountinhibit = read_mcountinhibit();
        write_mcountinhibit(current_mcountinhibit | mcountinhibit_to_set);
    }

    #ifdef SBI_PMU_DEBUG
    // printf("  mcountinhibit after stop: 0x%x\n", read_mcountinhibit());
    // dump_hpm_state();
    #endif

    return (struct SbiRet){ .error = SBI_SUCCESS, .value = 0 };
}

/**
 * FID #5: Read Firmware Counter (Lower Bits)
 * Reads the value of a firmware counter using its SBI index.
 */
struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx) {
    #ifdef SBI_PMU_DEBUG
    // printf("sbi_pmu_counter_fw_read_impl(SBI Idx=%d)\n", counter_idx);
    #endif

    if (!isFirmwareCounterIdx(counter_idx)) { // Check if it's a valid FW SBI index
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    uint64 fw_struct_idx = counter_idx - actual_num_hw_counters; // Convert SBI index to array index
    uint64 value = firmware_counters[fw_struct_idx].counter;

    return (struct SbiRet){ .error = SBI_SUCCESS, .value = value };
}

/**
 * FID #6: Read Firmware Counter (Higher Bits)
 * Returns 0 for RV64. Checks for valid FW SBI index.
 */
struct SbiRet sbi_pmu_counter_fw_read_hi_impl(uint64 counter_idx) {
    #ifdef SBI_PMU_DEBUG
    // printf("sbi_pmu_counter_fw_read_hi_impl(SBI Idx=%d)\n", counter_idx);
    #endif

    if (!isFirmwareCounterIdx(counter_idx)) { // Check if it's a valid FW SBI index
        return (struct SbiRet){ .error = SBI_ERR_INVALID_PARAM, .value = 0 };
    }

    return (struct SbiRet){ .error = SBI_SUCCESS, .value = 0 }; // High bits are 0 for RV64
}

/**
 * FID #7: Set Snapshot Shared Memory
 * Configures the memory region for counter snapshots.
 */
struct SbiRet sbi_pmu_snapshot_set_shmem_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags) {

    #ifdef SBI_PMU_DEBUG
    printf("sbi_pmu_snapshot_set_shmem_impl(lo=0x%x, hi=0x%x, flags=0x%x)\n",
           shmem_phys_lo, shmem_phys_hi, flags);
    #endif

    struct SbiRet ret = { .error = SBI_SUCCESS, .value = 0 };

    if (flags != 0) {
        ret.error = SBI_ERR_INVALID_PARAM;
        return ret;
    }
    if (shmem_phys_lo == SBI_PMU_NO_COUNTER_IDX && shmem_phys_hi == SBI_PMU_NO_COUNTER_IDX) {
        snapshot_shmem_phys_lo = SBI_PMU_NO_COUNTER_IDX;
        snapshot_shmem_phys_hi = SBI_PMU_NO_COUNTER_IDX;
        snapshot_flags = 0;
        #ifdef SBI_PMU_DEBUG
        printf("  Snapshotting disabled.\n");
        #endif
        return ret;
    }
    if ((shmem_phys_lo & (4096 - 1)) != 0) {
         #ifdef SBI_PMU_DEBUG
         printf("  Error: shmem_phys_lo (0x%x) not 4 KiB aligned.\n", shmem_phys_lo);
         #endif
        ret.error = SBI_ERR_INVALID_PARAM;
        return ret;
    }

    // --- Address Validation (CRITICAL TODO) ---
    bool address_is_valid_and_writable = false; // <<<<----- IMPLEMENT THIS CHECK!
    if (!address_is_valid_and_writable) {
         #ifdef SBI_PMU_DEBUG
         printf("  Error: Shared memory address validation FAILED (or not implemented).\n");
         #endif
        ret.error = SBI_ERR_INVALID_ADDRESS;
        return ret;
    }
    // --- End Address Validation ---

    snapshot_shmem_phys_lo = shmem_phys_lo;
    snapshot_shmem_phys_hi = shmem_phys_hi;
    snapshot_flags = flags;

     #ifdef SBI_PMU_DEBUG
     printf("  Snapshot memory set: base=0x%x%x\n", snapshot_shmem_phys_hi, snapshot_shmem_phys_lo);
     #endif

    return ret;
}

// --- Firmware Event Counting Implementation ---
void sbi_pmu_fw_count(uint64 event_idx) {
    for (uint64 fw_struct_idx = 0; fw_struct_idx < SBI_PMU_COUNTER_NUM_FW; fw_struct_idx++) {
        if (firmware_counters[fw_struct_idx].active &&
            firmware_counters[fw_struct_idx].event == event_idx)
        {
            firmware_counters[fw_struct_idx].counter++;
        }
    }
}
void sbi_pmu_fw_misaligned_load() { sbi_pmu_fw_count(SBI_PMU_FW_MISALIGNED_LOAD); }
void sbi_pmu_fw_misaligned_store(){ sbi_pmu_fw_count( SBI_PMU_FW_MISALIGNED_STORE); }
void sbi_pmu_fw_access_load() { sbi_pmu_fw_count(SBI_PMU_FW_ACCESS_LOAD); }
void sbi_pmu_fw_access_store(){ sbi_pmu_fw_count(SBI_PMU_FW_ACCESS_STORE); }
void sbi_pmu_fw_illegal_insn() { sbi_pmu_fw_count(SBI_PMU_FW_ILLEGAL_INSN); }
void sbi_pmu_fw_set_timer() { sbi_pmu_fw_count(SBI_PMU_FW_SET_TIMER); }
void sbi_pmu_fw_ipi_sent() { sbi_pmu_fw_count(SBI_PMU_FW_IPI_SENT); }
void sbi_pmu_fw_ipi_received() { sbi_pmu_fw_count(SBI_PMU_FW_IPI_RECEIVED); }
void sbi_pmu_fw_fence_i_sent() { sbi_pmu_fw_count(SBI_PMU_FW_FENCE_I_SENT); }
void sbi_pmu_fw_fence_i_received() { sbi_pmu_fw_count(SBI_PMU_FW_FENCE_I_RECEIVED); }
void sbi_pmu_fw_sfence_vma_sent() { sbi_pmu_fw_count(SBI_PMU_FW_SFENCE_VMA_SENT); }
void sbi_pmu_fw_sfence_vma_received() { sbi_pmu_fw_count(SBI_PMU_FW_SFENCE_VMA_RECEIVED); }
void sbi_pmu_fw_sfence_vma_asid_sent() { sbi_pmu_fw_count(SBI_PMU_FW_SFENCE_VMA_ASID_SENT); }
void sbi_pmu_fw_sfence_vma_asid_received() { sbi_pmu_fw_count(SBI_PMU_FW_SFENCE_VMA_ASID_RECEIVED); }
void sbi_pmu_fw_hfence_gvma_sent() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_GVMA_SENT); }
void sbi_pmu_fw_hfence_gvma_received() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_GVMA_RECEIVED); }
void sbi_pmu_fw_hfence_gvma_vmid_sent() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_GVMA_VMID_SENT); }
void sbi_pmu_fw_hfence_gvma_vmid_received() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_GVMA_VMID_RECEIVED); }
void sbi_pmu_fw_hfence_vvma_sent() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_VVMA_SENT); }
void sbi_pmu_fw_hfence_vvma_received() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_VVMA_RECEIVED); }
void sbi_pmu_fw_hfence_vvma_asid_sent() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_VVMA_ASID_SENT); }
void sbi_pmu_fw_hfence_vvma_asid_received() { sbi_pmu_fw_count(SBI_PMU_FW_HFENCE_VVMA_ASID_RECEIVED); }

// --- Helper Function Implementations ---

// Checks if an SBI index maps to an implemented hardware counter
bool isHardwareCounterIdx(uint64 idx) {
    // Assumes sbi_pmu_init() has run
    return idx < actual_num_hw_counters;
}

// Checks if an SBI index maps to a firmware counter
bool isFirmwareCounterIdx(uint64 idx) {
    // Assumes sbi_pmu_init() has run
    uint64 total_counters = actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW;
    return idx >= actual_num_hw_counters && idx < total_counters;
}

// Checks if an SBI index is valid (within the range of implemented HW + FW counters)
bool isValidCounterIdx(uint64 idx) {
    // Assumes sbi_pmu_init() has run
    return idx < (actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW);
}

// isHardwareEvent and isFirmwareEvent remain the same as previous version
bool isHardwareEvent(uint64 event_idx) {
    uint64 type = event_idx >> 16;
    return type == 0 || type == 1; // Add other HW types if supported
}
bool isFirmwareEvent(uint64 event_idx) {
    return (event_idx >> 16) == 15;
}

/* --- Hardware CSR Access Helpers --- */
/*
 * Due to RISC-V requirements, the CSR number must be an immediate.
 * We therefore use a switch-case to select the correct CSR based on the counter index.
 */

 uint64 read_hw_counter(uint64 hw_counter_csr_idx) {
    uint64 value = 0;
    switch(hw_counter_csr_idx) {
        case 3:  asm volatile ("csrr %0, mhpmcounter3" : "=r"(value)); break;
        case 4:  asm volatile ("csrr %0, mhpmcounter4" : "=r"(value)); break;
        // ... cases 5 through 30 ...
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
        default: break; // Should not happen
    }
    return value;
}

uint64 read_hw_event_csr(uint64 hw_event_csr_idx) {
    uint64 value = 0;
    switch(hw_event_csr_idx) {
        case 3:  asm volatile ("csrr %0, mhpmevent3" : "=r"(value)); break;
        case 4:  asm volatile ("csrr %0, mhpmevent4" : "=r"(value)); break;
        // ... cases 5 through 30 ...
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

void write_hw_counter(uint64 hw_counter_csr_idx, uint64 value) {
     switch(hw_counter_csr_idx) {
        case 3:  asm volatile ("csrw mhpmcounter3, %0" :: "r"(value)); break;
        case 4:  asm volatile ("csrw mhpmcounter4, %0" :: "r"(value)); break;
        // ... cases 5 through 30 ...
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

void write_hw_event_csr(uint64 hw_event_csr_idx, uint64 value) {
    switch(hw_event_csr_idx) {
        case 3:  asm volatile ("csrw mhpmevent3, %0" :: "r"(value)); break;
        case 4:  asm volatile ("csrw mhpmevent4, %0" :: "r"(value)); break;
        // ... cases 5 through 30 ...
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

uint64 read_mcountinhibit(void) {
    uint64 value;
    asm volatile ("csrr %0, mcountinhibit" : "=r"(value));
    return value;
}
void write_mcountinhibit(uint64 value) {
    #ifdef SBI_PMU_DEBUG
    // printf("write_mcountinhibit(val=0x%x)\n", value);
    #endif
    asm volatile ("csrw mcountinhibit, %0" :: "r"(value));
}

// --- Debugging Helpers ---

void dump_hpm(uint64 counter_sbi_idx) {
    #ifdef SBI_PMU_DEBUG
    printf("dump_hpm(SBI Idx=%d): ", counter_sbi_idx);

    uint64 counter_state = 0;
    uint64 event_state = 0;
    bool is_counting = false;

    if (!isValidCounterIdx(counter_sbi_idx)) {
        printf("INVALID SBI INDEX\n");
        return;
    }

    if (isHardwareCounterIdx(counter_sbi_idx)) {
        uint64 hw_csr_idx = counter_sbi_idx + SBI_PMU_HW_COUNTER_IDX_BASE;
        is_counting = !((read_mcountinhibit() >> hw_csr_idx) & 1UL);
        counter_state = read_hw_counter(hw_csr_idx);
        event_state = read_hw_event_csr(hw_csr_idx);
        printf("[HW] ");
    } else {
        uint64 fw_struct_idx = counter_sbi_idx - actual_num_hw_counters;
        is_counting = firmware_counters[fw_struct_idx].active;
        counter_state = firmware_counters[fw_struct_idx].counter;
        event_state = firmware_counters[fw_struct_idx].event;
        printf("[FW] ");
    }

    printf("%s [Event: 0x%x]: %d\n",
           is_counting ? "Counting" : "Stopped ",
           event_state, counter_state);
    #endif // SBI_PMU_DEBUG
}

void dump_hpm_state(void) {
     #ifdef SBI_PMU_DEBUG
    printf("--- PMU Counter State Dump ---\n");
    uint64 mcountinhibit = read_mcountinhibit();
    printf("mcountinhibit: 0x%x\n", mcountinhibit);
    uint64 total_counters = actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW;

    printf("Hardware Counters (SBI Index 0-%d):\n", actual_num_hw_counters - 1);
    for (uint64 i = 0; i < actual_num_hw_counters; i++) {
        dump_hpm(i); // Use SBI index
    }

    printf("Firmware Counters (SBI Index %d-%d):\n", actual_num_hw_counters, total_counters - 1);
     for (uint64 i = actual_num_hw_counters; i < total_counters; i++) {
        dump_hpm(i); // Use SBI index
    }
    printf("--- End Dump ---\n");
     #endif // SBI_PMU_DEBUG
}




