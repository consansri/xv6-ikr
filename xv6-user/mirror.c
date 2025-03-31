#include <sys/select.h>
#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/sysinfo.h"
#include "xv6-user/user.h"

int aux_uart_fd;

int get_chars(void* dst, int n) {
	int total = 0;
	int this_num;
	int time_out_ticks = 0;

	while(total != n){
		this_num = read(aux_uart_fd, (void * ) ((int) dst + total), n - total);

		total += this_num;
		if(total > n){
			return total;
		}
		if(this_num == 0){
			if(time_out_ticks < 5){
				time_out_ticks++;
				sleep(3);
			} else {
				return total;
			}
				
		} else {
			time_out_ticks = 0;
		}

	}

	return total;
}

int
main(int argc, char *argv[])
{

  /* open file to write recv file */
	aux_uart_fd = dev(O_RDWR, AUX_UART_DEV_ID, 0);
  char buf[133];

  while(1){
    printf("-%d-", get_chars(&buf, 133));
    write(aux_uart_fd, buf, 133);
    printf("\n");
    
  }

  exit(0);
}
