
#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/proc.h"
#include "include/intr.h"
#include "include/kalloc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/fat32.h"
#include "include/file.h"
#include "include/trap.h"
#include "include/vm.h"
#include <stdbool.h>

struct cpu cpus[NCPU];

struct proc procs[NPROC];

struct proc *initproc;


int nextpid = 1;
volatile int is_exit = 0;
struct spinlock pid_lock;

extern void forkret(void);
extern void swtch(struct context*, struct context*);
static void wakeup1(struct proc *chan);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

void reg_info(void) {
  printf("register info: {\n");
  printf("sstatus: %p\n", r_sstatus());
  printf("sip: %p\n", r_sip());
  printf("sie: %p\n", r_sie());
  printf("sepc: %p\n", r_sepc());
  printf("stvec: %p\n", r_stvec());
  printf("satp: %p\n", r_satp());
  printf("scause: %p\n", r_scause());
  printf("stval: %p\n", r_stval());
  printf("sp: %p\n", r_sp());
  printf("tp: %p\n", r_tp());
  printf("ra: %p\n", r_ra());
  printf("}\n");
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  for(p = procs; p < &procs[NPROC]; p++) {
      initlock(&p->lock, "proc");

      p->state = UNUSED;
  }
  //kvminithart();

  memset(cpus, 0, sizeof(cpus));
  #ifdef DEBUG
  printf("procinit\n");
  #endif
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  push_off();
  pid = nextpid;
  nextpid = (nextpid + 1) % 30000;
  pop_off();

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = procs; p < &procs[NPROC]; p++) {
    push_off();
    if(p->state == UNUSED) {
      goto found;
    } else {
      pop_off();
    }
  }

  return NULL;

found:
  p->pid = allocpid();


  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == NULL){
    pop_off();
    return NULL;
  }


  // An empty user page table.
  // And an identical kernel page table for this proc.
  if ((p->pagetable = proc_pagetable(p)) == NULL ||
      (p->kpagetable = proc_kpagetable()) == NULL) {
    freeproc(p);
    pop_off();
    return NULL;
  }

  p->kstack = VKSTACK;

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;



  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if (p->kpagetable) {
    kvmfree(p->kpagetable, 1);
  }

  p->kpagetable = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);

  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return NULL;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return NULL;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    vmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return NULL;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  vmunmap(pagetable, TRAMPOLINE, 1, 0);
  vmunmap(pagetable, TRAPFRAME, 1, 0);

  // also remove frame buffer mapping if we added it before
  if(walkaddr(pagetable, FBUFFER_UVA) != NULL){
    vmunmap(pagetable, FBUFFER_UVA, FRAME_BFR_SIZE/PGSIZE, 0);
  }

  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// uchar printhello[] = {
//     0x13, 0x00, 0x00, 0x00,     // nop
//     0x13, 0x00, 0x00, 0x00,     // nop 
//     0x13, 0x00, 0x00, 0x00,     // nop 
//     // <start>
//     0x17, 0x05, 0x00, 0x00,     // auipc a0, 0x0 
//     0x13, 0x05, 0x05, 0x00,     // mv a0, a0 
//     0x93, 0x08, 0x60, 0x01,     // li a7, 22 
//     0x73, 0x00, 0x00, 0x00,     // ecall 
//     0xef, 0xf0, 0x1f, 0xff,     // jal ra, <start>
//     // <loop>
//     0xef, 0x00, 0x00, 0x00,     // jal ra, <loop>
// };


