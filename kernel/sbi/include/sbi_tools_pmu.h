#ifndef __SBI_TOOLS_PMU
#define __SBI_TOOLS_PMU

#include "sbi_call.h"
#include "sbi_impl_pmu.h"
#include <stdbool.h>

struct CounterSet {
    bool    valid;
    uint64  counter_idx_0;
    uint64  event_idx_0;
    bool    is_hw_0;
    uint64  counter_idx_1;
    uint64  event_idx_1;
    bool    is_hw_1;
    uint64  counter_idx_2;
    uint64  event_idx_2;
    bool    is_hw_2;
    uint64  counter_idx_3;
    uint64  event_idx_3;
    bool    is_hw_3;
};

uint64 sbi_pmu_counter_hw_read(uint64 counter_idx) {
    #ifdef SBI_PMU_DEBUG
    printf("read_hw_counter(%d)\n", counter_idx);
    #endif

    uint64 value = 0UL;
    switch(counter_idx) {
        case 0:  asm volatile ("csrr %0, hpmcounter3" : "=r"(value)); break;
        case 1:  asm volatile ("csrr %0, hpmcounter4" : "=r"(value)); break;
        case 2:  asm volatile ("csrr %0, hpmcounter5" : "=r"(value)); break;
        case 3:  asm volatile ("csrr %0, hpmcounter6" : "=r"(value)); break;
        case 4:  asm volatile ("csrr %0, hpmcounter7" : "=r"(value)); break;
        case 5:  asm volatile ("csrr %0, hpmcounter8" : "=r"(value)); break;
        case 6:  asm volatile ("csrr %0, hpmcounter9" : "=r"(value)); break;
        case 7:  asm volatile ("csrr %0, hpmcounter10" : "=r"(value)); break;
        case 8:  asm volatile ("csrr %0, hpmcounter11" : "=r"(value)); break;
        case 9:  asm volatile ("csrr %0, hpmcounter12" : "=r"(value)); break;
        case 10: asm volatile ("csrr %0, hpmcounter13" : "=r"(value)); break;
        case 11: asm volatile ("csrr %0, hpmcounter14" : "=r"(value)); break;
        case 12: asm volatile ("csrr %0, hpmcounter15" : "=r"(value)); break;
        case 13: asm volatile ("csrr %0, hpmcounter16" : "=r"(value)); break;
        case 14: asm volatile ("csrr %0, hpmcounter17" : "=r"(value)); break;
        case 15: asm volatile ("csrr %0, hpmcounter18" : "=r"(value)); break;
        case 16: asm volatile ("csrr %0, hpmcounter19" : "=r"(value)); break;
        case 17: asm volatile ("csrr %0, hpmcounter20" : "=r"(value)); break;
        case 18: asm volatile ("csrr %0, hpmcounter21" : "=r"(value)); break;
        case 19: asm volatile ("csrr %0, hpmcounter22" : "=r"(value)); break;
        case 20: asm volatile ("csrr %0, hpmcounter23" : "=r"(value)); break;
        case 21: asm volatile ("csrr %0, hpmcounter24" : "=r"(value)); break;
        case 22: asm volatile ("csrr %0, hpmcounter25" : "=r"(value)); break;
        case 23: asm volatile ("csrr %0, hpmcounter26" : "=r"(value)); break;
        case 24: asm volatile ("csrr %0, hpmcounter27" : "=r"(value)); break;
        case 25: asm volatile ("csrr %0, hpmcounter28" : "=r"(value)); break;
        case 26: asm volatile ("csrr %0, hpmcounter29" : "=r"(value)); break;
        case 27: asm volatile ("csrr %0, hpmcounter30" : "=r"(value)); break;
        case 28: asm volatile ("csrr %0, hpmcounter31" : "=r"(value)); break;
        default: break;
    }
    return value;
}

