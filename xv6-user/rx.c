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

#define MAX_ERR  5
#define MAX_RETRY 5

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


//#define DEBUG_ERR
//#define DEBUG

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

#if defined(DEBUG) || defined(DEBUGERR)
int logf;
#endif


/**
 * return crc16 value
 */
static uint16_t crc16(const uint8_t *buf, int sz) {
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
 * check crc16
 * @return: return 0 on match, -1 on not match
 */
static int check_pkt_crc(xmodem_pkt *pkt, uint8_t id) {
	uint16_t crc_pkt;
	if (id != pkt->id || pkt->id + pkt->uid != 0xFF)
		return -1;

	crc_pkt = ((uint16_t) pkt->crc_hi << 8) + pkt->crc_lo;
	if (crc16(pkt->data, sizeof(pkt->data)) == crc_pkt)
		return 0;

	return -1;
}

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
static int recv_start(struct xmodem *rx) {
	uint8_t errcnt = 0;
	uint8_t pktnum = 1;
	xmodem_pkt pkt;
	int timeouts = 0;
	pkt.head = 0;


	while (1) {
		/* receive one packet */
		while(pkt.head != XMODEM_HEAD){
			// wait for header
			rx->get_chars(rx->args, &pkt.head, 1);

			#ifdef DEBUG
			fprintf(logf, "got %d for head\n", pkt.head);
			#endif

			switch (pkt.head) {
				case XMODEM_HEAD: {
					break;
				}
				case XMODEM_EOT: {
					rx->put_char(rx->args, XMODEM_ACK);
					/*finished ok*/
					return 0;
				}
				default: {
					errcnt++;
					if (errcnt > MAX_ERR) {
						/* to many error, cancel it */
						rx->put_char(rx->args, XMODEM_CAN);
						exit(0);
					}

					/* try again */
					sleep(5);
					empty_serial_buf();
					rx->put_char(rx->args, XMODEM_NAK);
					break;
				}
			} /* end switch */
		}/* while receive one packet */
		// now receive the paket
		int got;
		if((got = rx->get_chars(rx->args, &pkt.id, XMODEM_PKT_SIZE - 1)) != XMODEM_PKT_SIZE - 1){
			if(timeouts > MAX_RETRY){
				/* to many errors, cancel it */
				rx->put_char(rx->args, XMODEM_CAN);
				printf("timeout-exit\n");
				exit(0);
			} else {
				sleep(5);
				empty_serial_buf();

				#ifdef DEBUG_ERR
				fprintf(logf, "only got %d for main:\n", got);
				for(uint8_t * i =  &pkt.head ; i < &pkt.crc_lo; i++){
					fprintf(logf, "%d ", *i);
				}
				#endif

				rx->put_char(rx->args, XMODEM_NAK); /* error packet */
				sleep(5);
				pkt.head = 0;
				timeouts++;
				continue;
			}
		} else {
			timeouts = 0;
		}

		/* check crc */
		if (check_pkt_crc(&pkt, pktnum) == 0) {
			/* handle one packet */
			pktnum++;
			if (rx->write(rx->args, pkt.data, sizeof(pkt.data))
					!= sizeof(pkt.data)) {
				/* write error, stop */
				rx->put_char(rx->args, XMODEM_CAN);
				return -1;
			}
			rx->put_char(rx->args, XMODEM_ACK);
		} else {
			#ifdef DEBUG_ERR
			fprintf(logf, "bad paket: \n");
			for(uint8_t * i =  &pkt.head ; i < &pkt.crc_lo; i++){
				fprintf(logf, "%d ", *i);
			}
			fprintf(logf, "\n");
			#endif
			rx->put_char(rx->args, XMODEM_NAK); /* error packet */
		}

		pkt.head = 0;
	}/* while 1 */
	return -1;
}

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
int xmodem_recv(struct xmodem *rx) {
	uint8_t retry_count = 0;
	while (retry_count < MAX_RETRY) {
		rx->put_char(rx->args, XMODEM_CRC_CHR);
		return recv_start(rx);
	}
	printf("Transmission start timeout\n");
	return -1;
}



void
empty_serial_buf()
{
	char trash;
	while(read(aux_uart_fd, &trash, 1) != 0)
	{}
}


int put_char(void *args, char c) {
	write(aux_uart_fd, &c, 1);
	#ifdef DEBUG
	fprintf(logf, "tx char: %d\n", c);
	#endif
	return 0;
}


int get_chars(void *args, void* dst, int n) {
	int total = 0;
	int this_num;
	int time_out_ticks = 0;
	#ifdef DEBUG
	fprintf(logf, "get_chars(%p, %d)\n",  dst, n);
	#endif

	while(total != n){
		this_num = read(aux_uart_fd, (void * ) ((int) dst + total), n - total);

		#ifdef DEBUG
		for(int i = 0; i < this_num; i++){
			fprintf(logf, "rx char: %d\n",  *((char *) (dst + i)));
		}
		#endif
		total += this_num;
		if(total > n){
			return total;
		}
		if(this_num == 0){
			if(time_out_ticks < 2){
				time_out_ticks++;
				sleep(3);
			} else {
				return total;
			}
				
		} else {
			time_out_ticks = 0;
		}

	}

	#ifdef DEBUG
	fprintf(logf, "done\n",  dst, n);
	#endif
	return total;
}


void delay_1s() {
	sleep(100);
}

void delay_10s() {
	sleep(1000);
}


int writer(void *args, void *buf, int size) {
	struct xmodem_args *p = (struct xmodem_args *)args;
	return write(p->filefd, buf, size);
}

int main(int argc, char *argv[]) {
	struct xmodem_args args;
	struct xmodem recver;
	char *filename = argv[1];
	if (argc != 2) {
		printf("receive file in xmodem protocol\n");
		printf("Build: %s %s\n", __DATE__, __TIME__);
		printf("usage: %s <filename>\n", argv[0]);
		exit(-1);
	}
	printf("Xmodem Build: %s %s\n", __DATE__, __TIME__);
	
	#if defined(DEBUG) || defined(DEBUG_ERR)
	logf = open("serial_log.txt", O_CREATE | O_WRONLY);
	#endif

	#ifdef DEBUG_ERR
	fprintf(logf, "Log of errors in rx\n");
	#endif

	
	/* open file to write recv file */
	args.filefd = open(filename, O_CREATE | O_WRONLY);

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
	delay_1s();
	delay_1s();
	delay_1s();

	recver.args = &args;
	recver.get_chars = get_chars;
	recver.put_char = put_char;
	recver.delay_1s = delay_1s;
	recver.delay_10s = delay_10s;
	recver.write = writer;
	xmodem_recv(&recver);

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
