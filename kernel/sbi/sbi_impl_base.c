#include "include/sbi_impl_base.h"

// BASE

struct SbiRet sbi_get_spec_version_impl(void) {

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = (SBI_SPEC_VERSION_MAJ << 24) | SBI_SPEC_VERSION_MIN;
    return sbiret;
}

struct SbiRet sbi_get_impl_id_impl(void) {

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = SBI_IMPL_ID;
    return sbiret;
}

struct SbiRet sbi_get_impl_version_impl(void) {

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = (SBI_IMPL_VERSION_MAJ << 24) | SBI_IMPL_VERSION_MIN;
    return sbiret;
}

struct SbiRet sbi_probe_extension_impl(uint64 extension_id) {

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;

    switch(extension_id) {
        case 0x10: // BASE
            sbiret.value = 1;
            break;

        case 0x504D55: // PMU
            sbiret.value = 1;
            break;

        default:
            sbiret.value = 0;
            break;
    }

    return sbiret;
}

struct SbiRet sbi_get_mvendorid_impl(void) {
    uint64 mvendorid;

    asm volatile(
        "csrr %0, mvendorid"
        :
        : "r" (mvendorid)
    );

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = mvendorid;

    return sbiret;
}

struct SbiRet sbi_get_marchid_impl(void) {

    uint64 marchid_val;

    asm volatile(
        "csrr %0, marchid"
        :
        : "r" (marchid_val)
    );

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = marchid_val;

    return sbiret;
}

struct SbiRet sbi_get_mimpid_impl(void) {

    uint64 mimpid;

    asm volatile(
        "csrr %0, mimpid"
        :
        : "r" (mimpid)
    );

    struct SbiRet sbiret;
    sbiret.error = SBI_SUCCESS;
    sbiret.value = mimpid;

    return sbiret;
}
