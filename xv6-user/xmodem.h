#ifndef XMODEM_H
#define XMODEM_H

struct xmodem {
	void *args; /* arguments pointer pass to operation function */
	int (*get_chars)(void *args, void *dst, int n); /* return one char */
	int (*put_char)(void *args, char c);
	int (*char_avail)(void *args); /* return none zero when available */
	void (*delay_1s)(void); /* delay 1s */
	void (*delay_10s)(void); /* delay 10s */
	int (*write)(void *args, void *buf, int len); /* return numbers of bytes written */
	int (*read)(void *args, void *buf, int len); /* return numbers of bytes read */
};

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
int xmodem_recv(struct xmodem *rx);
int xmodem_send(struct xmodem *rx);

#endif
