#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

/*

update/compile: make fsimage

upload:
1. Close Minicom
2. CMD: make upload_x FILE=fs/bin/pmutest
3. Open Minicom


snapshot

reset


*/

enum EventType{
    NONE,                               // 0 No Event                                                                       - WORKING

    // Program Event Counter
    MISPREDICTION,                      // 1 Number of mispredictions (Count Misprediction Stalls or all mispredictions?)   - NOT TESTED
    EXCEPTION,                          // 2 Number of exceptions                                                           - NOT TESTED
    INTERRUPT,                          // 3 Number of interrupts                                                           - NOT TESTED

    // Instruction Type Counter
    OP_CSR,                             // 4 Number of csr operations                                                       - NOT TESTED
    JUMP,                               // 5 Number of jumps                                                               - NOT TESTED
    BRANCH,                             // 6 Number of branches                                                             - NOT TESTED
    Reserved0,                          // 7                                                                                - RESERVED
    OP_MULDIV,                          // 8 Number of mul/div operations                                                   - RESERVED
    
    // Pipeline Event Counter
    STALL,                              // 9 Number of stalls                                                               - NOT TESTED

    // IO Counter
    STALL_IO,                           // 10 Number of stalls triggered by io                                              - NOT TESTED

    // I€ Counter
    ICACHE_STALL,                       // 11 Number of stalls triggered by I€ MISSes                                       - NOT TESTED seems same as MISSes
    ICACHE_ACCESS,                      // 12 Number of I€ ACCESSes                                                         - NOT TESTED
    ICACHE_MISS,                        // 13 Number of I€ MISSes                                                           - NOT TESTED
    ICACHE_HIT_FILL_BUFFER,             // 14 Number of I€ Fill Buffer HITs                                                 - TODO
    ICACHE_ACCESS_NON_CACHEABLE,        // 15 Number of I€ Non Cacheable ACCESSes                                           - NOT TESTED
    ICACHE_RELEASE_FILL_BUFFER,         // 16 Number of I€ Fill Buffer releases                                             - TODO

    // D€ Counter
    DCACHE_STALL,                       // 17 Number of D€ stalls                                                           - NOT TESTED seems same as MISSes
    DCACHE_ACCESS_READ,                 // 18 Number of D€ READ ACCESSes                                                    - NOT TESTED
    DCACHE_ACCESS_WRITE,                // 19 Number of D€ WRITE ACCESSes                                                   - NOT TESTED
    DCACHE_ACCESS_ATOMIC,               // 20 Number of D€ ATOMIC ACCESSes                                                  - TODO
    DCACHE_ACCESS_NON_CACHED_READ,      // 21 Number of D€ non cached READ ACCESSes                                         - NOT TESTED seems same as READ MISSes
    DCACHE_ACCESS_NON_CACHED_WRITE,     // 22 Number of D€ non cached WRITE ACCESSes                                        - NOT TESTED
    DCACHE_MISS_READ,                   // 23 Number of D€ READ MISSes                                                      - NOT TESTED
    DCACHE_MISS_WRITE,                  // 24 Number of D€ WRITE MISSes                                                     - NOT TESTED
    DCACHE_MISS_ATOMIC,                 // 25 Number of D€ ATOMIC MISSes                                                    - TODO
    DCACHE_HIT_READ_FILL_BUFFER,        // 26 Number of D€ READ fill-buffer HITs                                            - TODO
    DCACHE_HIT_WRITE_FILL_BUFFER,       // 27 Number of D€ WRITE fill-buffer HITs                                           - TODO
    DCACHE_HIT_ATOMIC_FILL_BUFFER,      // 28 Number of D€ ATOMIC fill-buffer HITs                                          - TODO
    DCACHE_RELEASE_FILL_BUFFER,         // 29 Number of D€ fill-buffer releases                                             - TODO
    DCACHE_LINE_EVICTION,               // 30 Number of D€ line-evictions                                                   - TODO

    // ITLB Counter
    ITLB_STALL,                         // 31 Number of ITLB stalls                                                         - NOT TESTED
    ITLB_STALL_AND_NOT_DC_STALL,        // 32 Clocks spent waiting for itlb independent of dc cache performance             - NOT TESTED

