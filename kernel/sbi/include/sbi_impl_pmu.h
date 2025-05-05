#ifndef __SBI_IMPL_PMU_H
#define __SBI_IMPL_PMU_H

#include "sbi_impl.h"
#include <stdbool.h>

// DEBUG
//#define SBI_PMU_DEBUG

// --- Configuration ---
#define SBI_PMU_MAX_HW_COUNTERS     29UL // Maximum possible HW counters (mhpmcounter3 to mhpmcounter31)
#define SBI_PMU_COUNTER_NUM_FW      16UL // Number of firmware counters
// SBI_PMU_COUNTER_NUM is dynamic: actual_num_hw_counters + SBI_PMU_COUNTER_NUM_FW
#define SBI_PMU_COUNTER_WIDTH       64UL // Assumed width of counters
#define SBI_PMU_COUNTER_ADDR_U      0xc00 // Base user-mode CSR address (cycle) - Note: PMU spec uses 0xC03 for hpmcounter3 etc.
#define SBI_PMU_HW_COUNTER_CSR_BASE 0xC03 // CSR base for mhpmcounter3
#define SBI_PMU_HW_EVENT_CSR_BASE   0x323 // CSR base for mhpmevent3
#define SBI_PMU_HW_COUNTER_IDX_BASE 3     // Hardware counter CSRs start at index 3
#define SBI_PMU_NO_COUNTER_IDX      0xFFFFFFFFFFFFFFFFUL // Sentinel value for no counter found

// --- FLAGS ---

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

// --- EVENTS ---
// (Type  0) Common Hardware Events
#define SBI_PMU_HW_NO_EVENT                 0
#define SBI_PMU_HW_CPU_CYCLES               1
#define SBI_PMU_HW_INSTRUCTIONS             2
#define SBI_PMU_HW_CACHE_REFERENCES         3
#define SBI_PMU_HW_CACHE_MISSES             4
#define SBI_PMU_HW_BRANCH_INSTRUCTIONS      5
#define SBI_PMU_HW_BRANCH_MISSES            6
#define SBI_PMU_HW_BUS_CYCLES               7
#define SBI_PMU_HW_STALLED_CYCLES_FRONTEND  8
#define SBI_PMU_HW_STALLED_CYCLES_BACKEND   9
#define SBI_PMU_HW_REF_CPU_CYCLES           10

// (Type  1) Hardware Cache Events
#define SBI_PMU_EVT_TYPE_1                  (1 << 16)
// cache_id
#define SBI_PMU_HW_CACHE_L1D                0
#define SBI_PMU_HW_CACHE_L1I                1
#define SBI_PMU_HW_CACHE_LL                 2
#define SBI_PMU_HW_CACHE_DTLB               3
#define SBI_PMU_HW_CACHE_ITLB               4
#define SBI_PMU_HW_CACHE_BPU                5
#define SBI_PMU_HW_CACHE_NODE               6
// op_id
#define SBI_PMU_HW_CACHE_OP_READ            0
#define SBI_PMU_HW_CACHE_OP_WRITE           1
#define SBI_PMU_HW_CACHE_OP_PREFETCH        2
// result_id
#define SBI_PMU_HW_CACHE_RESULT_ACCESS      0
#define SBI_PMU_HW_CACHE_RESULT_MISS        1

#define SBI_PMU_HW_CACHE_EVENT(cahce, op, result) \
    (SBI_PMU_EVT_TYPE_1 | (cache) | (op) | (result))

// (Type 15) Firmware Events
#define SBI_PMU_EVT_TYPE_15                     (15 << 16)
#define SBI_PMU_FW_MISALIGNED_LOAD              (SBI_PMU_EVT_TYPE_15 | 0)
#define SBI_PMU_FW_MISALIGNED_STORE             (SBI_PMU_EVT_TYPE_15 | 1)
#define SBI_PMU_FW_ACCESS_LOAD                  (SBI_PMU_EVT_TYPE_15 | 2)
#define SBI_PMU_FW_ACCESS_STORE                 (SBI_PMU_EVT_TYPE_15 | 3)
#define SBI_PMU_FW_ILLEGAL_INSN                 (SBI_PMU_EVT_TYPE_15 | 4)
#define SBI_PMU_FW_SET_TIMER                    (SBI_PMU_EVT_TYPE_15 | 5)
#define SBI_PMU_FW_IPI_SENT                     (SBI_PMU_EVT_TYPE_15 | 6)
#define SBI_PMU_FW_IPI_RECEIVED                 (SBI_PMU_EVT_TYPE_15 | 7)
#define SBI_PMU_FW_FENCE_I_SENT                 (SBI_PMU_EVT_TYPE_15 | 8)
#define SBI_PMU_FW_FENCE_I_RECEIVED             (SBI_PMU_EVT_TYPE_15 | 9)
#define SBI_PMU_FW_SFENCE_VMA_SENT              (SBI_PMU_EVT_TYPE_15 | 10)
#define SBI_PMU_FW_SFENCE_VMA_RECEIVED          (SBI_PMU_EVT_TYPE_15 | 11)
#define SBI_PMU_FW_SFENCE_VMA_ASID_SENT         (SBI_PMU_EVT_TYPE_15 | 12)
#define SBI_PMU_FW_SFENCE_VMA_ASID_RECEIVED     (SBI_PMU_EVT_TYPE_15 | 13)
#define SBI_PMU_FW_HFENCE_GVMA_SENT             (SBI_PMU_EVT_TYPE_15 | 14)
#define SBI_PMU_FW_HFENCE_GVMA_RECEIVED         (SBI_PMU_EVT_TYPE_15 | 15)
#define SBI_PMU_FW_HFENCE_GVMA_VMID_SENT        (SBI_PMU_EVT_TYPE_15 | 16)
#define SBI_PMU_FW_HFENCE_GVMA_VMID_RECEIVED    (SBI_PMU_EVT_TYPE_15 | 17)
#define SBI_PMU_FW_HFENCE_VVMA_SENT             (SBI_PMU_EVT_TYPE_15 | 18)
#define SBI_PMU_FW_HFENCE_VVMA_RECEIVED         (SBI_PMU_EVT_TYPE_15 | 19)
#define SBI_PMU_FW_HFENCE_VVMA_ASID_SENT        (SBI_PMU_EVT_TYPE_15 | 20)
#define SBI_PMU_FW_HFENCE_VVMA_ASID_RECEIVED    (SBI_PMU_EVT_TYPE_15 | 21)
#define SBI_PMU_FW_PLATFORM                     (SBI_PMU_EVT_TYPE_15 | 65535)

