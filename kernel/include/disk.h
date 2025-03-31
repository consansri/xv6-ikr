#ifndef __DISK_H
#define __DISK_H

#include "buf.h"

#define FS_BASE_SECTOR 0x20200
#define FS_SIZE_SECS 33 * 1024 * 4

void disk_init(void);
void disk_read(struct buf *b);
void disk_write(struct buf *b);
void disk_intr(void);
void disk_flush(void);

#endif
