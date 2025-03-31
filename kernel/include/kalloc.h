#ifndef __KALLOC_H
#define __KALLOC_H

#include "types.h"
#include <stddef.h>

void*           kalloc(void);
void            kfree(void *);
void            kinit(void);
uint64          freemem_amount(void);
void* memset_d(void *dest, register int val, register size_t len);

#endif