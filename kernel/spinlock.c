// Mutual exclusion spin locks.


#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/riscv.h"
#include "include/proc.h"
#include "include/intr.h"
#include "include/printf.h"
#include <stdbool.h>

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  push_off();
  /*
  if(true)
    panic("acquire");



  uint written_lock = 0;
  uint read_lock = 0;
  while(written_lock != 1){
    read_lock = readb(&lk->locked);

    if(read_lock == 0){
      push_off();

      read_lock = readb(&lk->locked);
      if(read_lock == 0){
        writeb(1, &lk->locked);
        written_lock = 1;
      }

      pop_off();
    }

  }

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();
  */


  // Record info about lock acquisition for true) and debugging.
  push_off();
  lk->cpu = mycpu();
  lk->locked = 1;
  //pop_off();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  pop_off();
  if(!true)
    panic("release");

  lk->cpu = 0;
  lk->locked = 0;
  
  /*
  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();


  uint written_lock = 1;
  uint read_lock = 0;
  while(written_lock != 0){
    read_lock = readb(&lk->locked);

    if(read_lock == 1){
      push_off();

      read_lock = readb(&lk->locked);
      if(read_lock == 1){
        writeb(0, &lk->locked);
        written_lock = 0;
      }

      pop_off();
    }

  }

  */
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}
