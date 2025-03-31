#ifndef __FLASH_H
#define __FLASH_H

#define FLASH_STATUS_REG_V 16
#define ID_REG_V 92
#define FLASH_STATUS_WR_REG_V 12
#define FOUR_BYTE_ENABLE_REG_V 80
#define SECTOR_PROTECT_REG_V 88
#define SUBSECTOR_ERASE_REG_V 20
#define WRITE_ENABLE_REG_V 4
#define SECTOR_CMD_STATUS_REG 0x0
#define SECTOR_RD_CMD 1
#define SECTOR_WR_CMD 2
#define SECTOR_SIZE_BYTES 256


void flash_init(void);
void flash_read_sector(uint8 *buf, int sectorno);
void flash_write_sector(uint8 *buf, int sectorno);
void test_flash(void);

#endif