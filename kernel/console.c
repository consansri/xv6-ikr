//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//
#include <stdarg.h>

#include "include/types.h"
#include "include/param.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/sleeplock.h"
#include "include/file.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/proc.h"
#include "include/uart.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

// from proc.c
extern struct proc *initproc;
extern struct proc procs[NPROC];



void consputc(int c) {
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc(PRIMARY_UART, '\b');
    uartputc(PRIMARY_UART, ' ');
    uartputc(PRIMARY_UART, '\b');
  } else {
    uartputc(PRIMARY_UART, c);
  }
}
struct {
  struct spinlock lock;
  
  // input
#define INPUT_BUF 256
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  push_off();
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(PRIMARY_UART, c);
  }
  pop_off();

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  push_off();
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(myproc()->killed){
        pop_off();
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  pop_off();

  return target - n;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c)
{
  push_off();

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  case C('U'):  // Kill line.
    while(cons.e != cons.w &&
          cons.buf[(cons.e-1) % INPUT_BUF] != '\n'){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  case C('C'):  // Kill all processes except init.
    struct proc *p;
    
    for(p = procs; p < &procs[NPROC]; p++) {
      if(p != initproc)
        p->killed = 1;
    }
    printf("\nkilled everything\n");
    break;

  case C('H'): // Backspace
  case '\x7f':
    if(cons.e != cons.w){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  default:
    if(c != 0 && cons.e-cons.r < INPUT_BUF){
      c = (c == '\r') ? '\n' : c;
      c = (c == '\f') ? '\n' : c;
      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      cons.buf[cons.e++ % INPUT_BUF] = c;

      if(c == '\n' || c == C('D') || cons.e == cons.r+INPUT_BUF){
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        cons.w = cons.e;
        wakeup(&cons.r);
      }
    }
    break;
  }
  
  pop_off();
}


void
console_input_wakeup()
{
  push_off();
  if(cons.e != cons.w){
    cons.w = cons.e;
    wakeup(&cons.r);
  }
  pop_off();
}


void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  cons.e = cons.w = cons.r = 0;
  
  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
