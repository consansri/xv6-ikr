#include "include/printf.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/buf.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/disk.h"

#include "include/flash.h"
#include "include/memlayout.h"


static struct sleeplock flash_lock;

void flash_init(void) {
	
	#ifdef DEBUG
	printf("flash_init\n");
	#endif

	#ifndef SMALLDEBUG
	uint64 id = readd(FLASH_CTRL + ID_REG_V); 
	printf("Flash silicon Id: %d\n", id);
	#endif

	// enable 4 byte mode
    writed(1, FLASH_CTRL + FOUR_BYTE_ENABLE_REG_V); 
	
	// write enable ip core
    writed(1, FLASH_CTRL + WRITE_ENABLE_REG_V); 

	// unprotect all sectors
    writed(0, FLASH_CTRL + SECTOR_PROTECT_REG_V);
}


void wait_flash_ready_for_write_erase()
{
	uint8 sret = 0b10000000;
	while((sret & 0b10000000) == 0){
		sret = (readd(FLASH_CTRL + FLASH_STATUS_REG_V) & 0xFF);
	}
}


void flash_read_sector(uint8 *buf, int sectorno) {

	#ifdef DEBUG
	printf("flash_read_sector(%p, %d)\n", buf, sectorno);
	#endif

	

	// enter critical section!
	acquiresleep(&flash_lock);

	#ifdef UNIFORM_TIMING
	uint64 last_ticks = readq(ACLINT_S);
	#endif

	// start sector read operation
	writed((sectorno + FS_BASE_SECTOR) << 8 | SECTOR_RD_CMD, FLASH_CTRL + SECTOR_CMD_STATUS_REG); 

	// wait for sector read operation to finish
	uint8 ret = 0;
	while(1){
		ret = readb(FLASH_CTRL + SECTOR_CMD_STATUS_REG);

		if ((ret & SECTOR_RD_CMD) == 0){
			break;
		}
	}

	uint64 data = 0;
	// copy the read data into the provided buffer
	for(int i = 0; i < SECTOR_SIZE_BYTES; i = i + 8){
		data = readq(FLASH_BFR + i);
		writeq(data, buf + i);
	}

	#ifdef UNIFORM_TIMING
	waituntilclockspassed(8000, last_ticks);
	#endif

	releasesleep(&flash_lock);
	// leave critical section!
}



void flash_read_sector_no_lock(uint8 *buf, int sectorno) {


	#ifdef UNIFORM_TIMING
	uint64 last_ticks = readq(ACLINT_S);
	#endif

	// start sector read operation
	writed((sectorno + FS_BASE_SECTOR) << 8 | SECTOR_RD_CMD, FLASH_CTRL + SECTOR_CMD_STATUS_REG); 

	// wait for sector read operation to finish
	uint8 ret = 0;
	while(1){
		ret = readb(FLASH_CTRL + SECTOR_CMD_STATUS_REG);

		if ((ret & SECTOR_RD_CMD) == 0){
			break;
		}
	}

	uint64 data = 0;
	// copy the read data into the provided buffer
	for(int i = 0; i < SECTOR_SIZE_BYTES; i = i + 8){
		data = readq(FLASH_BFR + i);
		writeq(data, buf + i);
	}

	#ifdef UNIFORM_TIMING
	waituntilclockspassed(8000, last_ticks);
	#endif

}


void flash_erase_subsector(uint64 addr)
{
	// enter critical section!
	acquiresleep(&flash_lock);

	// erase subsector
	writed(addr + FS_BASE_SECTOR * SECTOR_SIZE_BYTES, FLASH_CTRL + SUBSECTOR_ERASE_REG_V); 

	wait_flash_ready_for_write_erase();

	uint8 sret = (readd(FLASH_CTRL + FLASH_STATUS_REG_V) & 0xFF);

	if((sret & 0b00100010) != 0)
		panic("flash sector erase failed!");

	releasesleep(&flash_lock);
	// leave critical section!
}


void flash_write_sector(uint8 *buf, int sectorno) {


	#ifdef DEBUG
	printf("flash_write_sector(%p, %d)", buf, sectorno);
	#endif


	// enter critical section!
	acquiresleep(&flash_lock);
	


	#ifdef UNIFORM_TIMING
	uint64 last_ticks = readq(ACLINT_S);
	#endif

	uint64 data = 0;
	// copy the read data into the provided buffer
	for(int i = 0; i < SECTOR_SIZE_BYTES; i = i + 8){
		data = readq(buf + i);
		writeq(data, FLASH_BFR + i);
	}


	// start sector write 
	writed((sectorno + FS_BASE_SECTOR) << 8 | SECTOR_WR_CMD, FLASH_CTRL + SECTOR_CMD_STATUS_REG); 

	// wait for sector write operation to finish
	uint8 ret = 0;
	while(1){
		ret = readb(FLASH_CTRL + SECTOR_CMD_STATUS_REG);

		if ((ret & SECTOR_WR_CMD) == 0){
			break;
		}
	}

	#ifdef UNIFORM_TIMING
	waituntilclockspassed(1000000, last_ticks);
	#endif

	wait_flash_ready_for_write_erase();

	uint8 sret;
	sret = (readd(FLASH_CTRL + FLASH_STATUS_REG_V) & 0xFF);
	if((sret & 0b00000010) != 0)
		panic("flash sector write failed due to protection!");
	if((sret & 0b00010000) != 0)
		panic("flash sector write failed!");


	#ifdef DEBUG
	printf("done\n");
	#endif
	releasesleep(&flash_lock);
	// leave critical section!
}

// A simple test for flash read/write test 
void test_flash(void) {
	uint8 buf[BSIZE];

	int sec = 0;
	flash_read_sector(buf, sec);
	printf("content was: \n");
	for (int i = 0; i < SECTOR_SIZE_BYTES; i ++) {
		if (0 == i % 16) {
			printf("\n");
		}

		printf("%x ", buf[i]);
	}
	for (int i = 0; i < SECTOR_SIZE_BYTES; i ++) {
		buf[i] = 0x56;		// data to be written 
	}
	
	flash_erase_subsector(sec * SECTOR_SIZE_BYTES);
	flash_write_sector(buf, sec);

	for (int i = 0; i < SECTOR_SIZE_BYTES; i ++) {
		buf[i] = 0x15;		// fill in junk
	}

	flash_read_sector(buf, sec);
	printf("content is now: \n");
	for (int i = 0; i < SECTOR_SIZE_BYTES; i ++) {
		if (0 == i % 16) {
			printf("\n");
		}

		printf("%x ", buf[i]);
	}
	printf("\n");

	while (1) ;
}
