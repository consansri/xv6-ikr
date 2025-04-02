#ifndef __SBI_IMPL_PMU_H
#define __SBI_IMPL_PMU_H

#include "sbi_impl.h"

// DEBUG
//#define SBI_PMU_DEBUG


#define SBI_PMU_COUNTER_NUM         29
#define SBI_PMU_COUNTER_WIDTH       64
#define SBI_PMU_COUNTER_ADDR_U      0xc00

// FLAGS

// for FID #2
#define SBI_PMU_CFG_FLAG_SKIP_MATCH         (1UL << 0)
#define SBI_PMU_CFG_FLAG_CLEAR_VALUE        (1UL << 1)
#define SBI_PMU_CFG_FLAG_AUTO_START         (1UL << 2)
#define SBI_PMU_CFG_FLAG_SET_VUINH          (1UL << 3)
#define SBI_PMU_CFG_FLAG_SET_VSINH          (1UL << 4)
#define SBI_PMU_CFG_FLAG_SET_UINH           (1UL << 5)
#define SBI_PMU_CFG_FLAG_SET_SINH           (1UL << 6)
#define SBI_PMU_CFG_FLAG_SET_MINH           (1UL << 7)
#define SBI_PMU_CFG_RESERVED_MASK           (~((1UL << 8) - 1))

// for FID #3
#define SBI_PMU_START_SET_INIT_VALUE        (1UL << 0)
#define SBI_PMU_START_FLAG_INIT_SNAPSHOT    (1UL << 1)
#define SBI_PMU_START_RESERVED_MASK         (~((1UL << 2) - 1))

// for FID #4
#define SBI_PMU_STOP_FLAG_RESET             (1UL << 0)
#define SBI_PMU_STOP_FLAG_TAKE_SNAPSHOT     (1UL << 1)
#define SBI_PMU_STOP_RESERVED_MASK         (~((1UL << 2) - 1))

// for FID #7

// EVENTS
// type  0 
#define SBI_PMU_EVT_TYPE_0                  0
#define SBI_PMU_HW_NO_EVENT                 ((SBI_PMU_EVT_TYPE_0 << 16) | 0)
#define SBI_PMU_HW_CPU_CYCLES               ((SBI_PMU_EVT_TYPE_0 << 16) | 1)
#define SBI_PMU_HW_INSTRUCTIONS             ((SBI_PMU_EVT_TYPE_0 << 16) | 2)
#define SBI_PMU_HW_CACHE_REFERENCES         ((SBI_PMU_EVT_TYPE_0 << 16) | 3)
#define SBI_PMU_HW_CACHE_MISSES             ((SBI_PMU_EVT_TYPE_0 << 16) | 4)
#define SBI_PMU_HW_BRANCH_INSTRUCTIONS      ((SBI_PMU_EVT_TYPE_0 << 16) | 5)
#define SBI_PMU_HW_BRANCH_MISSES            ((SBI_PMU_EVT_TYPE_0 << 16) | 6)
#define SBI_PMU_HW_BUS_CYCLES               ((SBI_PMU_EVT_TYPE_0 << 16) | 7)
#define SBI_PMU_HW_STALLED_CYCLES_FRONTEND  ((SBI_PMU_EVT_TYPE_0 << 16) | 8)
#define SBI_PMU_HW_STALLED_CYCLES_BACKEND   ((SBI_PMU_EVT_TYPE_0 << 16) | 9)
#define SBI_PMU_HW_REF_CPU_CYCLES           ((SBI_PMU_EVT_TYPE_0 << 16) | 10)


// type  1

// type 15


// PMU Extension

/* 
 * We assume that hardware performance counters are available only for indices 3..31.
 * Their corresponding CSRs are:
 *   - mhpmcounter: CSR = 0xC00 + index  (e.g. mhpmcounter3 = 0xC03)
 *   - mhpmevent:   CSR = 0x320 + index  (e.g. mhpmevent3   = 0x323)
 *
 * For indices outside [3,31] we return SBI_ERR_INVALID_PARAM.
 */

struct SbiRet sbi_pmu_num_counters_impl(void);
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_counter_config_matching_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data);
struct SbiRet sbi_pmu_counter_start_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value);
struct SbiRet sbi_pmu_counter_stop_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags);
struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_counter_fw_read_hi_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_snapshot_set_shmem_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags);

// Helper functions

/* --- Assembly Helpers for CSR Access --- */
/*
 * Due to RISC-V requirements, the CSR number must be an immediate.
 * We therefore use a switch-case to select the correct CSR based on the counter index.
 */

uint64 read_hw_counter(uint64 idx);
uint64 read_hw_event(uint64 idx);
uint64 read_mcountinhibit(void);
void write_hw_counter(uint64 idx, uint64 value);
void write_hw_event(uint64 idx, uint64 value);
void write_mcountinhibit(uint64 value);


/* For debugging */
void dump_hpm(uint64 counter_idx);
void dump_hpm_state(void);

#endif