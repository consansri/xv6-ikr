#ifndef __SBI_DISPATCHER_H
#define __SBI_DISPATCHER_H

#include "../../include/types.h"
#include "sbi_impl_base.h"
#include "sbi_impl_pmu.h"
#include "sbi_call.h"

//#define SBI_DISPATCHER_DEBUG

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

#endif
