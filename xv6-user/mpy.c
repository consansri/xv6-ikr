#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "xv6-user/user.h"

#include "xv6-user/micropython_embed/port/micropython_embed.h"

#define HEAPSZ 1024 * 64
#define INBUFSZ 128

int
main(int argc, char *argv[]) 
{
    // Initialise MicroPython.
    //
    // Note: &stack_top below should be good enough for many cases.
    // However, depending on environment, there might be more appropriate
    // ways to get the stack top value.
    // eg. pthread_get_stackaddr_np, pthread_getattr_np,
    // __builtin_frame_address/__builtin_stack_address, etc.
    printf("Xv6 Micropython Build %s %s\n", __DATE__, __TIME__);
    int stack_top;
    char* heap = malloc(HEAPSZ);
    if(heap == NULL)
        fprintf(2, "mpy: cannot allocate memory for heap!\n");
    
    printf("mpy: Heap is %d Bytes large\n", HEAPSZ);
    int fd, i;
    struct stat st;

    mp_embed_init(heap, HEAPSZ, &stack_top);

    if(argc == 1){
        char* input_str = malloc(INBUFSZ);

        while(1){
            printf(">>> ");
            gets(input_str, INBUFSZ);
            mp_embed_exec_str(input_str);
        }
    } else if(argc == 2) {
        printf("opening %s ", argv[1]);
        if((fd = open(argv[1], 0)) < 0){
            fprintf(2, "mpy: cannot open %s\n", argv[1]);
            exit(1);
        }

        if(fstat(fd, &st) < 0){
            fprintf(2, "mpy: cannot stat %s\n", argv[1]);
            close(fd);
        }
        printf("(is %d bytes large)\n", st.size);
        char* input_str = malloc(st.size);
        if(input_str == NULL)
            fprintf(2, "mpy: cannot allocate memory for script!\n");

        read(fd, input_str, st.size);
        mp_embed_exec_str(input_str);

    } else {
        fprintf(2, "usage: mpy [file]\n");
    }
    // Deinitialise MicroPython.
    mp_embed_deinit();

    exit(0);
}