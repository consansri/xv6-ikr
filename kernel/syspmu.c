// kernel/syspmu.c
#include "include/syspmu.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/proc.h"
#include "sbi/include/sbi_call.h"
#include "include/vm.h" // For copyin, copyout

// Helper function from previous implementation
uint64 get_physical_mask(struct proc *p, uint64 handle_mask) {
    uint64 physical_mask = 0;
    // Ensure we only consider valid handles from the last successful setup
    uint64 effective_handle_mask = handle_mask & p->pmu_config_success_mask;
    for (int handle = 0; handle < MAX_PMU_HANDLES; ++handle) {
        if ((effective_handle_mask & (1L << handle)) && p->pmu_maps[handle].valid) {
            physical_mask |= (1L << p->pmu_maps[handle].counter_idx);
        }
    }
    return physical_mask;
}
// Helpers stop_physical_counters, start_physical_counters remain similar

// Helper to clear existing PMU config for a process
void pmu_clear_config(struct proc *p) {
    #ifdef KERNEL_PMU_DEBUG
    //printf("pmu_clear_config(proc: %d)\n", p->pid);
    #endif

    acquire(&p->lock);
    uint64 started_physical_mask = get_physical_mask(p, p->pmu_started_handles_mask);
    release(&p->lock);

    if(started_physical_mask != 0) {
        stop_physical_counters_with_reset(started_physical_mask); // Ignore errors during cleanup
    }

    acquire(&p->lock);
    for(int i=0; i < MAX_PMU_HANDLES; ++i) {
        p->pmu_maps[i].valid = 0;
    }
    p->pmu_config_success_mask = 0;
    p->pmu_started_handles_mask = 0;
    release(&p->lock);
}

int stop_physical_counters_with_reset(uint64 physical_mask) {
    #ifdef KERNEL_PMU_DEBUG
    //printf("stop_physical_counters(physical_mask: %x)\n", physical_mask);
    #endif

    if (physical_mask == 0) return 0; // Nothing to stop
    // Call SBI stop with base=0, mask=physical_mask, flags=1 (reset)
    struct SbiRet ret = sbi_pmu_counter_stop(0, physical_mask, 1);
    if (ret.error != SBI_SUCCESS) {
        return -1;
    }

    return  0;
}

// Helper to stop specific physical counters
// Uses the sbi_pmu_counter_stop wrapper from sbi_call.h
int stop_physical_counters(uint64 physical_mask) {
    #ifdef KERNEL_PMU_DEBUG
    //printf("stop_physical_counters(physical_mask: %x)\n", physical_mask);
    #endif

    if (physical_mask == 0) return 0; // Nothing to stop
    // Call SBI stop with base=0, mask=physical_mask, flags=0 (don't reset value)
    struct SbiRet ret = sbi_pmu_counter_stop(0, physical_mask, 0);
    if (ret.error != SBI_SUCCESS) {
        return -1;
    }

    return  0;
}

// Helper to start specific physical counters with reset
// Uses the sbi_pmu_counter_start wrapper from sbi_call.h
int start_physical_counters_with_reset(uint64 physical_mask) {
    #ifdef KERNEL_PMU_DEBUG
    //printf("start_physical_counters_with_reset(physical_mask: %x)\n", physical_mask);
    #endif

    if (physical_mask == 0) return 0;
    // Call SBI start with base=0, mask=physical_mask, flags=1 (set initial value), initial_value=0
    struct SbiRet ret = sbi_pmu_counter_start(0, physical_mask, 1, 0);

    if(ret.error != SBI_SUCCESS) {
        return -1;
    }

    return 0;
}

// Helper to start specific physical counters
// Uses the sbi_pmu_counter_start wrapper from sbi_call.h
int start_physical_counters(uint64 physical_mask) {
    #ifdef KERNEL_PMU_DEBUG
    //printf("start_physical_counters(physical_mask: %x)\n", physical_mask);
    #endif

    if (physical_mask == 0) return 0;
    // Call SBI start with base=0, mask=physical_mask, flags=1 (set initial value), initial_value=0
    struct SbiRet ret = sbi_pmu_counter_start(0, physical_mask, 0, 0);

    if(ret.error != SBI_SUCCESS) {
        return -1;
    }

    return 0;
}


