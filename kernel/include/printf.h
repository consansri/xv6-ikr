#ifndef __PRINTF_H
#define __PRINTF_H

#include "types.h"

void printfinit(void);

void printf(char *fmt, ...);

void panic(char *s) __attribute__((noreturn));

void backtrace();

void print_logo();

void draw_spinner(uint64 prog, uint64 maxprog);



#endif 
