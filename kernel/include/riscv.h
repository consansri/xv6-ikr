#ifndef __RISCV_H
#define __RISCV_H

// which hart (core) is this?
static inline uint64
r_mhartid()
{
  uint64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.

static inline uint64
r_mstatus()
{
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void 
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void 
w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

// Supervisor Status Register, sstatus

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

static inline uint64
r_sstatus()
{
  uint64 x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void 
w_sstatus(uint64 x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

// Supervisor Interrupt Pending
static inline uint64
r_sip()
{
  uint64 x;
  asm volatile("csrr %0, sip" : "=r" (x) );
  return x;
}

static inline void 
w_sip(uint64 x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline uint64
r_sie()
{
  uint64 x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void 
w_sie(uint64 x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software
static inline uint64
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void 
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void 
w_sepc(uint64 x)
{
  asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64
r_sepc()
{
  uint64 x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

// Machine Exception Delegation
static inline uint64
r_medeleg()
{
  uint64 x;
  asm volatile("csrr %0, medeleg" : "=r" (x) );
  return x;
}

static inline void 
w_medeleg(uint64 x)
{
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

// Machine Interrupt Delegation
static inline uint64
r_mideleg()
{
  uint64 x;
  asm volatile("csrr %0, mideleg" : "=r" (x) );
  return x;
}

static inline void 
w_mideleg(uint64 x)
{
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.
static inline void 
w_stvec(uint64 x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64
r_stvec()
{
  uint64 x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

// Machine-mode interrupt vector
static inline void 
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

// use riscv's sv39 page table scheme.
#define SATP_SV39 (8L << 60)

#define MAKE_SATP(pagetable, asid) (SATP_SV39 | (((uint64)pagetable) >> 12) | ((uint64) asid << 44))

// supervisor address translation and protection;
// holds the address of the page table.
static inline void 
w_satp(uint64 x)
{
  asm volatile("csrw satp, %0" : : "r" (x));
}

static inline uint64
r_satp()
{
  uint64 x;
  asm volatile("csrr %0, satp" : "=r" (x) );
  return x;
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void 
w_sscratch(uint64 x)
{
  asm volatile("csrw sscratch, %0" : : "r" (x));
}

static inline void 
w_mscratch(uint64 x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Supervisor Trap Cause
static inline uint64
r_scause()
{
  uint64 x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

// Supervisor Trap Value
static inline uint64
r_stval()
{
  uint64 x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

// Machine-mode Counter-Enable
static inline void 
w_mcounteren(uint64 x)
{
  asm volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline uint64
r_mcounteren()
{
  uint64 x;
  asm volatile("csrr %0, mcounteren" : "=r" (x) );
  return x;
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Consti was here 03.02.2025

// Machine-mode Counter Write
static inline void 
w_mhpmcounter(int counter_id, uint64 x) {
  switch (counter_id) {
    case 3: asm volatile("csrw mhpmcounter3, %0" : "=r" (x)); break;
    case 4: asm volatile("csrw mhpmcounter4, %0" : "=r" (x)); break;
    case 5: asm volatile("csrw mhpmcounter5, %0" : "=r" (x)); break;
    case 6: asm volatile("csrw mhpmcounter6, %0" : "=r" (x)); break;
    case 7: asm volatile("csrw mhpmcounter7, %0" : "=r" (x)); break;
    case 8: asm volatile("csrw mhpmcounter8, %0" : "=r" (x)); break;
    case 9: asm volatile("csrw mhpmcounter9, %0" : "=r" (x)); break;
    case 10: asm volatile("csrw mhpmcounter10, %0" : "=r" (x)); break;
    case 11: asm volatile("csrw mhpmcounter11, %0" : "=r" (x)); break;
    case 12: asm volatile("csrw mhpmcounter12, %0" : "=r" (x)); break;
    case 13: asm volatile("csrw mhpmcounter13, %0" : "=r" (x)); break;
    case 14: asm volatile("csrw mhpmcounter14, %0" : "=r" (x)); break;
    case 15: asm volatile("csrw mhpmcounter15, %0" : "=r" (x)); break;
    case 16: asm volatile("csrw mhpmcounter16, %0" : "=r" (x)); break;
    case 17: asm volatile("csrw mhpmcounter17, %0" : "=r" (x)); break;
    case 18: asm volatile("csrw mhpmcounter18, %0" : "=r" (x)); break;
    case 19: asm volatile("csrw mhpmcounter19, %0" : "=r" (x)); break;
    case 20: asm volatile("csrw mhpmcounter20, %0" : "=r" (x)); break;
    case 21: asm volatile("csrw mhpmcounter21, %0" : "=r" (x)); break;
    case 22: asm volatile("csrw mhpmcounter22, %0" : "=r" (x)); break;
    case 23: asm volatile("csrw mhpmcounter23, %0" : "=r" (x)); break;
    case 24: asm volatile("csrw mhpmcounter24, %0" : "=r" (x)); break;
    case 25: asm volatile("csrw mhpmcounter25, %0" : "=r" (x)); break;
    case 26: asm volatile("csrw mhpmcounter26, %0" : "=r" (x)); break;
    case 27: asm volatile("csrw mhpmcounter27, %0" : "=r" (x)); break;
    case 28: asm volatile("csrw mhpmcounter28, %0" : "=r" (x)); break;
    case 29: asm volatile("csrw mhpmcounter29, %0" : "=r" (x)); break;
    case 30: asm volatile("csrw mhpmcounter30, %0" : "=r" (x)); break;
    case 31: asm volatile("csrw mhpmcounter31, %0" : "=r" (x)); break;
    default: break;
  }
}

// Machine-mode Counter Read
static inline uint64 
r_mhpmcounter(int counter_id) {
  uint64 x = 0;

  switch (counter_id) {
    case 3: asm volatile("csrr %0, mhpmcounter3" : "=r" (x)); break;
    case 4: asm volatile("csrr %0, mhpmcounter4" : "=r" (x)); break;
    case 5: asm volatile("csrr %0, mhpmcounter5" : "=r" (x)); break;
    case 6: asm volatile("csrr %0, mhpmcounter6" : "=r" (x)); break;
    case 7: asm volatile("csrr %0, mhpmcounter7" : "=r" (x)); break;
    case 8: asm volatile("csrr %0, mhpmcounter8" : "=r" (x)); break;
    case 9: asm volatile("csrr %0, mhpmcounter9" : "=r" (x)); break;
    case 10: asm volatile("csrr %0, mhpmcounter10" : "=r" (x)); break;
    case 11: asm volatile("csrr %0, mhpmcounter11" : "=r" (x)); break;
    case 12: asm volatile("csrr %0, mhpmcounter12" : "=r" (x)); break;
    case 13: asm volatile("csrr %0, mhpmcounter13" : "=r" (x)); break;
    case 14: asm volatile("csrr %0, mhpmcounter14" : "=r" (x)); break;
    case 15: asm volatile("csrr %0, mhpmcounter15" : "=r" (x)); break;
    case 16: asm volatile("csrr %0, mhpmcounter16" : "=r" (x)); break;
    case 17: asm volatile("csrr %0, mhpmcounter17" : "=r" (x)); break;
    case 18: asm volatile("csrr %0, mhpmcounter18" : "=r" (x)); break;
    case 19: asm volatile("csrr %0, mhpmcounter19" : "=r" (x)); break;
    case 20: asm volatile("csrr %0, mhpmcounter20" : "=r" (x)); break;
    case 21: asm volatile("csrr %0, mhpmcounter21" : "=r" (x)); break;
    case 22: asm volatile("csrr %0, mhpmcounter22" : "=r" (x)); break;
    case 23: asm volatile("csrr %0, mhpmcounter23" : "=r" (x)); break;
    case 24: asm volatile("csrr %0, mhpmcounter24" : "=r" (x)); break;
    case 25: asm volatile("csrr %0, mhpmcounter25" : "=r" (x)); break;
    case 26: asm volatile("csrr %0, mhpmcounter26" : "=r" (x)); break;
    case 27: asm volatile("csrr %0, mhpmcounter27" : "=r" (x)); break;
    case 28: asm volatile("csrr %0, mhpmcounter28" : "=r" (x)); break;
    case 29: asm volatile("csrr %0, mhpmcounter29" : "=r" (x)); break;
    case 30: asm volatile("csrr %0, mhpmcounter30" : "=r" (x)); break;
    case 31: asm volatile("csrr %0, mhpmcounter31" : "=r" (x)); break;
    default: break;
  }

  return x;
}

// Machine-mode Counter-Event Write
static inline void 
w_mhpmevent(int counter_id, uint64 x) {
  switch (counter_id) {
    case 3: asm volatile("csrw mhpmevent3, %0" : "=r" (x)); break;
    case 4: asm volatile("csrw mhpmevent4, %0" : "=r" (x)); break;
    case 5: asm volatile("csrw mhpmevent5, %0" : "=r" (x)); break;
    case 6: asm volatile("csrw mhpmevent6, %0" : "=r" (x)); break;
    case 7: asm volatile("csrw mhpmevent7, %0" : "=r" (x)); break;
    case 8: asm volatile("csrw mhpmevent8, %0" : "=r" (x)); break;
    case 9: asm volatile("csrw mhpmevent9, %0" : "=r" (x)); break;
    case 10: asm volatile("csrw mhpmevent10, %0" : "=r" (x)); break;
    case 11: asm volatile("csrw mhpmevent11, %0" : "=r" (x)); break;
    case 12: asm volatile("csrw mhpmevent12, %0" : "=r" (x)); break;
    case 13: asm volatile("csrw mhpmevent13, %0" : "=r" (x)); break;
    case 14: asm volatile("csrw mhpmevent14, %0" : "=r" (x)); break;
    case 15: asm volatile("csrw mhpmevent15, %0" : "=r" (x)); break;
    case 16: asm volatile("csrw mhpmevent16, %0" : "=r" (x)); break;
    case 17: asm volatile("csrw mhpmevent17, %0" : "=r" (x)); break;
    case 18: asm volatile("csrw mhpmevent18, %0" : "=r" (x)); break;
    case 19: asm volatile("csrw mhpmevent19, %0" : "=r" (x)); break;
    case 20: asm volatile("csrw mhpmevent20, %0" : "=r" (x)); break;
    case 21: asm volatile("csrw mhpmevent21, %0" : "=r" (x)); break;
    case 22: asm volatile("csrw mhpmevent22, %0" : "=r" (x)); break;
    case 23: asm volatile("csrw mhpmevent23, %0" : "=r" (x)); break;
    case 24: asm volatile("csrw mhpmevent24, %0" : "=r" (x)); break;
    case 25: asm volatile("csrw mhpmevent25, %0" : "=r" (x)); break;
    case 26: asm volatile("csrw mhpmevent26, %0" : "=r" (x)); break;
    case 27: asm volatile("csrw mhpmevent27, %0" : "=r" (x)); break;
    case 28: asm volatile("csrw mhpmevent28, %0" : "=r" (x)); break;
    case 29: asm volatile("csrw mhpmevent29, %0" : "=r" (x)); break;
    case 30: asm volatile("csrw mhpmevent30, %0" : "=r" (x)); break;
    case 31: asm volatile("csrw mhpmevent31, %0" : "=r" (x)); break;
    default: break;
  }
}

// Machine-mode Counter-Event Read
static inline uint64 
r_mhpmevent(int counter_id) {
  uint64 x = 0;
  switch (counter_id) {
    case 3: asm volatile("csrr %0, mhpmevent3" : "=r" (x)); break;
    case 4: asm volatile("csrr %0, mhpmevent4" : "=r" (x)); break;
    case 5: asm volatile("csrr %0, mhpmevent5" : "=r" (x)); break;
    case 6: asm volatile("csrr %0, mhpmevent6" : "=r" (x)); break;
    case 7: asm volatile("csrr %0, mhpmevent7" : "=r" (x)); break;
    case 8: asm volatile("csrr %0, mhpmevent8" : "=r" (x)); break;
    case 9: asm volatile("csrr %0, mhpmevent9" : "=r" (x)); break;
    case 10: asm volatile("csrr %0, mhpmevent10" : "=r" (x)); break;
    case 11: asm volatile("csrr %0, mhpmevent11" : "=r" (x)); break;
    case 12: asm volatile("csrr %0, mhpmevent12" : "=r" (x)); break;
    case 13: asm volatile("csrr %0, mhpmevent13" : "=r" (x)); break;
    case 14: asm volatile("csrr %0, mhpmevent14" : "=r" (x)); break;
    case 15: asm volatile("csrr %0, mhpmevent15" : "=r" (x)); break;
    case 16: asm volatile("csrr %0, mhpmevent16" : "=r" (x)); break;
    case 17: asm volatile("csrr %0, mhpmevent17" : "=r" (x)); break;
    case 18: asm volatile("csrr %0, mhpmevent18" : "=r" (x)); break;
    case 19: asm volatile("csrr %0, mhpmevent19" : "=r" (x)); break;
    case 20: asm volatile("csrr %0, mhpmevent20" : "=r" (x)); break;
    case 21: asm volatile("csrr %0, mhpmevent21" : "=r" (x)); break;
    case 22: asm volatile("csrr %0, mhpmevent22" : "=r" (x)); break;
    case 23: asm volatile("csrr %0, mhpmevent23" : "=r" (x)); break;
    case 24: asm volatile("csrr %0, mhpmevent24" : "=r" (x)); break;
    case 25: asm volatile("csrr %0, mhpmevent25" : "=r" (x)); break;
    case 26: asm volatile("csrr %0, mhpmevent26" : "=r" (x)); break;
    case 27: asm volatile("csrr %0, mhpmevent27" : "=r" (x)); break;
    case 28: asm volatile("csrr %0, mhpmevent28" : "=r" (x)); break;
    case 29: asm volatile("csrr %0, mhpmevent29" : "=r" (x)); break;
    case 30: asm volatile("csrr %0, mhpmevent30" : "=r" (x)); break;
    case 31: asm volatile("csrr %0, mhpmevent31" : "=r" (x)); break;
    default: break;
  }

  return x;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Consti was here 03.02.2025

// supervisor-mode cycle counter
static inline uint64
r_time()
{
  uint64 x;
  // asm volatile("csrr %0, time" : "=r" (x) );
  // this instruction will trap in SBI
  asm volatile("rdtime %0" : "=r" (x) );
  return x;
}

// enable device interrupts
static inline void
intr_on()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void
intr_off()
{
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// are device interrupts enabled?
static inline int
intr_get()
{
  uint64 x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

static inline uint64
r_sp()
{
  uint64 x;
  asm volatile("mv %0, sp" : "=r" (x) );
  return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline uint64
r_tp()
{
  uint64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void 
w_tp(uint64 x)
{
  asm volatile("mv tp, %0" : : "r" (x));
}

static inline uint64
r_ra()
{
  uint64 x;
  asm volatile("mv %0, ra" : "=r" (x) );
  return x;
}

static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}

// flush the TLB.
static inline void
sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  // asm volatile("sfence.vma zero, zero");
  asm volatile("sfence.vma");
}

// flush the TLB entries for specific ASID
static inline void
sfence_vma_proc(uint64 i)
{
  // the zero, zero means flush all TLB entries.
  // asm volatile("sfence.vma zero, zero");
  asm volatile("sfence.vma zero, %0" : : "r" (i));
}


static inline void
fence_i()
{
  // the zero, zero means flush all TLB entries.
  // asm volatile("sfence.vma zero, zero");
  asm volatile("fence.i");
}

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#define PTE_V (1L << 0) // valid
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4) // 1 -> user can access
#define PTE_S (1L << 8) // 1 -> page is swapped out 
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

typedef uint64 pte_t;
typedef uint64 *pagetable_t; // 512 PTEs

#endif