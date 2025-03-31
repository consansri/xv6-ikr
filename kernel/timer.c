// Timer Interrupt handler


#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/timer.h"
#include "include/printf.h"
#include "include/proc.h"
#include "include/memlayout.h"


extern volatile int panicked;
struct spinlock tickslock;
uint64 ticks;

void timerinit() {
    initlock(&tickslock, "time");
    #ifdef DEBUG
    printf("timerinit\n");
    #endif
}

void
set_next_timeout() {
    uint64 current_ticks = readq(ACLINT_S);

    writeq(current_ticks + INTERVAL, ACLINT_S + 8); 
}


void
waitMs(uint64 ms)
{
    wait_clocks(ms * (SYS_CLK / 1000));
}

void
wait_clocks(uint64 cycles)
{
    uint64 start_clocks = readq(ACLINT_S);

    uint64 current_clocks;
    do{
        current_clocks = readq(ACLINT_S);
    } while(start_clocks + cycles > current_clocks);
}


void timer_tick() {
    push_off();
    ticks++;
    wakeup(&ticks);

    #ifdef TERMEMU
    if(panicked != 1 && ((ticks%20) == 0))
        temu_tick();
    #endif

    pop_off();
    set_next_timeout();
}
