#include "include/types.h"
#include "include/param.h"
#include "include/exception.h"


char* exception_str(uint64 xcause)
{
  static char *exc_strs[] = {
    [EXC_INSTR_ALIGN]       "Instruction address misaligned",
    [EXC_INSTR_ACCS]        "Instruction access fault",
    [EXC_INSTR_ILLEG]       "Illegal instruction",
    [EXC_BRK]               "Breakpoint",
    [EXC_LOAD_ALIGN]        "Load address misaligned",
    [EXC_LOAD_ACCS]         "Load access fault",
    [EXC_STORE_ALIGN]       "Store/AMO address misaligned",
    [EXC_STORE_ACCS]        "Store/AMO access fault",
    [EXC_ECALL_U]           "Environment call from U-mode",
    [EXC_ECALL_S]           "Environment call from S-mode",
    [10]                    "-",
    [EXC_ECALL_M]           "Environment call from M-mode",
    [EXC_INSTR_PAGE_FAULT]  "Instruction Page Fault",
    [EXC_LOAD_PAGE_FAULT]   "Load Page Fault",
    [14]                    "-",
    [EXC_STORE_PAGE_FAULT]  "Store Page Fault",
    [16]                    "???"
  };


  if(xcause < (uint64) 16){
    return exc_strs[xcause];
  } else {
    return exc_strs[16];
  }

}