void print_evt(uint64 event_idx) {
    uint64 event_idx_type = event_idx >> 16;
    uint64 event_idx_code = event_idx & 0xffffUL;

    switch(event_idx_type) {
        case 0UL: // Type 0
            switch(event_idx_code) {
                case SBI_PMU_HW_NO_EVENT:                   printf("SBI_PMU_HW_NO_EVENT"); break;
                case SBI_PMU_HW_CPU_CYCLES:                 printf("SBI_PMU_HW_CPU_CYCLES"); break;
                case SBI_PMU_HW_INSTRUCTIONS:               printf("SBI_PMU_HW_INSTRUCTIONS"); break;
                case SBI_PMU_HW_CACHE_REFERENCES:           printf("SBI_PMU_HW_CACHE_REFERENCES"); break;
                case SBI_PMU_HW_CACHE_MISSES:               printf("SBI_PMU_HW_CACHE_MISSES"); break;
                case SBI_PMU_HW_BRANCH_INSTRUCTIONS:        printf("SBI_PMU_HW_BRANCH_INSTRUCTIONS"); break;
                case SBI_PMU_HW_BRANCH_MISSES:              printf("SBI_PMU_HW_BRANCH_MISSES"); break;
                case SBI_PMU_HW_BUS_CYCLES:                 printf("SBI_PMU_HW_BUS_CYCLES"); break;
                case SBI_PMU_HW_STALLED_CYCLES_FRONTEND:    printf("SBI_PMU_HW_STALLED_CYCLES_FRONTEND"); break;
                case SBI_PMU_HW_STALLED_CYCLES_BACKEND:     printf("SBI_PMU_HW_STALLED_CYCLES_BACKEND"); break;
                case SBI_PMU_HW_REF_CPU_CYCLES:             printf("SBI_PMU_HW_REF_CPU_CYCLES"); break;
                default:                                    printf("reserved"); break;
            }
            break;

        case 1UL: // Type 1
            uint64 cache_id     = event_idx_code >> 3;
            uint64 op_id        = (event_idx_code >> 1) & 0b11UL;
            uint64 result_id    = event_idx_code & 1UL;

            printf("SBI_PMU_HW_CACHE(");
            switch(cache_id) {
                case SBI_PMU_HW_CACHE_L1D:  printf("L1D "); break;
                case SBI_PMU_HW_CACHE_L1I:  printf("L1I "); break;
                case SBI_PMU_HW_CACHE_LL:   printf("LL "); break;
                case SBI_PMU_HW_CACHE_DTLB: printf("DTLB "); break;
                case SBI_PMU_HW_CACHE_ITLB: printf("ITLB "); break;
                case SBI_PMU_HW_CACHE_BPU:  printf("BPU "); break;
                case SBI_PMU_HW_CACHE_NODE: printf("NODE "); break;
                default: printf("[invalid cache_id]"); break;
            }

            switch(op_id) {
                case SBI_PMU_HW_CACHE_OP_READ: printf("READ "); break;
                case SBI_PMU_HW_CACHE_OP_WRITE: printf("WRITE "); break;
                case SBI_PMU_HW_CACHE_OP_PREFETCH: printf("PREFETCH "); break;
                default: printf("[invalid op_id]"); break;
            }

            switch(result_id) {
                case SBI_PMU_HW_CACHE_RESULT_ACCESS: printf("ACCESS"); break;
                case SBI_PMU_HW_CACHE_RESULT_MISS: printf("MISS"); break;
                default: printf("[invalid result_id]"); break;
            }

            printf(")");

            break;

        case 2UL: // Type 2
            printf("SBI_PMU_HW_RAW(not specified)");
            break;

        case 15UL: // Type 15
            switch(event_idx_code) {
                case SBI_PMU_FW_MISALIGNED_LOAD:            printf("SBI_PMU_FW_MISALIGNED_LOAD"); break;
                case SBI_PMU_FW_MISALIGNED_STORE:           printf("SBI_PMU_FW_MISALIGNED_STORE"); break;
                case SBI_PMU_FW_ACCESS_LOAD:                printf("SBI_PMU_FW_ACCESS_LOAD"); break;
                case SBI_PMU_FW_ACCESS_STORE:               printf("SBI_PMU_FW_ACCESS_STORE"); break;
                case SBI_PMU_FW_ILLEGAL_INSN:               printf("SBI_PMU_FW_ILLEGAL_INSN"); break;
                case SBI_PMU_FW_SET_TIMER:                  printf("SBI_PMU_FW_SET_TIMER"); break;
                case SBI_PMU_FW_IPI_SENT:                   printf("SBI_PMU_FW_IPI_SENT"); break;
                case SBI_PMU_FW_IPI_RECEIVED:               printf("SBI_PMU_FW_IPI_RECEIVED"); break;
                case SBI_PMU_FW_FENCE_I_SENT:               printf("SBI_PMU_FW_FENCE_I_SENT"); break;
                case SBI_PMU_FW_FENCE_I_RECEIVED:           printf("SBI_PMU_FW_FENCE_I_RECEIVED"); break;
                case SBI_PMU_FW_SFENCE_VMA_SENT:            printf("SBI_PMU_FW_SFENCE_VMA_SENT"); break;
                case SBI_PMU_FW_SFENCE_VMA_RECEIVED:        printf("SBI_PMU_FW_SFENCE_VMA_RECEIVED"); break;
                case SBI_PMU_FW_SFENCE_VMA_ASID_SENT:       printf("SBI_PMU_FW_SFENCE_VMA_ASID_SENT"); break;
                case SBI_PMU_FW_SFENCE_VMA_ASID_RECEIVED:   printf("SBI_PMU_FW_SFENCE_VMA_ASID_RECEIVED"); break;
                case SBI_PMU_FW_HFENCE_GVMA_SENT:           printf("SBI_PMU_FW_HFENCE_GVMA_SENT"); break;
                case SBI_PMU_FW_HFENCE_GVMA_RECEIVED:       printf("SBI_PMU_FW_HFENCE_GVMA_RECEIVED"); break;
                case SBI_PMU_FW_HFENCE_GVMA_VMID_SENT:      printf("SBI_PMU_FW_HFENCE_GVMA_VMID_SENT"); break;
                case SBI_PMU_FW_HFENCE_GVMA_VMID_RECEIVED:  printf("SBI_PMU_FW_HFENCE_GVMA_VMID_RECEIVED"); break;
                case SBI_PMU_FW_HFENCE_VVMA_SENT:           printf("SBI_PMU_FW_HFENCE_VVMA_SENT"); break;
                case SBI_PMU_FW_HFENCE_VVMA_RECEIVED:       printf("SBI_PMU_FW_HFENCE_VVMA_RECEIVED"); break;
                case SBI_PMU_FW_HFENCE_VVMA_ASID_SENT:      printf("SBI_PMU_FW_HFENCE_VVMA_ASID_SENT"); break;
                case SBI_PMU_FW_HFENCE_VVMA_ASID_RECEIVED:  printf("SBI_PMU_FW_HFENCE_VVMA_ASID_RECEIVED"); break;
                case SBI_PMU_FW_PLATFORM:                   printf("SBI_PMU_FW_PLATFORM"); break;
                default:                                    printf("reserved"); break;
            }

            break;

        default: break;

    }
}