// void test_proc_init(int proc_num) {
//   if(proc_num > NPROC) panic("test_proc_init\n");
//   struct proc *p;
//   for(int i = 0; i < proc_num; i++) {
//     p = allocproc();
//     uvminit(p->pagetable, (uchar*)printhello, sizeof(printhello));
//     p->sz = PGSIZE;
//     p->trapframe->epc = 0x0;
//     p->trapframe->sp = PGSIZE;
//     safestrcpy(p->name, "test_code", sizeof(p->name));
//     p->state = RUNNABLE;
//     pop_off();
//   }
//   initproc = proc;
//   printf("[test_proc]test_proc init done\n");
// }

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  #ifdef DEBUG
  printf("userinit - allocating mem for init");
  #endif

  p = allocproc();
  initproc = p;
  
  #ifdef DEBUG
  printf("userinit - alloc done");
  #endif

  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable , p->kpagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0x0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));

  // make sure icache loads initcode
  fence_i();

  #ifdef DEBUG
  printf("userinit - copied init\n");
  #endif

  p->state = RUNNABLE;

  p->tmask = 0;

  pop_off();
  #ifdef DEBUG
  printf("userinit\n");
  #endif
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();
  uint16 asid = procnum(p);

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, p->kpagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, p->kpagetable, sz, sz + n);
    // clear tlb from all entries with this asid
    // this is probably more efficient and also should only occur rarely
    sfence_vma_proc(asid);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == NULL){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, np->kpagetable, p->sz) < 0){
    freeproc(np);
    pop_off();
    return -1;
  }
  np->sz = p->sz;

  np->parent = p;

  // copy tracing mask from parent.
  np->tmask = p->tmask;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = edup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->state = RUNNABLE;

  pop_off();

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold p->lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = procs; pp < &procs[NPROC]; pp++){
    // this code uses pp->parent without holding pp->lock.
    // acquiring the lock first could cause a deadlock
    // if pp or a child of pp were also in exit()
    // and about to try to lock p.
    if(pp->parent == p){
      // pp->parent can't change between the check and the acquire()
      // because only the parent changes it, and we're the parent.
      push_off();
      pp->parent = initproc;
      // we should wake up init here, but that would require
      // initproc->lock, which would be a deadlock, since we hold
      // the lock on one of init's children (pp). this is why
      // exit() always wakes init (before acquiring any locks).
      pop_off();
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  eput(p->cwd);
  p->cwd = 0;

  
  // we might re-parent a child to init. we can't be precise about
  // waking up init, since we can't acquire its lock once we've
  // acquired any other proc lock. so wake up init whether that's
  // necessary or not. init may miss this wakeup, but that seems
  // harmless.
  push_off();
  wakeup1(initproc);
  pop_off();

  // grab a copy of p->parent, to ensure that we unlock the same
  // parent we locked. in case our parent gives us away to init while
  // we're waiting for the parent lock. we may then race with an
  // exiting parent, but the result will be a harmless spurious wakeup
  // to a dead or wrong process; proc structs are never re-allocated
  // as anything else.
  push_off();
  struct proc *original_parent = p->parent;
  pop_off();
  
  // we need the parent's lock in order to wake it up from wait().
  // the parent-then-child rule says we have to lock it first.
  push_off();

  push_off();

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup1(original_parent);

  p->xstate = status;
  p->state = ZOMBIE;

  pop_off();

  // Jump into the scheduler, never to return.

  // tell main kernel instance to clear tlbs for us
  sched(1);
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  // hold p->lock for the whole time to avoid lost
  // wakeups from a child's exit().
  push_off();

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = procs; np < &procs[NPROC]; np++){
      // this code uses np->parent without holding np->lock.
      // acquiring the lock first would cause a deadlock,
      // since np might be an ancestor, and we already hold p->lock.
      if(np->parent == p){
        // np->parent can't change between the check and the acquire()
        // because only the parent changes it, and we're the parent.
        push_off();
        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate, sizeof(np->xstate)) < 0) {
            pop_off();
            pop_off();
            return -1;
          }
          freeproc(np);
          pop_off();
          pop_off();
          return pid;
        }
        pop_off();
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      pop_off();
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &p->lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  extern pagetable_t kernel_pagetable;

  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    
    for(p = procs; p < &procs[NPROC]; p++) {
      push_off();
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        // printf("[scheduler]found runnable proc with pid: %d\n", p->pid);
        p->state = RUNNING;
        c->proc = p;
        //w_satp(MAKE_SATP(p->kpagetable, p->pid * 2 + 1));
        w_satp(MAKE_SATP(p->kpagetable, procnum(p) * 2 + 1));

        // switch to kernel instance of runnable proc
        swtch(&c->context, &p->context);
        // the kernel instance of this proc gave back control

        // if this processes exited, remove its entries from the tlbs
        if(is_exit){
            // clear all entries of the killed program in case they were still in the tlbs somewhere
            sfence_vma_proc(procnum(p) * 2 + 2);
            // also clear all entries related to its kernel instance
            sfence_vma_proc(procnum(p) * 2 + 1);
        }

        w_satp(MAKE_SATP(kernel_pagetable, 0));
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;

      }
      pop_off();
    }
    
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(int exit)
{
  int intena;
  struct proc *p = myproc();

  if(!true)
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;

  is_exit = exit;
  // give control to scheduler
  swtch(&p->context, &mycpu()->context);
  // we got control back

  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  push_off();
  p->state = RUNNABLE;
  sched(0);
  pop_off();
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  // printf("run in forkret\n");
  static int fat_ready = 0;

  // Still holding p->lock from scheduler.
  pop_off();

  if (fat_ready == 0) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    // printf("[forkret]first scheduling\n");
    fat_ready = 1;
    fat32_init();
    myproc()->cwd = ename("/");
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  if(lk != &p->lock){  //DOC: sleeplock0
    push_off();  //DOC: sleeplock1
    pop_off();
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched(0);

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &p->lock){
    pop_off();
    push_off();
  }
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = procs; p < &procs[NPROC]; p++) {
    push_off();
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
    }
    pop_off();
  }
}

// Wake up p if it is sleeping in wait(); used by exit().
// Caller must hold p->lock.
static void
wakeup1(struct proc *p)
{
  if(!true)
    panic("wakeup1");
  if(p->chan == p && p->state == SLEEPING) {
    p->state = RUNNABLE;
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = procs; p < &procs[NPROC]; p++){
    push_off();
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }

      pop_off();
      return 0;
    }
    pop_off();
  }



  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n ========= process list ==========\nPID\tSTATE\tNAME\tMEM\tPARENT\n");
  for(p = procs; p < &procs[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    if(p->parent != NULL){
      printf("%d\t%s\t%s\t", p->pid, state, p->name);
      print_size(p->sz);
      printf("\t%d", p->parent->pid);
    } else {
      printf("%d\t%s\t%s\t", p->pid, state, p->name);
      print_size(p->sz);
      printf("\t-");
    }
    printf("\n");
  }
  print_size(freemem_amount());
  printf(" free\n =================================\n", freemem_amount() >> 10);
}

uint64
procsnum(void)
{
  int num = 0;
  struct proc *p;

  for (p = procs; p < &procs[NPROC]; p++) {
    if (p->state != UNUSED) {
      num++;
    }
  }

  return num;
}


uint64
procnum(struct proc *p)
{
  return ((void*)p - (void*) &procs[0]) / sizeof(struct proc);
}


pagetable_t
get_proc_pagetable_by_pid(uint pid)
{
  struct proc *p;

  for (p = procs; p < &procs[NPROC]; p++) {
    if (p->pid = pid) {
      return p->pagetable;
    }
  }

  return NULL;
}