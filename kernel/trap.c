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



extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
extern void kernelvec();

int devintr();

// from uart.c
extern volatile int use_sync_uart_for_console;

// void
// trapinit(void)
// {
//   initlock(&tickslock, "time");
//   #ifdef DEBUG
//   printf("trapinit\n");
//   #endif
// }

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  #ifdef DEBUG
    printf("kernelvec: %p  ", kernelvec);
  #endif
  w_stvec((uint64)kernelvec);
  w_sstatus(r_sstatus() | SSTATUS_SIE);
  set_next_timeout();
  // enable external interrupts and timer interrupts, in ikr-rv64i there are no software interrupts so dont even try enabling those 
  // off for testing rigth now
  w_sie(r_sie() | SIE_STIE | SIE_SEIE);

  use_sync_uart_for_console = 0;

  #ifdef DEBUG
  printf("trapinithart\n");
  #endif
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  // printf("run in usertrap\n");
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == EXC_ECALL_U){
    // system call
    if(p->killed)
      exit(-1);
    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;
    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();
    syscall();
  } 

  else if((which_dev = devintr()) != 0){
    // ok, was just a device craving some attention
  } 
  

  else {
    #ifdef USERFAULTFATAL
    printf("\nusertrap(): unexpected scause %p(%s) pid=%d %s\n", r_scause(), exception_str(r_scause()), p->pid, p->name);
    printf("            sepc=%p stval=%p\n => killed\n", r_sepc(), r_stval());
    trapframedump(p->trapframe);

    // DEBUG
    hexDump("virtual process memory ", r_sepc() - 0x20, 0x40);

    // find out where the physical page is to this process
    uint64 pa = walkaddr(p->pagetable, r_sepc());
    vmprint(p->pagetable);
    if(pa == NULL){
      printf("not mapped? (pt: %p)", p->pagetable);
      vmprint(p->pagetable);
      panic("bad");
    }
    

    hexDump("physical process memory ", (pa | (r_sepc() & 0xFFF)) - 0x20, 0x40);

    panic("bad");
    #else
    printf("<killed pid %d '%s' because of %s at %p due to %p>\n", p->pid, p->name, exception_str(r_scause()), r_sepc(), r_stval());
    #endif
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2){
    yield();
  }

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  // printf("[usertrapret]p->pagetable: %p\n", p->pagetable);
  //uint64 satp = MAKE_SATP(p->pagetable, p->pid * 2 + 2);
  uint64 satp = MAKE_SATP(p->pagetable, procnum(p) * 2 + 2);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap() {
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  struct proc *p = myproc();

  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");
  

  else if((which_dev = devintr()) == 0){
    sfence_vma();
    printf("\nscause %p(%s)\n", scause, exception_str(r_scause()));
    printf("sepc=%p stval=%p hart=%d\n", r_sepc(), r_stval(), r_tp());
    if (p != 0) {
      printf("pid: %d, name: %s\n", p->pid, p->name);
    }

    hexDump("kernel memory ", (r_sepc() - 0x20, 0x80));
    vmprint(p->kpagetable);
    panic("kerneltrap");

  }
  // printf("which_dev: %d\n", which_dev);
  
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING) {
    yield();
  }
  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

// Check if it's an external/software interrupt, 
// and handle it. 
// returns  2 if timer interrupt, 
//          1 if other device, 
//          0 if not recognized. 
int devintr(void) {
	uint64 scause = r_scause();

	// handle external interrupt (uart is directly hooked up to seip, plic does not get used rn)
	if ((0x8000000000000000L & scause) != 0) 
	{
    switch (scause & 0xff){
      // SEIP
      case 9:
        uartintr();
        return 1;
      // STIP
      case 5:
        //printf("tick!");
        timer_tick();
        return 2;
      // something else?
      default:
        return 0;
    }
	} else { 
    return 0;
  }
}



void trapframedump(struct trapframe *tf)
{
  printf("a0:  %p\t", tf->a0);
  printf("a1:  %p\t", tf->a1);
  printf("a2:  %p\t", tf->a2);
  printf("a3:  %p\n", tf->a3);
  printf("a4:  %p\t", tf->a4);
  printf("a5:  %p\t", tf->a5);
  printf("a6:  %p\t", tf->a6);
  printf("a7:  %p\n", tf->a7);
  printf("t0:  %p\t", tf->t0);
  printf("t1:  %p\t", tf->t1);
  printf("t2:  %p\t", tf->t2);
  printf("t3:  %p\n", tf->t3);
  printf("t4:  %p\t", tf->t4);
  printf("t5:  %p\t", tf->t5);
  printf("t6:  %p\t", tf->t6);
  printf("s0:  %p\n", tf->s0);
  printf("s1:  %p\t", tf->s1);
  printf("s2:  %p\t", tf->s2);
  printf("s3:  %p\t", tf->s3);
  printf("s4:  %p\n", tf->s4);
  printf("s5:  %p\t", tf->s5);
  printf("s6:  %p\t", tf->s6);
  printf("s7:  %p\t", tf->s7);
  printf("s8:  %p\n", tf->s8);
  printf("s9:  %p\t", tf->s9);
  printf("s10: %p\t", tf->s10);
  printf("s11: %p\t", tf->s11);
  printf("ra:  %p\n", tf->ra);
  printf("sp:  %p\t", tf->sp);
  printf("gp:  %p\t", tf->gp);
  printf("tp:  %p\t", tf->tp);
  printf("epc: %p\n", tf->epc);
}
