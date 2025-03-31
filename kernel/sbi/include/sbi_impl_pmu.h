#ifndef __SBI_IMPL_PMU_H
#define __SBI_IMPL_PMU_H

#include "sbi_impl.h"

#define SBI_PMU_COUNTER_NUM         29
#define SBI_PMU_COUNTER_WIDTH       64
#define SBI_PMU_COUNTER_ADDR_U      0xc00

#define SBI_PMU_SNAPSHOT_FLAG_ENABLE  (1UL << 0)
#define SBI_PMU_SNAPSHOT_FLAG_CLEAR   (1UL << 1)

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

/* --- Inline Assembly Helpers for CSR Access --- */
/* 
 * Because the CSR number must be an immediate in inline assembly, we implement 
 * read/write helpers via switch-case.
 */

/* --- Inline Assembly Helpers for CSR Access --- */
/*
 * Due to RISC-V requirements, the CSR number must be an immediate.
 * We therefore use a switch-case to select the correct CSR based on the counter index.
 */

inline uint64 read_hw_counter(uint64 idx);
inline void write_hw_counter(uint64 idx, uint64 value);
inline void write_hw_event(uint64 idx, uint64 value);

/* Inline helpers for mcountinhibit CSR */
inline uint64 read_mcountinhibit(void);
inline void write_mcountinhibit(uint64 val);

#endif