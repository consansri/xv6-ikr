#ifndef __SYSPMU_H
#define __SYSPMU_H

#include "types.h"
#include "../sbi/include/sbi_impl.h"

#define MAX_PMU_HANDLES         8
#define PMU_ACTION_START        1
#define PMU_ACTION_STOP         2
#define PMU_ACTION_READ         3
#define PMU_ACTION_STOP_READ    4


struct pmu_mapping {
    int valid;
    uint64 event_code;
    uint64 flags;
    uint64 counter_idx; // Physical counter index
};

void pmu_clear_config(struct proc*);

uint64 get_physical_mask(struct proc*, uint64 handle_mask);

// Helper to stop specific physical counters
// Uses the sbi_pmu_counter_stop wrapper from sbi_call.h
int stop_physical_counters(uint64 physical_mask);

// Helper to start specific physical counters with reset
// Uses the sbi_pmu_counter_start wrapper from sbi_call.h
int start_physical_counters_with_reset(uint64 physical_mask);

#endif