    // DTLB Counter
    DTLB_STALL,                         // 33 Numberof DTLB stalls                                                          - NOT TESTED
    DTLB_STALL_AND_NOT_DC_STALL         // 34 Clocks spent waiting for dtlb independent of dc cache performance             - NOT TESTED
};


enum EventType evtForOrdinal(int ordinal) {
    for (enum EventType i = NONE; i <= DTLB_STALL_AND_NOT_DC_STALL; i++){
        if(i == ordinal) {
            return i;
        }
    }

    return NONE;
}

enum EventType get_hpm_event(int hpm_no) {
  int x = 0;
  switch (hpm_no){
    case 0:
      break;
    case 1:
      // this seems to be evil and need its own instruction
      //asm volatile("csrr %0, time" : "=r" (x));
      break;
    case 2:
      // this one too
      //asm volatile("csrr %0, instret" : "=r" (x));
      break;
    case 3:
      asm volatile("csrr %0, mhpmevent3" : "=r" (x));
      break;
    case 4:
      asm volatile("csrr %0, mhpmevent4" : "=r" (x));
      break;
    case 5:
      asm volatile("csrr %0, mhpmevent5" : "=r" (x));
      break;
    case 6:
      asm volatile("csrr %0, mhpmevent6" : "=r" (x));
      break;
    case 7:
      asm volatile("csrr %0, mhpmevent7" : "=r" (x));
      break;
    case 8:
      asm volatile("csrr %0, mhpmevent8" : "=r" (x));
      break;
    case 9:
      asm volatile("csrr %0, mhpmevent9" : "=r" (x));
      break;
    case 10:
      asm volatile("csrr %0, mhpmevent10" : "=r" (x));
      break;
    case 11:
      asm volatile("csrr %0, mhpmevent11" : "=r" (x));
      break;
    case 12:
      asm volatile("csrr %0, mhpmevent12" : "=r" (x));
      break;
    case 13:
      asm volatile("csrr %0, mhpmevent13" : "=r" (x));
      break;
    case 14:
      asm volatile("csrr %0, mhpmevent14" : "=r" (x));
      break;
    case 15:
      asm volatile("csrr %0, mhpmevent15" : "=r" (x));
      break;
    case 16:
      asm volatile("csrr %0, mhpmevent16" : "=r" (x));
      break;
    case 17:
      asm volatile("csrr %0, mhpmevent17" : "=r" (x));
      break;
    case 18:
      asm volatile("csrr %0, mhpmevent18" : "=r" (x));
      break;
    case 19:
      asm volatile("csrr %0, mhpmevent19" : "=r" (x));
      break;
    case 20:
      asm volatile("csrr %0, mhpmevent20" : "=r" (x));
      break;
    case 21:
      asm volatile("csrr %0, mhpmevent21" : "=r" (x));
      break;
    case 22:
      asm volatile("csrr %0, mhpmevent22" : "=r" (x));
      break;
    case 23:
      asm volatile("csrr %0, mhpmevent23" : "=r" (x));
      break;
    case 24:
      asm volatile("csrr %0, mhpmevent24" : "=r" (x));
      break;
    case 25:
      asm volatile("csrr %0, mhpmevent25" : "=r" (x));
      break;
    case 26:
      asm volatile("csrr %0, mhpmevent26" : "=r" (x));
      break;
    case 27:
      asm volatile("csrr %0, mhpmevent27" : "=r" (x));
      break;
    case 28:
      asm volatile("csrr %0, mhpmevent28" : "=r" (x));
      break;
    case 29:
      asm volatile("csrr %0, mhpmevent29" : "=r" (x));
      break;
    case 30:
      asm volatile("csrr %0, mhpmevent30" : "=r" (x));
      break;
    case 31:
      asm volatile("csrr %0, mhpmevent31" : "=r" (x));
      break;
    default:
      x = 0;
  }

  return evtForOrdinal(x);
}

/*

*/
void set(int counter, enum EventType event){

}


