#ifndef __PLIC_H
#define __PLIC_H 

#include "memlayout.h"

#ifdef QEMU     // QEMU 
#define UART_IRQ    10 
#define DISK_IRQ    1
#else           // ikr rv64i on cyclone 10 gx
#define UART_IRQ    1
#define DISK_IRQ    2
#endif 

void plicinit(void);

// enable PLIC for each hart 
void plicinithart(void);

// ask PLIC what interrupt we should serve 
int plic_claim(void);

// tell PLIC that we've served this IRQ 
void plic_complete(int irq);

#endif 
