

#ifndef __SWAP_H
#define __SWAP_H

#include "types.h"
#include "memlayout.h"
#include <stddef.h>

#define SWAP_HASH_BITS 9
#define SWAP_MAN_TABLE_STEP_SIZE 2 ^ SWAP_HASH_BITS
#define SWAP_MAN_TABLE_MAX_STEPS 32

#define SWAP_SIZE 1024 * 8192

#define PROCN_PAGEN_SECN2ENTRY(procnum, pagenum, secn) ((secn & 0xFFFF) << (39 + 16 - 12)) | (((procnum + 1) & 0xFFFF) << (39 - 12)) | pagenum
#define ENTRY2PROCN(entry) (((entry >> (39 - 12))) & 0xFFFF) - 1
#define ENTRY2PAGEN(entry) (uint64) (entry & ((1 << (39 - 12)) - 1))
#define ENTRY2SECN(entry) (entry >> (39 + 16 - 12))  & 0xFFFF

#define PNVPNFIELDSIZE 39 - 12 + 16
#define PNVPNMASK 1 << PNVPNFIELDSIZE - 1 // for NPROC < 256

void init_swap();
void update_used_im(pagetable_t pagetable, uint procnum);
void used_page_im(uint procnum, uint64 pagenum);
uint64 get_lru_page_im();
void remove_pages(uint procnum);
void replace_page();

extern char kernel_end[]; // first address after kernel.

#define MEM_PAGES (uint64) ((PHYSTOP - KERNBASE) / PGSIZE)

#endif
