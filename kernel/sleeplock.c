// Sleeping locks


#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/proc.h"
#include "include/sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  push_off();
  while (lk->locked) {
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->pid = myproc()->pid;
  pop_off();
}

void
releasesleep(struct sleeplock *lk)
{
  push_off();
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk);
  pop_off();
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  push_off();
  r = lk->locked && (lk->pid == myproc()->pid);
  pop_off();
  return r;
}