struct CounterSet sbi_pmu_test_start_counter(uint64 event_idx_0, uint64 event_idx_1, uint64 event_idx_2, uint64 event_idx_3) {

    struct CounterSet set;
    struct SbiRet ret;

    set.valid = false;
    set.event_idx_0 = event_idx_0;
    set.event_idx_1 = event_idx_1;
    set.event_idx_2 = event_idx_2;
    set.event_idx_3 = event_idx_3;
    set.is_hw_0 = isHardwareEvent(set.event_idx_0);
    set.is_hw_1 = isHardwareEvent(set.event_idx_1);
    set.is_hw_2 = isHardwareEvent(set.event_idx_2);
    set.is_hw_3 = isHardwareEvent(set.event_idx_3);
    

    // choose from all counters
    uint64 counter_base = 0UL;
    uint64 counter_mask = 0x1fffffffUL;

    printf("------------------------- SBI TEST PMU -------------------------\n");

    printf("\n");

    uint64 info;

    // Select
    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask, SBI_PMU_CFG_FLAG_CLEAR_VALUE, event_idx_0, 0UL);
    set.counter_idx_0 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Config] ERROR: %d (counter_idx_0 = %d)\n", ret.error, set.counter_idx_0);
        return set;
    }

    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask, SBI_PMU_CFG_FLAG_CLEAR_VALUE, event_idx_1, 0UL);
    set.counter_idx_1 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Config] ERROR: %d (counter_idx_1 = %d)\n", ret.error, set.counter_idx_1);
        return set;
    }

    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask, SBI_PMU_CFG_FLAG_CLEAR_VALUE, event_idx_2, 0UL);
    set.counter_idx_2 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Config] ERROR: %d (counter_idx_2 = %d)\n", ret.error, set.counter_idx_2);
        return set;
    }

    ret = sbi_pmu_counter_config_matching(counter_base, counter_mask, SBI_PMU_CFG_FLAG_CLEAR_VALUE, event_idx_3, 0UL);
    set.counter_idx_3 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Config] ERROR: %d (counter_idx_3 = %d)\n", ret.error, set.counter_idx_3);
        return set;
    }
        
    // Info
    ret = sbi_pmu_counter_get_info(set.counter_idx_0);
    uint64 info_0 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Info] ERROR: %d (counter_idx_0 = %d)\n", ret.error, set.counter_idx_0);
        return set;
    }

    ret = sbi_pmu_counter_get_info(set.counter_idx_1);
    uint64 info_1 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Info] ERROR: %d (counter_idx_1 = %d)\n", ret.error, set.counter_idx_1);
        return set;
    }

    ret = sbi_pmu_counter_get_info(set.counter_idx_2);
    uint64 info_2 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Info] ERROR: %d (counter_idx_2 = %d)\n", ret.error, set.counter_idx_2);
        return set;
    }

    ret = sbi_pmu_counter_get_info(set.counter_idx_3);
    uint64 info_3 = ret.value;
    if (ret.error != SBI_SUCCESS) {
        printf("    [Info] ERROR: %d (counter_idx_3 = %d)\n", ret.error, set.counter_idx_3);
        return set;
    }


    uint64 counter_value;
    printf("    EVENT : VALUE [IDX] CSR | WIDTH | HW(0)/FW(1)\n");

    printf("    ");
    print_evt(event_idx_0);
    
    if (set.is_hw_0) 
        counter_value = sbi_pmu_counter_hw_read(set.counter_idx_0);
    else 
        counter_value = sbi_pmu_counter_fw_read(set.counter_idx_0).value;

    printf(" : %d [%d] %x | %d | %d\n", counter_value, set.counter_idx_0, info_0 & 0xfffUL, (info_0 >> 12) & 0x3fUL, info_0 >> 63);

    printf("    ");
    print_evt(event_idx_1);
    
    if (set.is_hw_1) 
        counter_value = sbi_pmu_counter_hw_read(set.counter_idx_1);
    else 
        counter_value = sbi_pmu_counter_fw_read(set.counter_idx_1).value;

    printf(" : %d [%d] %x | %d | %d\n", counter_value, set.counter_idx_1, info_1 & 0xfffUL, (info_1 >> 12) & 0x3fUL, info_1 >> 63);

    printf("    ");
    print_evt(event_idx_2);
    
    if (set.is_hw_2) 
        counter_value = sbi_pmu_counter_hw_read(set.counter_idx_2);
    else 
        counter_value = sbi_pmu_counter_fw_read(set.counter_idx_2).value;

    printf(" : %d [%d] %x | %d | %d\n", counter_value, set.counter_idx_2, info_2 & 0xfffUL, (info_2 >> 12) & 0x3fUL, info_2 >> 63);

    printf("    ");
    print_evt(event_idx_3);
    
    if (set.is_hw_3) 
        counter_value = sbi_pmu_counter_hw_read(set.counter_idx_3);
    else 
        counter_value = sbi_pmu_counter_fw_read(set.counter_idx_3).value;

    printf(" : %d [%d] %x | %d | %d\n", counter_value, set.counter_idx_3, info_3 & 0xfffUL, (info_3 >> 12) & 0x3fUL, info_3 >> 63);


    // Start 
    ret = sbi_pmu_counter_start(set.counter_idx_0, 0b1UL, 0UL, 0UL);
    if (ret.error != SBI_SUCCESS) {
        printf("    [Start] ERROR: %d (counter_idx_0 = %d)\n", ret.error, set.counter_idx_0);
        return set;
    }
    ret = sbi_pmu_counter_start(set.counter_idx_1, 0b1UL, 0UL, 0UL);
    if (ret.error != SBI_SUCCESS) {
        printf("    [Start] ERROR: %d (counter_idx_1 = %d)\n", ret.error, set.counter_idx_1);
        return set;
    }
    ret = sbi_pmu_counter_start(set.counter_idx_2, 0b1UL, 0UL, 0UL);
    if (ret.error != SBI_SUCCESS) {
        printf("    [Start] ERROR: %d (counter_idx_2 = %d)\n", ret.error, set.counter_idx_2);
        return set;
    }
    ret = sbi_pmu_counter_start(set.counter_idx_3, 0b1UL, 0UL, 0UL);
    if (ret.error != SBI_SUCCESS) {
        printf("    [Start] ERROR: %d (counter_idx_3 = %d)\n", ret.error, set.counter_idx_3);
        return set;
    }


    printf("vvvvvvvvvvvvvvvvvvvvvvvvv measureframe vvvvvvvvvvvvvvvvvvvvvvvvv\n");

    set.valid = true;    

    return set;
}

