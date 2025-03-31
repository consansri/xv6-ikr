#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/buf.h"
#include "include/flash.h"
#include "include/disk.h"
#include "include/printf.h"

void disk_init(void)
{
	flash_init();

    #ifdef RAMDISK
	printf("loading ramdisk...");

	uint8* dst = SYSTOP;
	uint64 i = 0;
	for(int sec = 0; sec < FS_SIZE_SECS; sec++){
		flash_read_sector_no_lock(dst, sec);
		draw_spinner(i, 2000);
		i++;
		dst += SECTOR_SIZE_BYTES;
	}
	printf("done\n");
	 
	#endif
}

void disk_read(struct buf *b)
{   
    #ifdef RAMDISK
    memmove(b->data, SYSTOP + BSIZE * b->sectorno, BSIZE);
    #else
	flash_read_sector(b->data, b->sectorno * 2);
    flash_read_sector(b->data + BSIZE/2, b->sectorno * 2 + 1);
    #endif
}

void disk_write(struct buf *b)
{
    #ifdef RAMDISK
    memmove(SYSTOP + BSIZE * b->sectorno, b->data, BSIZE);
    #else
	flash_write_sector(b->data, b->sectorno * 2);
    flash_write_sector(b->data + BSIZE/2, b->sectorno * 2 + 1);
    #endif
}

void disk_flush()
{
    #ifdef RAMDISK
    printf("\nflushing ramdisk to flash...");

	// erase required subsectors first
	for(int sec = 0; sec < FS_SIZE_SECS; sec += 16){
		flash_erase_subsector(sec * SECTOR_SIZE_BYTES);
	}

	uint8* src = SYSTOP;
	uint64 i = 0;
	for(int sec = 0; sec < FS_SIZE_SECS; sec++){
		flash_write_sector(src, sec);
		src += SECTOR_SIZE_BYTES;
		i++;
		draw_spinner(i, 2000);
	}
	printf("done\n");
    #endif
}

void disk_intr(void)
{
    //at some pointe one could hook up flash interrupt using PLIC
}