uint64 hw_read_counter(uint64 hw_csr_addr) {
    uint64 value = 0;
    switch(hw_csr_addr & 0b11111) {
        case 3:  asm volatile ("csrr %0, hpmcounter3" : "=r"(value)); break;
        case 4:  asm volatile ("csrr %0, hpmcounter4" : "=r"(value)); break;
        case 5:  asm volatile ("csrr %0, hpmcounter5" : "=r"(value)); break;
        case 6:  asm volatile ("csrr %0, hpmcounter6" : "=r"(value)); break;
        case 7:  asm volatile ("csrr %0, hpmcounter7" : "=r"(value)); break;
        case 8:  asm volatile ("csrr %0, hpmcounter8" : "=r"(value)); break;
        case 9:  asm volatile ("csrr %0, hpmcounter9" : "=r"(value)); break;
        case 10: asm volatile ("csrr %0, hpmcounter10" : "=r"(value)); break;
        case 11: asm volatile ("csrr %0, hpmcounter11" : "=r"(value)); break;
        case 12: asm volatile ("csrr %0, hpmcounter12" : "=r"(value)); break;
        case 13: asm volatile ("csrr %0, hpmcounter13" : "=r"(value)); break;
        case 14: asm volatile ("csrr %0, hpmcounter14" : "=r"(value)); break;
        case 15: asm volatile ("csrr %0, hpmcounter15" : "=r"(value)); break;
        case 16: asm volatile ("csrr %0, hpmcounter16" : "=r"(value)); break;
        case 17: asm volatile ("csrr %0, hpmcounter17" : "=r"(value)); break;
        case 18: asm volatile ("csrr %0, hpmcounter18" : "=r"(value)); break;
        case 19: asm volatile ("csrr %0, hpmcounter19" : "=r"(value)); break;
        case 20: asm volatile ("csrr %0, hpmcounter20" : "=r"(value)); break;
        case 21: asm volatile ("csrr %0, hpmcounter21" : "=r"(value)); break;
        case 22: asm volatile ("csrr %0, hpmcounter22" : "=r"(value)); break;
        case 23: asm volatile ("csrr %0, hpmcounter23" : "=r"(value)); break;
        case 24: asm volatile ("csrr %0, hpmcounter24" : "=r"(value)); break;
        case 25: asm volatile ("csrr %0, hpmcounter25" : "=r"(value)); break;
        case 26: asm volatile ("csrr %0, hpmcounter26" : "=r"(value)); break;
        case 27: asm volatile ("csrr %0, hpmcounter27" : "=r"(value)); break;
        case 28: asm volatile ("csrr %0, hpmcounter28" : "=r"(value)); break;
        case 29: asm volatile ("csrr %0, hpmcounter29" : "=r"(value)); break;
        case 30: asm volatile ("csrr %0, hpmcounter30" : "=r"(value)); break;
        case 31: asm volatile ("csrr %0, hpmcounter31" : "=r"(value)); break;
        default: break;
    }

    return value;
}

