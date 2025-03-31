#ifndef __MEMLAYOUT_H
#define __MEMLAYOUT_H

// ======================= I/O ======================
#define LEDS_ADDR               0x0000000000000000L
#define UART                    0x0000000000000001L
#define ULPI                    0x0000000000000004L

#define ACLINT_M                0x0000000000000018L
#define ACLINT_S                0x0000000000000080L

#define I2C_ADAPT               0x0000000000000028L
#define SII_CTRL                0x000000000000002AL

#define HDMI_CTRL               0x0000000000000030L
#define FRAME_BFR               0x0000000000100000L
#define FRAME_BFR_SIZE          0x00000000000080000

#define PLIC                    0x0000000000200100L
#define PLIC_PENDING            (PLIC + 0x1000)
#define PLIC_ENABLE(hart)       (PLIC + 0x2000 + (hart) * 0x100)
#define PLIC_PRIORITY(hart)     (PLIC + 0x200000 + (hart) * 0x2000)
#define PLIC_CLAIM(hart)        (PLIC + 0x200004 + (hart) * 0x2000)

#define FLASH_BFR               0x0000000000200000
#define FLASH_CTRL              0x00000000000000A0



// ======================= physical memory  ======================
// the kernel expects there to be RAM
// for use by the kernel and user pages
#define KERNBASE                0x40000000


#ifdef NOSIM
#define PHYSTOP                 0x42800000
#else
#define PHYSTOP                 0x40180000 // smallest viable system ram
#endif

#ifndef RAMDISK 
#define SYSTOP PHYSTOP
#else
#define SYSTOP PHYSTOP - 0x2100000 // 33 MB of RAMDISK space 
#endif


// ======================= virtual memory  ======================
// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE              (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
// #define KSTACK(p)               (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)
#define VKSTACK                 0x3EC0000000L

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME               (TRAMPOLINE - PGSIZE)

#define MAXUVA                  KERNBASE
#define FBUFFER_UVA             MAXUVA - FRAME_BFR_SIZE - PGSIZE

#endif