void sbi_pmu_test_stop_counter(struct CounterSet set) {

    if (!set.valid) {
        printf("[sbi_pmu_test_stop] Trying to stop invalid set!\n");
        return;
    }

    printf("^^^^^^^^^^^^^^^^^^^^^^^^^ measureframe ^^^^^^^^^^^^^^^^^^^^^^^^^\n");

    struct SbiRet ret;

    printf("    EVENT : VALUE [IDX]\n");
    
    
    printf("    ");
    print_evt(set.event_idx_0);

    if (set.is_hw_0) {
        ret.error = SBI_SUCCESS;
        ret.value = sbi_pmu_counter_hw_read(set.counter_idx_0);
    } else {
        ret = sbi_pmu_counter_fw_read(set.counter_idx_0);
    }
    
    if (ret.error == SBI_SUCCESS) {    
        printf(" : %d [%d]\n", ret.value, set.counter_idx_0);
    } else {
        printf(" : ERROR %d [%d]\n", ret.error, set.counter_idx_0);
    }
    
    printf("    ");
    print_evt(set.event_idx_1);

    if (set.is_hw_1) {
        ret.error = SBI_SUCCESS;
        ret.value = sbi_pmu_counter_hw_read(set.counter_idx_1);
    } else {
        ret = sbi_pmu_counter_fw_read(set.counter_idx_1);
    }

    if (ret.error == SBI_SUCCESS) {    
        printf(" : %d [%d]\n", ret.value, set.counter_idx_1);
    } else {
        printf(" : ERROR %d [%d]\n", ret.error, set.counter_idx_1);
    }

    printf("    ");
    print_evt(set.event_idx_2);

    if (set.is_hw_2) {
        ret.error = SBI_SUCCESS;
        ret.value = sbi_pmu_counter_hw_read(set.counter_idx_2);
    } else {
        ret = sbi_pmu_counter_fw_read(set.counter_idx_2);
    }

    if (ret.error == SBI_SUCCESS) {    
        printf(" : %d [%d]\n", ret.value, set.counter_idx_2);
    } else {
        printf(" : ERROR %d [%d]\n", ret.error, set.counter_idx_2);
    }

    printf("    ");
    print_evt(set.event_idx_3);

    if (set.is_hw_3) {
        ret.error = SBI_SUCCESS;
        ret.value = sbi_pmu_counter_hw_read(set.counter_idx_3);
    } else {
        ret = sbi_pmu_counter_fw_read(set.counter_idx_3);
    }

    if (ret.error == SBI_SUCCESS) {    
        printf(" : %d [%d]\n", ret.value, set.counter_idx_3);
    } else {
        printf(" : ERROR %d [%d]\n", ret.error, set.counter_idx_3);
    }

    printf("\n");

    
    ret = sbi_pmu_counter_stop(set.counter_idx_0, 1UL, SBI_PMU_STOP_FLAG_RESET);
    if (ret.error != SBI_SUCCESS){
        printf("    [Stop] ERROR: %d (counter_idx = %d)\n", ret.error, set.counter_idx_0);
    }

    ret = sbi_pmu_counter_stop(set.counter_idx_1, 1UL, SBI_PMU_STOP_FLAG_RESET);
    if (ret.error != SBI_SUCCESS){
        printf("    [Stop] ERROR: %d (counter_idx = %d)\n", ret.error, set.counter_idx_1);
    }

    ret = sbi_pmu_counter_stop(set.counter_idx_2, 1UL, SBI_PMU_STOP_FLAG_RESET);
    if (ret.error != SBI_SUCCESS){
        printf("    [Stop] ERROR: %d (counter_idx = %d)\n", ret.error, set.counter_idx_2);
    }

    ret = sbi_pmu_counter_stop(set.counter_idx_3, 1UL, SBI_PMU_STOP_FLAG_RESET);
    if (ret.error != SBI_SUCCESS){
        printf("    [Stop] ERROR: %d (counter_idx = %d)\n", ret.error, set.counter_idx_3);
    }

    printf("------------------------- SBI TEST PMU -------------------------\n");

}

#endif