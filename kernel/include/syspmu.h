#ifndef __SYSPMU_H
#define __SYSPMU_H

#include "types.h"
#include "proc.h"
#include "../sbi/include/sbi_impl.h"

//#define KERNEL_PMU_DEBUG

#define PMU_ACTION_START        1
#define PMU_ACTION_STOP         2
#define PMU_ACTION_READ         3
#define PMU_ACTION_STOP_READ    4

void pmu_clear_config(struct proc* p);

uint64 get_physical_mask(struct proc* p, uint64 handle_mask);

// Helper to stop specific physical counters
// Uses the sbi_pmu_counter_stop wrapper from sbi_call.h
int stop_physical_counters_with_reset(uint64 physical_mask);

// Helper to stop specific physical counters
// Uses the sbi_pmu_counter_stop wrapper from sbi_call.h
int stop_physical_counters(uint64 physical_mask);

// Helper to start specific physical counters with reset
// Uses the sbi_pmu_counter_start wrapper from sbi_call.h
int start_physical_counters_with_reset(uint64 physical_mask);

// Helper to start specific physical counters
// Uses the sbi_pmu_counter_start wrapper from sbi_call.h
int start_physical_counters(uint64 physical_mask);

// Helper to read specific hardware counter
uint64 hw_read_counter(uint64 hw_csr_addr);

// Syscall Implementation: pmu_setup
uint64 sys_pmu_setup(void);


// Syscall Implementation: pmu_control
uint64 sys_pmu_control(void);


#endif
