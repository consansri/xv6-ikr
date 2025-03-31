#ifndef __UART_H
#define __UART_H

#define UART_BUF_SIZE 256
#define NUM_UARTS 2

#define PRIMARY_UART 0
#define AUX_UART 1

struct UART_BUFFER{
  char uart_buf[UART_BUF_SIZE];
  int uart_w; // write next to uart_tx_buf[uart_tx_w++]
  int uart_r; // read next from uart_tx_buf[uar_tx_r++]
  struct spinlock lock;
};


void uartinit(void);
void uartstart(void);
void uartintr(void);
void aux_uart_reset_buffer(void);
#endif
