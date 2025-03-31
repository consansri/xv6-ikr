#ifndef __EXCEPTION_H
#define __EXCEPTION_H

char* exception_str(uint64);

#define EXC_INSTR_ALIGN 0
#define EXC_INSTR_ACCS 1
#define EXC_INSTR_ILLEG 2
#define EXC_BRK 3
#define EXC_LOAD_ALIGN 4
#define EXC_LOAD_ACCS 5
#define EXC_STORE_ALIGN 6
#define EXC_STORE_ACCS 7
#define EXC_ECALL_U 8
#define EXC_ECALL_S 9
#define EXC_ECALL_M 11
#define EXC_INSTR_PAGE_FAULT 12
#define EXC_LOAD_PAGE_FAULT 13
#define EXC_STORE_PAGE_FAULT 15


#endif