#ifndef __SBI_CALL_H
#define __SBI_CALL_H

#include "../../include/types.h"
#include "sbi_dispatcher.h"

//#define SBI_CALLER_DEBUG

// RUNS IN S MODE

/*

SBI Functions

    a0: SbiRet error
    a1: SbiRet value

    a2-a5: must be preserved across an SBI call by the callee.

    a6: FID - encodes SBI function ID for a given EID
    a7: EID - encodes SBI extension ID

*/

/*
    Return value of every sbi function  
*/
struct SbiRet {
    uint64 error;
    uint64 value;
};

struct SbiRet sbi_ecall(
    uint64 arg0, 
    uint64 arg1, 
    uint64 arg2, 
    uint64 arg3, 
    uint64 arg4, 
    uint64 arg5,
    uint64 fid,
    uint64 eid
);

// ------------------ Base Extension                           EID #0x10

struct SbiRet sbi_get_spec_version(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 0, /*EID*/ 0x10);
}

struct SbiRet sbi_get_impl_id(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 1, /*EID*/ 0x10);
}

struct SbiRet sbi_get_impl_version(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 2, /*EID*/ 0x10);
}

struct SbiRet sbi_probe_extension(uint64 extension_id) {
    return sbi_ecall(extension_id, 0, 0, 0, 0, 0, /*FID*/ 3, /*EID*/ 0x10);
}

struct SbiRet sbi_get_mvendorid(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 4, /*EID*/ 0x10);
}

struct SbiRet sbi_get_marchid(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 5, /*EID*/ 0x10);
}

struct SbiRet sbi_get_mimpid(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 6, /*EID*/ 0x10);
}

// ------------------ Timer Extension                          EID #0x54494D45 "TIME"

struct SbiRet sbi_set_timer(uint64 stime_value) {
    return sbi_ecall(stime_value, 0, 0, 0, 0, 0, /*FID*/ 0, /*EID*/ 0x54494D45);
}

// ------------------ Performance Monitoring Unit Extension    EID #0x504D55 "PMU"
/*
    counter_idx:
        cycle                           : 0
        time                            : 1
        instret                         : 2
        configurable                    : 3-31


    csr addresses:
        M Level Counter Start Address   : 0xb00 + counter_idx
        M Level Event Start Address     : 0x320 + counter_idx
        U Level Counter Start Address   : 0xc00 + counter_idx
*/

/*
    @return Number of available configurable counters. (hardware + firmware)
*/
struct SbiRet sbi_pmu_num_counters(void) {
    return sbi_ecall(0, 0, 0, 0, 0, 0, /*FID*/ 0, /*EID*/ 0x504D55);
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
struct SbiRet sbi_pmu_counter_get_info(uint64 counter_idx) {
    return sbi_ecall(counter_idx, 0, 0, 0, 0, 0, /*FID*/ 1, /*EID*/ 0x504D55);
}

struct SbiRet sbi_pmu_counter_config_matching(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 config_flags, uint64 event_idx, uint64 event_data) {
    return sbi_ecall(counter_idx_base, counter_idx_mask, config_flags, event_idx, event_data, 0, /*FID*/ 2, /*EID*/ 0x504D55);
}

struct SbiRet sbi_pmu_counter_start(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 start_flags, uint64 initial_value) {
    return sbi_ecall(counter_idx_base, counter_idx_mask, start_flags, initial_value, 0, 0, /*FID*/ 3, /*EID*/ 0x504D55);
}

struct SbiRet sbi_pmu_counter_stop(uint64 counter_idx_base, uint64 counter_idx_mask, uint64 stop_flags) {
    return sbi_ecall(counter_idx_base, counter_idx_mask, stop_flags, 0, 0, 0, /*FID*/ 4, /*EID*/ 0x504D55);
}

struct SbiRet sbi_pmu_counter_fw_read(uint64 counter_idx) {
    return sbi_ecall(counter_idx, 0, 0, 0, 0, 0, /*FID*/ 5, /*EID*/ 0x504D55);
}

#endif