// PMU Extension

// --- Structures ---

// Structure to hold the state of a single firmware counter
struct FirmwareCounterState {
    uint64 counter; // Current value
    uint64 event;   // Event ID configured for this counter (0 if none)
    bool active;    // True if the counter is currently counting
};

// --- SBI Initialization ---
/**
 * sbi_pmu_init()
 * Probes hardware counters and initializes the PMU extension state.
 * MUST be called once during SBI initialization before any other PMU functions.
 */
void sbi_pmu_init(void);

// --- SBI Function Implementations (Prototypes) ---
struct SbiRet sbi_pmu_num_counters_impl(void);
struct SbiRet sbi_pmu_counter_get_info_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_counter_config_matching_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data);
struct SbiRet sbi_pmu_counter_start_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value);
struct SbiRet sbi_pmu_counter_stop_impl(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags);
struct SbiRet sbi_pmu_counter_fw_read_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_counter_fw_read_hi_impl(uint64 counter_idx);
struct SbiRet sbi_pmu_snapshot_set_shmem_impl(uint64 shmem_phys_lo, uint64 shmem_phys_hi, uint64 flags);

// --- Firmware Event Counting Functions ---
// Specific event trigger functions (call sbi_pmu_fw_count)
void sbi_pmu_fw_count(uint64 event_idx); // Central counting function

void sbi_pmu_fw_misaligned_load();
void sbi_pmu_fw_misaligned_store();
void sbi_pmu_fw_access_load();
void sbi_pmu_fw_access_store();
void sbi_pmu_fw_illegal_insn();
void sbi_pmu_fw_set_timer();
void sbi_pmu_fw_ipi_sent();
void sbi_pmu_fw_ipi_received();
void sbi_pmu_fw_fence_i_sent();
void sbi_pmu_fw_fence_i_received();
void sbi_pmu_fw_sfence_vma_sent();
void sbi_pmu_fw_sfence_vma_received();
void sbi_pmu_fw_sfence_vma_asid_sent();
void sbi_pmu_fw_sfence_vma_asid_received();
void sbi_pmu_fw_hfence_gvma_sent();
void sbi_pmu_fw_hfence_gvma_received();
void sbi_pmu_fw_hfence_gvma_vmid_sent();
void sbi_pmu_fw_hfence_gvma_vmid_received();
void sbi_pmu_fw_hfence_vvma_sent();
void sbi_pmu_fw_hfence_vvma_received();
void sbi_pmu_fw_hfence_vvma_asid_sent();
void sbi_pmu_fw_hfence_vvma_asid_received();

// --- Helper functions ---

// Check counter index validity and type
bool isHardwareCounterIdx(uint64 idx);  // Check if index corresponds to a hardware counter
bool isFirmwareCounterIdx(uint64 idx);  // Check if index corresponds to a firmware counter
bool isValidCounterIdx(uint64 idx);     // Check if index is within the total valid range

// Check event type
bool isHardwareEvent(uint64 event_idx);
bool isFirmwareEvent(uint64 event_idx);

// Hardware CSR access helpers (using switch internally)
uint64 read_hw_counter(uint64 hw_counter_csr_idx); // Takes CSR index (3+)
uint64 read_hw_event_csr(uint64 hw_event_csr_idx); // Takes CSR index (3+)
void write_hw_counter(uint64 hw_counter_csr_idx, uint64 value); // Takes CSR index (3+)
void write_hw_event_csr(uint64 hw_event_csr_idx, uint64 value); // Takes CSR index (3+)

uint64 read_mcountinhibit(void);
void write_mcountinhibit(uint64 value);

/* For debugging */
void dump_hpm(uint64 counter_idx);
void dump_hpm_state(void);

#endif