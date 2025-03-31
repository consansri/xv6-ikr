
#include "include/perf.h"
#include "sbi/include/sbi_call.h"

void perf_init(void) {
    printf("init: PERF\n");
    
    
    
    struct SbiRet sbi_pmu_counter_get_info_ret = sbi_pmu_counter_get_info(4);
    struct SbiRet sbi_pmu_num_counters_ret = sbi_pmu_num_counters();
    
    printf(
        "PERF Test:\n"
        "   sbi_pmu_num_counters: %d\n"
        "   sbi_pmu_counter_get_info: %x\n",
        sbi_pmu_num_counters_ret.value,
        sbi_pmu_counter_get_info_ret.value
    );
}