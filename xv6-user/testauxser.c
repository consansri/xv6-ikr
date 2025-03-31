
#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

int
main(void)
{
  int uart_fd = dev(O_RDWR, AUX_UART_DEV_ID, 0);

  printf("auxiliary uart test Build Time %s %s\n", __DATE__, __TIME__);
  printf("outputting string on auxiliary uart (has fd %d)\n", uart_fd);

  char teststr[] = "second test str\n";

  fprintf(uart_fd, "Hello from AUX!");

  printf("sent something");
  fprintf(uart_fd, "Hello from AUX!");
  write(uart_fd, teststr, sizeof(teststr));
  exit(0);
}
