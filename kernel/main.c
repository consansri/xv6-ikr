// Copyright (c) 2006-2019 Frans Kaashoek, Robert Morris, Russ Cox,
//                         Massachusetts Institute of Technology

#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/console.h"
#include "include/printf.h"
#include "include/kalloc.h"
#include "include/timer.h"
#include "include/trap.h"
#include "include/proc.h"
#include "include/plic.h"
#include "include/vm.h"
#include "include/disk.h"
#include "include/buf.h"
#include "include/flash.h"
#include "include/uart.h"
#include "include/perf.h"
#include "sbi/include/sbi_tools.h"
#include <stdbool.h>

static inline void inithartid(unsigned long hartid) {
  asm volatile("mv tp, %0" : : "r" (hartid & 0x1));
}

volatile static int started = 0;
extern char kernel_end[]; // first address after kernel.


void
main(unsigned long hartid, unsigned long dtb_pa)
{
  inithartid(hartid);
  uartinit();
  // we only have a single hart
  consoleinit();
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '1');
  #endif

  printfinit();   // init a lock for printf 
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '2');
  #endif
  #ifdef UNIFORM_TIMING
  waituntilclockspassed(150000, 0);
  #endif

  #ifdef VIDEO
  video_init();
  #endif

  print_logo();
  #ifdef NOSIM
  printf("Usable RAM ");
  print_size((uint64)((void*)SYSTOP - (void*) kernel_end));
  printf("\n");
  #endif
  
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '3');
  #endif

  #ifdef DEBUG
  printf("hart %d enter main()...\n", hartid);
  #endif


  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '4');
  #endif
  kinit();         // physical page allocator
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '5');
  #endif
  kvminit();       // create kernel page table
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '6');
  #endif
  kvminithart();   // turn on paging
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '7');
  #endif
  timerinit();     // init a lock for timer

  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '8');
  #endif
  trapinithart();  // install kernel trap vector, including interrupt handler
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, '9');
  #endif
  procinit();      // initialise processes
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, 'A');
  #endif
  disk_init();     // initialise flash controller
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, 'B');
  #endif
  binit();         // buffer cache
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, 'C');
  #endif
  fileinit();      // file table
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, 'D');
  #endif
  userinit();      // first user process
  #ifdef SMALLDEBUG
  uartputc_sync(PRIMARY_UART, 'E');
  #endif
  
  #ifdef DEBUG
  printf("hart %d init done\n", hartid);
  #endif

  sbi_info();

  sbi_test_pmu();  

  // kernel -> user break
  printf("Kernel Startup finished.\n");
  scheduler();
}
