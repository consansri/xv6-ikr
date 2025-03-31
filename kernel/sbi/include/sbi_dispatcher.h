#ifndef __SBI_DISPATCHER_H
#define __SBI_DISPATCHER_H

#include "../../include/types.h"
#include "sbi_call.h"

//#define SBI_DISPATCHER_DEBUG

#define SBI_SPEC_VERSION_MAJ        3
#define SBI_SPEC_VERSION_MIN        0
#define SBI_IMPL_ID                 0xC3
#define SBI_IMPL_VERSION_MAJ        0
#define SBI_IMPL_VERSION_MIN        2

#define SBI_SUCCESS                 0
#define SBI_ERR_FAILED              -1
#define SBI_ERR_NOT_SUPPORTED       -2
#define SBI_ERR_INVALID_PARAM       -3
#define SBI_ERR_DENIED              -4
#define SBI_ERR_INVALID_ADDRESS     -5
#define SBI_ERR_ALREADY_AVAILABLE   -6
#define SBI_ERR_ALREADY_STARTED     -7
#define SBI_ERR_ALREADY_STOPPED     -8
#define SBI_ERR_NO_SHMEM            -9
#define SBI_ERR_INVALID_STATE       -10
#define SBI_ERR_BAD_RANGE           -11
#define SBI_ERR_TIMEOUT             -12
#define SBI_ERR_IO                  -13

// RUNS IN M MODE

struct SbiRet handle_sbi(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 fid, uint64 eid);

inline void print_sbi_info(void);

// ----------------------------------------------------------------- EXTENSIONS

// BASE

struct SbiRet sbi_get_spec_version_impl(void);
struct SbiRet sbi_get_impl_id_impl(void);
struct SbiRet sbi_get_impl_version_impl(void);
struct SbiRet sbi_probe_extension_impl(uint64 extension_id);
struct SbiRet sbi_get_mvendorid_impl(void);
struct SbiRet sbi_get_marchid_impl(void);
struct SbiRet sbi_get_mimpid_impl(void);

// PMU
struct SbiRet sbi_pmu_num_counters_impl(void);

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
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx);

struct SbiRet sbi_pmu_counter_config_matching(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data);

struct SbiRet sbi_pmu_counter_start(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value);

struct SbiRet sbi_pmu_counter_stop(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags);

struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx);

struct SbiRet sbi_pmu_counter_fw_read_hi(uint64 counter_idx);

struct SbiRet sbi_pmu_snapshot_set_shmem(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags);

struct SbiRet sbi_pmu_event_get_info(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 num_entries, uint64 flags);



// PMU Defines
#define SBI_PMU_COUNTER_NUM         32
#define SBI_PMU_COUNTER_WIDTH       64
#define SBI_PMU_COUNTER_ADDR_U      0xc00

#endif
