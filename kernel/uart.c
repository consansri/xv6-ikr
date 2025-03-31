
#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/proc.h"
#include "include/intr.h"
#include "include/uart.h"
#include "include/file.h"
#include "include/console.h"
#include <string.h>

// the UART control registers are memory-mappeduart_out_buffers
// at address UART0. this macro returns the
// address of one of the registers.
#define UART_Reg(reg) ((volatile unsigned char *)(UART + reg))
#define ULPI_Regh(reg) ((volatile uint16  *)(ULPI + reg))
#define ULPI_Reg(reg) ((volatile unsigned char *)(ULPI + reg))


#define UART_ReadReg(reg) (*(UART_Reg(reg)))
#define UART_WriteReg(reg, v) (*(UART_Reg(reg)) = (v))

#define ULPI_ReadReg(reg) (*(ULPI_Reg(reg)))
#define ULPI_ReadRegh(reg) (*(ULPI_Regh(reg)))
#define ULPI_WriteReg(reg, v) (*(ULPI_Reg(reg)) = (v))



struct UART_BUFFER uart_out_buffers[NUM_UARTS];
struct UART_BUFFER uart_in_buffers[NUM_UARTS];

char testbuf[16];

extern volatile int panicked; // from printf.c
volatile int use_sync_uart_for_console = 1;
int console_input_disabled = 0;



void ulpiwrreg(uint8 cmd, uint8 addr)
{
  ULPI_WriteReg(4, cmd);

  ULPI_WriteReg(13, addr);

  ULPI_WriteReg(7, cmd);

  uint16 ret;

  while(1){
    ret = ULPI_ReadRegh(6);

    if (ret == 0) {
      break;
    }
  }

};


void ulpireset()
{ 
  // turn of SOF gen
  ULPI_WriteReg(5, 0);
  uint16 ret;

  while(1){
    ret = ULPI_ReadRegh(6);

    if (ret == 0) {
      break;
    }
  }
  //1 RESET
  ulpiwrreg(0x85, 0x20);
  //2 DISCHARGE BUS
  ulpiwrreg(0x8B, 0x02);
  //3 DISABLE INTERRUPTS 1
  ulpiwrreg(0x8D, 0x02);
  //4 DISABLE INTERRUPTS 2
  ulpiwrreg(0x90, 0x02);
  //5 TURN ON CORRECT POWER SUPPLY INDICATOR
  ulpiwrreg(0x88, 0x40);

};



void
aux_uart_reset_buffer()
{
  uart_in_buffers[AUX_UART].uart_r = uart_in_buffers[AUX_UART].uart_w;
}


int
aux_uart_write(int user_src, uint64 src, int n)
{
  int i;

  push_off();
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(AUX_UART, c);
  }
  pop_off();

  return i;
}

int
aux_uart_read(int user_dst, uint64 dst, int n)
{
  uint target;
  char c;
  char cbuf;
  uint64 avail;

  target = n;
  push_off();
  while(n > 0){
    // nothing in buffer?
    if(uart_in_buffers[AUX_UART].uart_r == uart_in_buffers[AUX_UART].uart_w){
      if(myproc()->killed){
        pop_off();
        return -1;
      }
      #ifdef AUX_UART_BLOCKING
      sleep(&uart_in_buffers[AUX_UART].uart_r, &uart_in_buffers[AUX_UART].lock);
      #else
      pop_off();
      return target - n;
      #endif
    }

    #ifndef SINGLE_BYTE_XFER
    // fast multibyte copyout implementation
    if (uart_in_buffers[AUX_UART].uart_w > uart_in_buffers[AUX_UART].uart_r) {
      // no wrap case
      avail = uart_in_buffers[AUX_UART].uart_w - uart_in_buffers[AUX_UART].uart_r;
      if(avail > n)
        avail = n;
    
      if(either_copyout(user_dst, dst, uart_in_buffers[AUX_UART].uart_buf + uart_in_buffers[AUX_UART].uart_r , avail) == -1)
        break;

      uart_in_buffers[AUX_UART].uart_r = uart_in_buffers[AUX_UART].uart_r + avail;
    } else {
      // possible wraparound case

      // first get end of uart buffer
      avail = UART_BUF_SIZE - uart_in_buffers[AUX_UART].uart_r;
      if(avail > n)
        avail = n;

      if(either_copyout(user_dst, dst, &uart_in_buffers[AUX_UART].uart_buf[uart_in_buffers[AUX_UART].uart_r], avail) == -1)
        break;

      uart_in_buffers[AUX_UART].uart_r = (uart_in_buffers[AUX_UART].uart_r + avail ) % UART_BUF_SIZE;
    }
    n = n - avail;
    dst = dst + avail;

    #else

    // old single byte transfer implementation
    c = uart_in_buffers[AUX_UART].uart_buf[uart_in_buffers[AUX_UART].uart_r];
    uart_in_buffers[AUX_UART].uart_r = (uart_in_buffers[AUX_UART].uart_r + 1) % UART_BUF_SIZE;
    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;
    #endif
  }
  pop_off();

  return target - n;
}



