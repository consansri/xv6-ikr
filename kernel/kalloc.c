// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.


#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"

void freerange(void *pa_start, void *pa_end);

extern char kernel_end[]; // first address after kernel.

struct run {
  struct run *next;
};

void* memset_d(void *dest, register int val, register size_t len){
  register uint64 *ptr = (uint64*)dest;
  while (len-- > 0){
    *ptr = val;
    ptr += 1;
  }
  return dest;

}

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 npage;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.freelist = 0;
  kmem.npage = 0;
  freerange(kernel_end, (void*)SYSTOP);
  #ifdef DEBUG
  printf("kernel_end: %p, systop: %p\n", kernel_end, (void*)SYSTOP);
  printf("kinit\n");
  #endif
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < kernel_end || (uint64)pa >= SYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset_d(pa, 1, PGSIZE/8);

  r = (struct run*)pa;

  push_off();
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.npage++;
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.npage--;
  }
  pop_off();

  #ifdef DEBUG
   //printf("kalloc(): %d left\n", kmem.npage);
   if(r == NULL)
    printf("kalloc(): no more space\n");
  #endif

  if(r)
    memset_d((char*)r, 1, PGSIZE/8); // fill with junk
  return (void*)r;
}

uint64
freemem_amount(void)
{
  return kmem.npage << PGSHIFT;
}
