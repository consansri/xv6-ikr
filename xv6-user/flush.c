#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "xv6-user/user.h"

int main()
{
    if (flush_disk() < 0) {
        printf("flush fail!\n");
    }
    exit(0);
}
