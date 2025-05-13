#ifndef __SBI_IMPL_BASE_H
#define __SBI_IMPL_BASE_H

#include "sbi_impl.h"

#define SBI_SPEC_VERSION_MAJ        2
#define SBI_SPEC_VERSION_MIN        0
#define SBI_IMPL_ID                 0xC3
#define SBI_IMPL_VERSION_MAJ        0
#define SBI_IMPL_VERSION_MIN        1

struct SbiRet sbi_get_spec_version_impl(void);
struct SbiRet sbi_get_impl_id_impl(void);
struct SbiRet sbi_get_impl_version_impl(void);
struct SbiRet sbi_probe_extension_impl(uint64 extension_id);
struct SbiRet sbi_get_mvendorid_impl(void);
struct SbiRet sbi_get_marchid_impl(void);
struct SbiRet sbi_get_mimpid_impl(void);


#endif