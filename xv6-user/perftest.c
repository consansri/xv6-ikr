#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

char *argv_usertests[] = { "/bin/usertests", "execout", 0};


uint64 hpm_vals[32];

uint64 get_hpm_value(int hpm_no) {
  uint64 x;
  switch (hpm_no){
    case 0:
      asm volatile("csrr %0, cycle" : "=r" (x));
      break;
    case 1:
      // this seems to be evil and need its own instruction
      //asm volatile("csrr %0, time" : "=r" (x));
      x = 0;
      break;
    case 2:
      // this one too
      //asm volatile("csrr %0, instret" : "=r" (x));
      x = 0;
      break;
    case 3:
      asm volatile("csrr %0, hpmcounter3" : "=r" (x));
      break;
    case 4:
      asm volatile("csrr %0, hpmcounter4" : "=r" (x));
      break;
    case 5:
      asm volatile("csrr %0, hpmcounter5" : "=r" (x));
      break;
    case 6:
      asm volatile("csrr %0, hpmcounter6" : "=r" (x));
      break;
    case 7:
      asm volatile("csrr %0, hpmcounter7" : "=r" (x));
      break;
    case 8:
      asm volatile("csrr %0, hpmcounter8" : "=r" (x));
      break;
    case 9:
      asm volatile("csrr %0, hpmcounter9" : "=r" (x));
      break;
    case 10:
      asm volatile("csrr %0, hpmcounter10" : "=r" (x));
      break;
    case 11:
      asm volatile("csrr %0, hpmcounter11" : "=r" (x));
      break;
    default:
      x = 0;
  }
  return x;
}


void
configure_tlb_settings(uint64 strategy, uint64 maxways)
{
  uint64 x = (strategy << 5) + maxways;
  asm volatile("csrw 2048, %0" : : "r" (x));
}

void
dump_hpm_value(int id, int fd)
{
  fprintf(fd, "hpm value: %d = %l\n", id, get_hpm_value(id));
  hpm_vals[id] = get_hpm_value(id);
}

void
dump_hpm_values(int max, int fd)
{
  if(max > 32)
    max = 32;

  fprintf(fd, "hpm values: \n");
  for(int i = 0; i < max; i++){
    fprintf(fd, "hpm%d : %l\n", i, get_hpm_value(i));
    hpm_vals[i] = get_hpm_value(i);
  }
}


uint64
get_hpm_delta(int id)
{
  return get_hpm_value(id) - hpm_vals[id];
}

void
store_hpm_values(int max)
{
  if(max > 32)
    max = 32;

  for(int i = 0; i < max; i++){
    hpm_vals[i] = get_hpm_value(i);
  }
}

void
store_hpm_value(int id)
{
  hpm_vals[id] = get_hpm_value(id);
}

void
dump_hpm_deltas(int max, int fd)
{
  if(max > 32)
    max = 32;

  fprintf(fd, "hpm deltas: \n");
  for(int i = 0; i < max; i++){
    fprintf(fd, "hpm%d : %l\n", i, get_hpm_value(i) - hpm_vals[i]);
  }
}

void
dump_hpm_delta(int id, int fd)
{
  fprintf(fd, "%d += %p", id, get_hpm_value(id) - hpm_vals[id]);
}

int
main(int argc, char *argv[])
{ 
    uint64 pid, wpid;
    int fd;

    configure_tlb_settings(1, 7);

    printf("PERFORMANCE TEST START!\n");
    printf("Build : %s %s\n", __DATE__, __TIME__);
    if((fd = open(argv[1], O_CREATE | O_RDWR)) < 0){
      printf("perftest: cannot open %s\n", argv[1]);
      exit(1);
    }

    fprintf(fd, "PERFORMANCE TEST REPORT\n");
    for(uint64 repl_strat = 3; repl_strat > 0; repl_strat--){
      printf("starting performance test for strategy %d\n", repl_strat);
      fprintf(fd, "starting performance test for strategy %d\n", repl_strat);
      for(uint64 waymask = 9; waymask != 0; waymask--){
        printf("waymask %d : \n", waymask);
        fprintf(fd, "waymask %d : ", waymask);
        //set replacement strategy
        configure_tlb_settings(repl_strat, waymask);

        store_hpm_value(6);
        store_hpm_value(8);
        store_hpm_value(9);
        store_hpm_value(10);

        pid = fork();
        if(pid < 0){
          printf("perftest: fork failed\n");
          exit(1);
        }
        if(pid == 0){
          exec("/bin/usertests", argv_usertests);
          printf("perftest: running usertests failed\n");
          exit(1);

        }

        wpid = wait((int *) 0);
        
        fprintf(fd, "%l/%l\n", get_hpm_delta(9), get_hpm_delta(10));
      }
    }

    printf("PERFORMANCE TEST COMPLETE!\n");
    close(fd);
    exit(0);
}
