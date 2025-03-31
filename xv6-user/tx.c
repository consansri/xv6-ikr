// from https://github.com/heavyii/rx/blob/master/rx.c
#include <sys/select.h>
#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/sysinfo.h"
#include "xv6-user/user.h"
#include "xmodem.h"


struct xmodem_args {
	int fd;
	int filefd;
};

#include <stdint.h>
#include <ctype.h>

#define MAX_ERR  100
#define MAX_RETRY 20

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_CRC_CHR 'C'

#define XMODEM_DATA_SIZE_SOH 128  /* for Xmodem protocol */
#define XMODEM_DATA_SIZE_STX 1024 /* for 1K xmodem protocol */
#define USE_1K_XMODEM 0  /* 1 for use 1k_xmodem 0 for xmodem */

#if (USE_1K_XMODEM)
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_STX
#define XMODEM_HEAD  XMODEM_STX
#else
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_SOH
#define XMODEM_HEAD   XMODEM_SOH
#endif

#define XMODEM_PKT_SIZE (XMODEM_DATA_SIZE + 5)

/*
 * Xmodem Frame form: <SOH><blk #><255-blk #><--128 data bytes--><CRC hi><CRC lo>
 */
typedef struct _pkt {
	uint8_t head;
	uint8_t id;
	uint8_t uid;
	uint8_t data[XMODEM_DATA_SIZE];
	uint8_t crc_hi;
	uint8_t crc_lo;
} xmodem_pkt;

int aux_uart_fd;
int logf;


/**
 * return crc16 value
 */
uint16_t crc16(const uint8_t *buf, int sz) {
	uint16_t crc = 0;
	while (--sz >= 0) {
		int i;
		crc = crc ^ ((uint16_t) *buf++) << 8;
		for (i = 0; i < 8; i++) {
			if (crc & 0x8000) {
				crc = crc << 1 ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}


/**
 * start to send a file
 * @return: 0 on success, -1 on error
 */
int xmodem_send(struct xmodem *rx) {
	uint8_t errcnt = 0;
	uint8_t pktnum = 1;
	xmodem_pkt pkt;
	uint8_t *p = NULL;
	uint8_t * const endp = (uint8_t *) &pkt + XMODEM_PKT_SIZE;

	uint8_t retry_count = 0;
	pkt.head = XMODEM_HEAD;
	// wait for receiver to get ready
	rx->delay_1s();

	// wait for C character from receiver
	char rx;
	while(rx->read(rx->args, (uint8_t *) &pkt.data[0], 1) == 0){
		if(rx == 'C')
			break;
		else
			errcnt++;
			if(errcnt > MAX_ERR)
				return -1;

	}



	while (1) {
		for(int i = 0; i < 128; i++)
			pkt.data[i] = 0;

		int bytes = rx->read(rx->args, (uint8_t *) &pkt.data[0], 128);

		if (bytes == 0){
			rx->put_char(rx->args, XMODEM_EOT);
			return 0;
		} else if (bytes == -1){
			fprintf(2, "could not read from file!");
		}

		pkt.id = pktnum;
		pkt.uid = ~pktnum;
		uint16_t crc = crc16((uint8_t *) &pkt.data[0], 128);

		pkt.crc_lo =  crc & 0xFF;
		pkt.crc_hi = (crc >> 8) & 0xFF;
		
		
		uint8_t retcode;
		for(int try = 0; try < MAX_RETRY; try++){
			rx->put_char(rx->args, pkt.head);
			rx->put_char(rx->args, pkt.id);
			rx->put_char(rx->args, pkt.uid);
			for(int di = 0; di < sizeof(pkt.data); di++)
				rx->put_char(rx->args, pkt.data[di]);
			
			rx->put_char(rx->args, pkt.crc_hi);
			rx->put_char(rx->args, pkt.crc_lo);
			rx->get_chars(rx->args, retcode, 1);
			if(retcode == XMODEM_ACK){
				break;
			}
		}

		if(retcode != XMODEM_ACK){
			return -1;
		}

		pktnum = (pktnum + 1) & 0xFF;
	}
	return 0;
}



char inbuf[2];
int inbufchars = 0;

int time_out_ticks = 0;

void
empty_serial_buf()
{
	char trash;
	inbufchars = 0;
	while(read(aux_uart_fd, &trash, 1) != 0)
	{}
}


int put_char(void *args, char c) {
	write(aux_uart_fd, &c, 1);
	//fprintf(logf, "tx char: %d\n", c);
	return 0;
}

int char_avail(void *args) {
	char dst;
	if(read(aux_uart_fd, &dst, 1) == 0){
		return 0;
	} else {
		inbuf[inbufchars] = dst;
		inbufchars++;
		//fprintf(logf, "rx char: %d\n", dst);
		return 1;
	}
}


int get_char(void *args) {
	while(inbufchars != 1){
		char_avail(args);

		if(inbufchars != 1){
			if(time_out_ticks < 200){
				time_out_ticks++;
				sleep(1);
			} else {
				return -1;
			}
				
		} else {
			time_out_ticks = 0;
		}
	}
	inbufchars = 0;

	return (int) inbuf[0];
}


void delay_1s() {
	sleep(100);
}

void delay_10s() {
	sleep(1000);
}


int reader(void *args, void *buf, int size) {
	struct xmodem_args *p = (struct xmodem_args *)args;
	return read(p->filefd, buf, size);
}

int main(int argc, char *argv[]) {
	struct xmodem_args args;
	struct xmodem sender;
	char *filename = argv[1];
	if (argc != 2) {
		printf("transmit file in xmodem protocol\n");
		printf("Build: %s %s\n", __DATE__, __TIME__);
		printf("usage: %s <filename>\n", argv[0]);
		exit(-1);
	}
	printf("Xmodem Build: %s %s\n", __DATE__, __TIME__);
	

	//logf = open("serial_log.txt", O_CREATE | O_WRONLY);
	/* open file to write recv file */
	args.filefd = open(filename, O_RDONLY);

	if (args.filefd < 0) {
		printf("error: open");
		goto main_quit;
	}

	// open auxiliary uart
	aux_uart_fd = dev(O_RDWR, AUX_UART_DEV_ID, 0);

	if (aux_uart_fd < 0) {
		printf("error: open direct uart access");
		goto main_quit;
	}
	empty_serial_buf();
	printf("filefd = %d, uart_dev = %d\n", args.filefd, aux_uart_fd);
	
	delay_1s();

	sender.args = &args;
	sender.get_char = get_char;
	sender.put_char = put_char;
	sender.char_avail = char_avail;
	sender.delay_1s = delay_1s;
	sender.delay_10s = delay_10s;
	sender.read = reader;
	xmodem_send(&sender);

	main_quit:
	if (args.filefd >= 0) {
		close(args.filefd);
		args.filefd = -1;
	}
	if (args.fd >= 0) {
		close(args.fd);
		args.fd = -1;
	}
	exit(0);
}