// Syscall Implementation: pmu_setup
uint64 sys_pmu_setup(void) {
    uint64 config_mask, user_event_codes_ptr, user_flags_ptr;
    struct proc *p = myproc();
    uint64 success_mask = 0;
    uint64 allocated_physical_mask = 0; // Track physical counters used in this call

    if(argaddr(0, &config_mask) < 0 || argaddr(1, &user_event_codes_ptr) < 0 || argaddr(2, &user_flags_ptr) < 0) {
        return -1; // Use standard negative return for pointer errors
    }

    #ifdef KERNEL_PMU_DEBUG
    printf("sys_pmu_setup(config_mask: %d, user_event_codes_ptr: %x, user_flags_ptr: %x)\n", config_mask, user_event_codes_ptr, user_flags_ptr);
    #endif

    if(config_mask == 0) {
        pmu_clear_config(p); // Clear old config even if new one is empty
        return 0;
    }

    // Get total number of physical counters
    struct SbiRet num_ret = sbi_pmu_num_counters();
    if(num_ret.error != SBI_SUCCESS || num_ret.value <= 0) {
       return 0; // Return 0 for success_mask if PMU unavailable
    }
    uint64 num_physical_counters = num_ret.value;
    uint64 all_physical_mask = (num_physical_counters >= 64) ? ~0L : (1L << num_physical_counters) - 1;


    // --- Critical Section: Clear old config, prepare for new ---
    pmu_clear_config(p);
    // --- End Critical Section ---


    // Iterate through requested handles (bits in config_mask)
    for (int handle = 0; handle < MAX_PMU_HANDLES; ++handle) {
        if (!(config_mask & (1L << handle))) {
            continue; // Skip handles not requested
        }

        uint64 event_code, flags;
        // Fetch event code and flags from user space for this handle i
        // Need to handle potential faults during copyin
        if(copyin(p->pagetable, (char*)&event_code, user_event_codes_ptr + handle * sizeof(uint64), sizeof(uint64)) != 0 ||
           copyin(p->pagetable, (char*)&flags, user_flags_ptr + handle * sizeof(uint64), sizeof(uint64)) != 0)
        {
             // Error fetching args for this handle, stop configuration
             printf("pmu_setup: copyin failed for handle %d\n", handle);
             goto setup_cleanup; // Cleanup already allocated counters from this call
        }

        // Determine mask of *available* physical counters for this request
        uint64 available_physical_mask = all_physical_mask & ~allocated_physical_mask;
        if(available_physical_mask == 0) {
            printf("pmu_setup: no more physical counters available for handle %d\n", handle);
            goto setup_cleanup; // No physical counters left
        }

        // Ask SBI to configure a counter
        struct SbiRet config_ret = sbi_pmu_counter_config_matching(0, available_physical_mask, flags, event_code, 0); // event_data=0

        if(config_ret.error != SBI_SUCCESS) {
             printf("pmu_setup: config_matching failed (err %d) for handle %d\n", (int)config_ret.error, handle);
            // Could try next handle, but let's fail fast for simplicity
            goto setup_cleanup;
        }

        // Success for this handle
        uint64 physical_idx = config_ret.value;
        allocated_physical_mask |= (1L << physical_idx); // Mark physical counter as used *in this call*

        acquire(&p->lock); // Lock proc to update its state
        p->pmu_maps[handle].valid = 1;
        p->pmu_maps[handle].event_code = event_code;
        p->pmu_maps[handle].flags = flags;
        p->pmu_maps[handle].counter_idx = physical_idx;
        success_mask |= (1L << handle); // Add to overall success mask
        release(&p->lock);
    }

setup_cleanup:
    // Store the final success mask in the process structure
    acquire(&p->lock);
    p->pmu_config_success_mask = success_mask;
    release(&p->lock);

    // Note: If setup failed midway, success_mask reflects only handles configured *before* the failure.
    // Physical counters allocated *during* this failed call are implicitly released because
    // they are only tracked in the local `allocated_physical_mask` and not permanently associated
    // if the setup doesn't complete fully for all requested bits up to the failure point.
    // A more robust implementation might explicitly stop/release partially configured counters on failure.

    return success_mask;
}


