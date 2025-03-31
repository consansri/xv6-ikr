//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/console.h"
#include "include/printf.h"

volatile int panicked = 0;
static char digits[] = "0123456789abcdef";


// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int locking;
} pr;

void printstring(const char* s) {
    while (*s)
    {
        consputc(*s++);
    }
}


void printnibbles(uint64 val, int n)
{
  for(int i = n - 1; i > -1 ; i--){
    int nibbleval = (val >> (i * 4)) & 0xF;
    consputc(digits[nibbleval]);
  }
}


static void
printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}


static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  int locking;
  char *s;

  locking = pr.locking;
  if(locking)
    push_off();
  
  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }
  if(locking)
    pop_off();
}

void
panic(char *s)
{
  printf("panic: ");
  printf(s);
  printf("\n");
  procdump();
  backtrace();
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}

void backtrace()
{
  uint64 *fp = (uint64 *)r_fp();
  uint64 *bottom = (uint64 *)PGROUNDUP((uint64)fp);
  printf("backtrace:\n");
  while (fp < bottom) {
    uint64 ra = *(fp - 1);
    printf("%p\n", ra - 4);
    fp = (uint64 *)*(fp - 2);
  }
}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
  pr.locking = 1;   // changed, used to be 1
}



void print_logo() {
    #ifndef NOSIM
    printf("Xv6-IKR\n");
    printf("%s\n",__TIME__);     
    #else
    printf("\n");
    #ifdef TERMEMU
    printf("/$$    /H$            /$a$$$$        /$$s$$$ /$e   /$n /$$$$$$$       \n");
    #else
    printf("/$$    /$$            /$$$$$$        /$$$$$$ /$$   /$$ /$$$$$$$       \n");
    #endif
    printf("| $$  / $$           /$$__  $$      |_  $$_/| $$  /$$/| $$__  $$\n");
    printf("|  $$/ $$//$$    /$$| $$  \\__/        | $$  | $$ /$$/ | $$  \\ $$\n");
    printf(" \\  $$$$/|  $$  /$$/| $$$$$$$  /$$$$$$| $$  | $$$$$/  | $$$$$$$/\n");
    printf("  >$$  $$ \\  $$/$$/ | $$__  $$|______/| $$  | $$  $$  | $$__  $$\n");
    printf(" /$$/\\  $$ \\  $$$/  | $$  \\ $$        | $$  | $$\\  $$ | $$  \\ $$\n");
    printf("| $$  \\ $$  \\  $/   |  $$$$$$/       /$$$$$$| $$ \\  $$| $$  | $$\n");
    printf("|__/  |__/   \\_/     \\______/       |______/|__/  \\__/|__/  |__/ \n");
    printf("Build: %s %s\n", __DATE__, __TIME__);  
    #endif  
                                  
}
void
print_size(uint64 s)
{
  char *sizes[] = {"B", "kB", "MB", "GB"};

  uint64 dsize = s;
  uint64 last_dsize = s;
  int lvl;
  for(lvl = 0; dsize > 1024 && lvl < 4; lvl++){
    last_dsize = dsize;
    dsize = dsize >> 10;
  }
  if(lvl != 0)
    printf("%d.%d %s", dsize, (last_dsize / 100) % 100 % 10, sizes[lvl]);
  else
    printf("%d %s", dsize, sizes[lvl]);
};
  
  
void 
draw_spinner(uint64 prog, uint64 maxprog){
  if(prog % maxprog == 0){
    consputc(8);
    consputc('|');
  } else if(prog % maxprog == (maxprog / 4)){
    consputc(8);
    consputc('/');
  } else if((prog % maxprog) == 2 * (maxprog / 4)){
    consputc(8);
    consputc('-');
  } else if((prog % maxprog) == 3 * (maxprog / 4)){
    consputc(8);
    consputc(0x5c);
  }
 
}