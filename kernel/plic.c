
#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"

#include "include/plic.h"
#include "include/proc.h"
#include "include/printf.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void plicinit(void) {
	writed(1, PLIC + DISK_IRQ * sizeof(uint32));
	writed(1, PLIC + UART_IRQ * sizeof(uint32));

	#ifdef DEBUG 
	printf("plicinit\n");
	#endif 
}

void
plicinithart(void)
{

  // set uart's enable bit for this hart's S-mode. 
  *(uint32*)PLIC_ENABLE(0)= (1 << UART_IRQ) | (1 << DISK_IRQ);
  // set this hart's priority threshold to 0.
  *(uint32*)PLIC_PRIORITY(0) = 0;

  #ifdef DEBUG
  printf("plicinithart\n");
  #endif
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int irq = *(uint32*)PLIC_CLAIM(0);

  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  *(uint32*)PLIC_CLAIM(0) = irq;

}

