
#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("test framebuffer user access\n");
		printf("Build: %s %s\n", __DATE__, __TIME__);
		printf("usage: %s <colid>/c\n", argv[0]);
		exit(-1);
	}

  uint64 fbuf = frame();

  if(argv[1][0] == 'c') {
    int colval = 0;

    while(1){
      memset(fbuf, colval + colval * 16, 0x80000);
      colval = (colval+1) % 16;
      printf("set up color value %d\n", colval);
      sleep(10);
    }
    
  } else {
    int colval = atoi(argv[1]);
    
    printf("frame buffer at %p\n", fbuf);
    memset(fbuf, colval + colval * 16, 0x80000);
    printf("set up color value %d\n", colval);
  }
  exit(0);
}