void
uartinit(void)
{
  ulpireset();

  ulpiwrreg(0x84, 0x49);
  ulpiwrreg(0xB9, 0x00);
  ulpiwrreg(0x99, 0x0C);
  ulpiwrreg(0x88, 0x04);

  for(int i = 0; i < NUM_UARTS; i++){
    uart_out_buffers[i].uart_w = uart_out_buffers[i].uart_r = 0;
  }

  // clear pending characters
  while((UART_ReadReg(1) & 0x01) != 0){
    int c = UART_ReadReg(0);
    UART_WriteReg(1, 1);
  }


  // primary uart output
  initlock(&uart_out_buffers[PRIMARY_UART].lock, "uart_prim_tx");

  // auxiliary uart in and output
  initlock(&uart_in_buffers[AUX_UART].lock, "uart_aux_rx");
  initlock(&uart_out_buffers[AUX_UART].lock, "uart_aux_tx");


  // connect read and write system calls
  // to second uart
  devsw[AUX_UART_DEV_ID].read = aux_uart_read;
  devsw[AUX_UART_DEV_ID].write = aux_uart_write;
}


// add a character to the output buffer and tell the
// UART to start sending if it isn't already.
// blocks if the output buffer is full.
// because it may block, it can't be called
// from interrupts; it's only suitable for use
// by write().
void
uartputc(int channel, int c)
{
  push_off();

  if(panicked){
    for(;;)
      ;
  }

  #ifdef TERMEMU
  if(channel == PRIMARY_UART)
    video_temu_putc(c);
  #endif

  if(use_sync_uart_for_console == 1){
    uartputc_sync(channel, c);
    return;
  }

  while(1){
    if(((uart_out_buffers[channel].uart_w + 1) % UART_BUF_SIZE) == uart_out_buffers[channel].uart_r){
      // buffer is full.
      // wait for uartstart() to open up space in the buffer.
      sleep(&uart_out_buffers[channel].uart_r, &uart_out_buffers[channel].lock);
    } else {
      uart_out_buffers[channel].uart_buf[uart_out_buffers[channel].uart_w] = c;
      uart_out_buffers[channel].uart_w = (uart_out_buffers[channel].uart_w + 1) % UART_BUF_SIZE;
      uartstart();
      pop_off();
      return;
    }
  }
}

// alternate version of uartputc() that doesn't 
// use interrupts, for use by kernel printf() and
// to echo characters. it spins waiting for the uart's
// output register to be empty.
void
uartputc_sync(int channel, int c)
{
  // we currently ignore the channel id since we only have a single uart
  push_off();

  if(panicked){
    for(;;)
      ;
  }
  while((UART_ReadReg(2)) != 0)
    ;
  UART_WriteReg(0, c);

  pop_off();
}

// if the UART is idle, and a character is waiting
// in the transmit buffer, send it.
// called from both the top- and bottom-half.
void
uartstart()
{

  for(int ch_id = 0; ch_id < NUM_UARTS; ch_id++){
    while(1){
      if(uart_out_buffers[ch_id].uart_w == uart_out_buffers[ch_id].uart_r){
        // transmit buffer is empty.
        // no TX ready interrupt
        UART_WriteReg(2, 0);
        //return;
        break;
      }
      
      //while (UART_ReadReg(2) != 0){}


      if (UART_ReadReg(2) != 0){
        // the UART transmit holding register is full,
        // so we cannot give it another byte.
        // it will interrupt when it's ready for a new byte.
        
        // tell the uart we want to get notified when its ready for the next byte
        UART_WriteReg(2, 1);
        //return;
        break;
      } 
      
      int c = uart_out_buffers[ch_id].uart_buf[uart_out_buffers[ch_id].uart_r];
      uart_out_buffers[ch_id].uart_r = (uart_out_buffers[ch_id].uart_r + 1) % UART_BUF_SIZE;
      
      // maybe uartputc() is waiting for space in the buffer.
      wakeup(&uart_out_buffers[ch_id].uart_r);
      
      UART_WriteReg(0, c);
    }
  }
}

// read one input character from the UART.
// return -1 if none is waiting.

// currently ignores channel
int
uartgetc(int channel)
{
  if(channel == PRIMARY_UART){
    if((UART_ReadReg(1) & 0x01) != 0){
      int c = UART_ReadReg(0);

      // input data is ready.
      // clear rx avail bit
      UART_WriteReg(1, 0);
    
      return c;
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from trap.c.
void
uartintr(void)
{
  // send buffered characters.
  

  // read and process incoming characters for main uart
  int gotsome = 0;
  while(1){
    int c = uartgetc(PRIMARY_UART);
    if(c == -1)
      break;

    gotsome = 1;

    if(console_input_disabled == 0)
      consoleintr(c);

    // hack for direct console access
    uart_in_buffers[AUX_UART].uart_buf[uart_in_buffers[AUX_UART].uart_w] = c;
    uart_in_buffers[AUX_UART].uart_w = (uart_in_buffers[AUX_UART].uart_w + 1) % UART_BUF_SIZE;

  }
  if(gotsome == 1){
    wakeup(&uart_in_buffers[AUX_UART].uart_r);
  }
  push_off();
  uartstart();
  pop_off();
}
