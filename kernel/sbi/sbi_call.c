#include "include/sbi_call.h"

struct SbiRet sbi_ecall(
    uint64 arg0, 
    uint64 arg1, 
    uint64 arg2, 
    uint64 arg3, 
    uint64 arg4, 
    uint64 arg5,
    uint64 fid,
    uint64 eid
) {

    #ifdef SBI_CALLER_DEBUG
    printf(
        "SBI Caller -> SbiCall(eid = 0x%x, fid = %d, a0 = 0x%x, a1 = 0x%x, a2 = 0x%x, a3 = 0x%x, a4 = 0x%x, a5 = 0x%x)\n", 
        eid, fid, arg0, arg1, arg2, arg3, arg4, arg5
    );
    #endif

    struct SbiRet sbiret;

    register uint64 a0 asm("a0") = (uint64)(arg0);
    register uint64 a1 asm("a1") = (uint64)(arg1);
    register uint64 a2 asm("a2") = (uint64)(arg2);
    register uint64 a3 asm("a3") = (uint64)(arg3);
    register uint64 a4 asm("a4") = (uint64)(arg4);
    register uint64 a5 asm("a5") = (uint64)(arg5);
    register uint64 a6 asm("a6") = (uint64)(fid);
    register uint64 a7 asm("a7") = (uint64)(eid);

    asm volatile(
        "addi   sp, sp, -8\n"
	    "sd     ra, 0(sp)\n"
        "ecall\n"
        "ld     ra, 0(sp)\n"
	    "addi   sp, sp, 8"

        : "+r" (a0), "+r" (a1)
        : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
        : "memory", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "ra"
    );

    sbiret.error = a0;
    sbiret.value = a1;

    #ifdef SBI_CALLER_DEBUG
    printf("SBI Caller -> SbiRet(error = 0x%x, value = 0x%x)\n", sbiret.error, sbiret.value);    
    #endif

    return sbiret;
}