// Syscall Implementation: pmu_control
uint64 sys_pmu_control(void) {
    int action;
    uint64 handle_mask, user_values_out_ptr;
    struct proc *p = myproc();

    if(argint(0, &action) < 0 || argaddr(1, &handle_mask) < 0 || argaddr(2, &user_values_out_ptr) < 0) {
        return -1;
    }

    #ifdef KERNEL_PMU_DEBUG
    printf("sys_pmu_control(action: %d, handle_mask: %x, user_values_out_ptr: %x)\n", action, handle_mask, user_values_out_ptr);
    #endif

    acquire(&p->lock);
    // Verify handle_mask is a subset of successfully configured handles
    if((handle_mask & ~p->pmu_config_success_mask) != 0) {
        release(&p->lock);
        printf("pmu_control: handle_mask contains unconfigured handles\n");
        return -1; // Error: requested handle was not successfully set up
    }

    // Determine effective mask based on action (e.g., only stop started handles)
    uint64 effective_handle_mask = handle_mask;
    if(action == PMU_ACTION_STOP || action == PMU_ACTION_STOP_READ) {
        effective_handle_mask &= p->pmu_started_handles_mask; // Only stop/stop-read handles that are actually running
    } else if (action == PMU_ACTION_START) {
         effective_handle_mask &= ~p->pmu_started_handles_mask; // Only start handles that are not already running
    }

    uint64 physical_mask = get_physical_mask(p, effective_handle_mask);
    uint64 read_physical_mask = get_physical_mask(p, handle_mask); // Mask for reading is always the requested mask

    release(&p->lock); // Release lock before potential SBI calls


    // --- Perform Actions ---
    int sbi_error = 0;

    // Stop Action (for STOP and STOP_READ)
    if((action == PMU_ACTION_STOP || action == PMU_ACTION_STOP_READ) && physical_mask != 0) {
        if(stop_physical_counters(physical_mask) != 0) {
            sbi_error = 1;
             printf("pmu_control: stop_physical_counters failed\n");
            // Decide whether to proceed or return error immediately
        }
    }

    // Read Action (for READ and STOP_READ)
    if((action == PMU_ACTION_READ || action == PMU_ACTION_STOP_READ) && handle_mask != 0) {
        int write_idx = 0;
        for(int handle = 0; handle < MAX_PMU_HANDLES; ++handle) {
            if(handle_mask & (1L << handle)) {
                // Check if handle is valid (should be, due to earlier check)

                acquire(&p->lock);
                uint64 phys_idx = p->pmu_maps[handle].counter_idx;
                release(&p->lock);

                struct SbiRet read_ret = sbi_pmu_counter_get_info(phys_idx);
                if(read_ret.error != SBI_SUCCESS) {
                    printf("pmu_control: read failed for handle %d (phys %d)\n", handle, phys_idx);
                    write_idx++;
                    continue;
                }

                uint64 value;
                if((read_ret.value & (1UL << 63)) == 0) {
                    // HW
                    uint64 csr_addr = read_ret.value & 0b111111111111;
                    value = hw_read_counter(csr_addr);
                    #ifdef KERNEL_PMU_DEBUG
                    printf("sys_pmu_control() -> HW READ(csr = %x) %d: %d \n", csr_addr, handle, value);
                    #endif
                } else {
                    // FW
                    value = sbi_pmu_counter_fw_read(phys_idx).value;
                    #ifdef KERNEL_PMU_DEBUG
                    printf("sys_pmu_control() -> FW READ %d: %d \n", handle, value);
                    #endif
                }

                if(copyout(p->pagetable, user_values_out_ptr + write_idx * sizeof(uint64), (char *)&value, sizeof(uint64)) != 0) {
                    return -1; // Copyout is fatal
                }

                write_idx++;
            }
        }
    }

    // Start Action (for START)
    if(action == PMU_ACTION_START && physical_mask != 0) {
         // Reset counters before starting by using flag bit 0 in sbi_pmu_counter_start
        if(start_physical_counters_with_reset(physical_mask) != 0) { // Need a new helper for this
            sbi_error = 1;
            printf("pmu_control: start_physical_counters_with_reset failed\n");
        }
    }

    // --- Update Kernel State ---
    acquire(&p->lock);
    if(action == PMU_ACTION_START) {
        p->pmu_started_handles_mask |= effective_handle_mask; // Add started handles
    } else if (action == PMU_ACTION_STOP || action == PMU_ACTION_STOP_READ) {
        p->pmu_started_handles_mask &= ~effective_handle_mask; // Remove stopped handles
    }
    release(&p->lock);

    if(sbi_error) {
        return -1; // Return error if any SBI call failed
    }

    return 0; 
}
