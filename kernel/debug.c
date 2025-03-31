#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/proc.h"

#include "include/plic.h"
#include "include/trap.h"
#include "include/syscall.h"
#include "include/printf.h"
#include "include/console.h"
#include "include/timer.h"
#include "include/disk.h"
#include "include/exception.h"
#include "include/swap.h"
#include <stdbool.h>

uint64 prng = 0x1111111111111111;

uint64 lfsr_step(uint64 last){
  uint64 x = 1;
  uint64 poly = 0x1ad93d2635a0; // from https://users.ece.cmu.edu/~koopman/crc/crc64.html
  uint64 mask = 1;
  for(int i = 0; i < 64; i++){
    if((mask & last & poly) != 0){
      x = ~x;
    }
    mask = mask << 1;
  }

  return (last << 1) | (x & 1);
}





void
memtest(uint64 start_addr, uint64 len, bool zeroing, uint64 pass_size)
{
  printf("MEMORY TEST V0.0\n");
  // memory test

  uint64 bankasize = len / 2;
  // zero out memory regions
  if(zeroing){
    for(void* pt = start_addr; pt < start_addr + bankasize; pt = pt + 8){
      writeq(0, pt);
      writeq(0, pt + bankasize);
    }
    printf("clearing done\n");
  }


  uint64 bankaval;
  uint64 bankbval;
  uint64 check_addr;
  uint64 total_bytes = 0;
  uint64 prng2 = 0x310016;
  prng = 0x1111111111111111;

  for(uint64 n = 0; n < 10000; n++){
    printf("Starting pass %d ... ", n);

    //prng = 0x1111111111111111;
    // fill the regions with random data
    printf("fill  ");
    for(uint64 i = 0; i < pass_size; i++){
      prng = lfsr_step(prng);

      check_addr = start_addr + (prng & (bankasize - 1));

      prng2 = lfsr_step(prng2);

      if(check_addr % 8 == 0){
        writeq(prng2, check_addr);
        writeq(prng2, check_addr + bankasize);
        total_bytes += 16;
      } else if (check_addr % 4 == 0) {
        writed((uint32) prng2, check_addr);
        writed((uint32) prng2, check_addr + bankasize);
        total_bytes += 8;
      } else if (check_addr % 2 == 0) {
        writew((uint16) prng2, check_addr);
        writew((uint16) prng2, check_addr + bankasize);
        total_bytes += 4;
      } else {
        writeb((uint8) prng2, check_addr);
        writeb((uint8) prng2, check_addr + bankasize);
        total_bytes += 2;
      }

      draw_spinner(i, pass_size/5);

    }
    consputc(8);

    prng = 0x1111111111111111;
    printf(" verify  ");
    // verify regions contain the same data
    for(uint64 i = 0; i < pass_size; i++){
      prng = lfsr_step(prng);

      check_addr = start_addr + (prng & (bankasize - 1));
      


      if(check_addr % 8 == 0){
        bankaval = readq(check_addr);
        bankbval = readq(check_addr + bankasize);
        //consputc('d');
      } else if (check_addr % 4 == 0) {
        bankaval = (uint64) readd(check_addr);
        bankbval = (uint64)readd(check_addr + bankasize);
        //consputc('w');
      } else if (check_addr % 2 == 0) {
        bankaval = (uint64)readw(check_addr);
        bankbval = (uint64)readw(check_addr + bankasize);
        //consputc('h');
      } else {
        bankaval = (uint64)readb(check_addr);
        bankbval = (uint64)readb(check_addr + bankasize);
        //consputc('b');
      }
      //printf("%p , %p\n", bankaval, bankbval);

      if(bankaval != bankbval){
        printf("\n\n%p(%p) and %p(%p) differ!\n", bankaval, check_addr, bankbval, check_addr + bankasize);
        panic(" VERY BAD ");
      }
      
      draw_spinner(i, pass_size/5);
    }
    consputc(8);
    printf("done. (last value %p)\n", bankaval);

    if(n % 100 == 0){
      printf("total bytes written %d\n", total_bytes);
    }
  };
  printf("\ntotal bytes written: %d\n", total_bytes);
  panic("done");
}



void
waituntilclockspassed(uint64 clocks, uint64 start)
{
  uint64 current_ticks = readq(ACLINT_S);

  if(current_ticks > start + clocks){
    panic("ulpi init took too long");
  }

  while(1){
    current_ticks = readq(ACLINT_S);

    if(current_ticks > start + clocks)
      break;
  }
}



// from https://gist.github.com/domnikl/af00cc154e3da1c5d965
void hexDump(char *desc, void *addr, int len) 
{
    int i;
    unsigned char *pc = (unsigned char*) addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if((unsigned char*)addr < pc + len)
      len += 16;
    
    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("\n");

            // Output the offset.
            printf("  ");
            printnibbles(i + pc, 10);
            printf(" ");
        }

        // Now the hex code for the specific character.
        printf(" ");
        printnibbles(pc[i], 2);

    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    printf("\n");


}


void
set_leds(uint8 val)
{
  writeb(val, LEDS_ADDR);
}