/*
    Returns the event name!
*/
char* evt_name(enum EventType evt_type) {
    switch(evt_type)
    {
        case NONE:   return "NONE";

        // Program Event Counter
        case MISPREDICTION: return "MISPREDICTION";
        case EXCEPTION:  return "EXCEPTION";
        case INTERRUPT:  return "INTERRUPT";

        // Instruction Type Counter
        case OP_CSR:     return "CSRs";
        case JUMP:       return "JMPs";
        case BRANCH:     return "BRAs";
        case Reserved0:  return "Reserved0";
        case OP_MULDIV:  return "MUL/DIV";
        
        // Pipeline Event Counter
        case STALL:      return "STALLs";

        // IO Counter
        case STALL_IO:   return "IO STALLs";

        // I€ Counter
        case ICACHE_STALL: return "ICache STALLs";
        case ICACHE_ACCESS: return "ICache Accesses";
        case ICACHE_MISS: return "ICache Misses";
        case ICACHE_HIT_FILL_BUFFER: return "ICache Fill Buffer Hits";
        case ICACHE_ACCESS_NON_CACHEABLE: return "ICache non cacheable Accesses";
        case ICACHE_RELEASE_FILL_BUFFER: return "ICache Fill Buffer Releases";

        // D€ Counter
        case DCACHE_STALL: return "DCache STALLs";
        case DCACHE_ACCESS_READ: return "DCache Read Accesses";
        case DCACHE_ACCESS_WRITE: return "DCache Write Accesses";
        case DCACHE_ACCESS_ATOMIC: return "DCache Atomic Accesses";
        case DCACHE_ACCESS_NON_CACHED_READ: return "DCache non cached Read Accesses";
        case DCACHE_ACCESS_NON_CACHED_WRITE: return "DCache non cached Write Accesses";
        case DCACHE_MISS_READ: return "DCache Read Misses";
        case DCACHE_MISS_WRITE: return "DCache Write Misses";
        case DCACHE_MISS_ATOMIC: return "DCache Atomic Misses";
        case DCACHE_HIT_READ_FILL_BUFFER: return "DCache Fill Buffer Read Hits";
        case DCACHE_HIT_WRITE_FILL_BUFFER: return "DCache Fill Buffer Write Hits";
        case DCACHE_HIT_ATOMIC_FILL_BUFFER: return "DCache Fill Buffer Atomic Hits";
        case DCACHE_RELEASE_FILL_BUFFER: return "DCache Fill Buffer Releases";
        case DCACHE_LINE_EVICTION: return "DCache Line Evictions";

        // ITLB Counter
        case ITLB_STALL:     
            return "ITLB STALLs";
        case ITLB_STALL_AND_NOT_DC_STALL: 
            return "Clocks spent waiting for itlb independent of dc cache performance";

        // DTLB Counter
        case DTLB_STALL: 
            return "DTLB STALLs";
        case DTLB_STALL_AND_NOT_DC_STALL: 
            return "Clocks spent waiting for dtlb independent of dc cache performance";

        default:
            return "NONE";
    }
}


/*

*/
void snapshot() {
    printf("\nPERF SNAPSHOT\n\n");
    for(int i = 3; i <= 31; i++){
        char* hpm_evt = "NONE"; //evt_name(get_hpm_event(i));
        uint64 hpm_value = get_hpm_value(i);
        printf(" %s : %d\n", hpm_evt, hpm_value);
    }

    printf("\n-------------\n");
}

void help() {

    printf(
        "HELP (Performance Monitoring):\n"
        "\n"
        " -s, --snapshot:     Take a snapshot of the performance counters.\n"
        " -h, --help:         Show some help.\n"
        "\n"
        "\n"
        );
}


int main(int argc, char *argv[]) {

    for (int i = 1; argv[i] != NULL && argv[i] != "\0"; i++) {

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            help();
            exit(0);
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--snapshot") == 0) {
            snapshot();
            exit(0);
        } else {
            printf("Error: Invalid Argument %s!\n", argv[i]);
            help();
            exit(0);
        }

    }

    printf("Error: Arguments missing!\n");
    help();
    exit(0);
}


