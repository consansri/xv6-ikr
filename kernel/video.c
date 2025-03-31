#include "include/printf.h"
#include "include/types.h"
#include "include/riscv.h"
#include "include/buf.h"
#include "include/spinlock.h"
#include "include/intr.h"
#include "include/disk.h"

#include "include/flash.h"
#include "include/memlayout.h"
#include "include/video.h"

#include <stdint.h>

#define WIDTH_CHARS 80
#define HEIGHT_CHARS 33

#define TAB_WIDTH 3

#define CHAR_X_DIST 6
#define CHAR_Y_DIST 8
char screen_chars[WIDTH_CHARS*HEIGHT_CHARS];

int redraw_required = 0;
int scroll_required = 0;

void
i2c_write(uint8_t addr, uint8_t data)
{	
	#ifdef DEBUG
	printf("i2c_write(%d, %d)\n", addr, data);
	#endif
	// wait for i2c to get ready
	uint8_t status;
	uint32_t waited = 0;
	do{
		status = readb(I2C_ADAPT + I2C_STATUS_REG_V);
		if(++waited > 100000)
			panic("i2c write timeout");
	} while((status & 0xC0) != 0);

	// set target addr
	uint16_t payload_and_addr = 0x7200 | addr;
	writew(payload_and_addr, I2C_ADAPT + I2C_DATA_REG_V);

	waited = 0;
	do{
		status = readb(I2C_ADAPT + I2C_STATUS_REG_V);
		if(++waited > 100000)
			panic("i2c write timeout");
	} while((status & 0x80) != 0);

	// set target data
	payload_and_addr = 0x7200 | data;
	writew(payload_and_addr, I2C_ADAPT + I2C_DATA_REG_V);
}


uint8_t
i2c_read(uint8_t addr)
{	
	#ifdef DEBUG
	printf("i2c_read(%d)\n", addr);
	#endif
	// wait for i2c to get ready
	uint32_t waited = 0;
	uint8_t status;
	do{
		status = readb(I2C_ADAPT + I2C_STATUS_REG_V);
		if(++waited > 100000)
			panic("i2c read timeout");
	} while((status & 0xC0)!= 0);

	// set target addr
	uint16_t payload_and_addr = 0x7200 | addr;
	writew(payload_and_addr, I2C_ADAPT + I2C_DATA_REG_V);


	// wait for i2c to get ready
	waited = 0;
	do{
		status = readb(I2C_ADAPT + I2C_STATUS_REG_V);
		if(++waited > 100000)
			panic("i2c read timeout");
	} while((status & 0xC0) != 0);

	// start read transaction
	payload_and_addr = 0x7300;
	writew(payload_and_addr, I2C_ADAPT + I2C_DATA_REG_V);

	status = readb(I2C_ADAPT + I2C_STATUS_REG_V);



	// wait for i2c to get ready
	waited = 0;
	do{
		status = readb(I2C_ADAPT + I2C_STATUS_REG_V);
		if(++waited > 100000)
			panic("i2c read timeout");
		if((status & 0x20) != 0)
			panic("i2c write");
	} while(status & 0xC0 != 0);

	return readb(I2C_ADAPT + I2C_DATA_REG_V);

}


void reset_sii(void){

	#ifdef DEBUG
	printf("reset_sii()\n");
	#endif


	writeb(0, SII_CTRL);


	waitMs(20);
	for(int i = 0; i < 1000000; i++){}

	writeb(1, SII_CTRL);

	for(int i = 0; i < 1000000; i++){}

	waitMs(200);

	i2c_write(0xC7, 0);

	uint8_t status;
	uint32_t waited = 0;
	do{
		status = i2c_read(0x1B);
		if(++waited > 100000)
			panic("i2c read timeout");
	} while(status != 0xB4);

	i2c_write(0x1A, 0x11);
	i2c_write(0x09, 0x00);
	i2c_write(0x1E, 0x00);
	i2c_write(0x09, 0x00);
	i2c_write(0x0A, 0x00);
	i2c_write(0xBC, 0x01);
	i2c_write(0xBD, 0x80);
	i2c_write(0xBE, 0x24);
	i2c_write(0x19, 0x01);
	i2c_write(0x3C, 0x00);
	i2c_write(0x3D, 0xF3);
	i2c_write(0x1A, 0x01);
	i2c_write(0x3C, 0xFB);
	i2c_read(0x3D);
}



int cursor_x = 0;
int cursor_y = 0;
int cursor_tick = 1;

int fg = BLACK;
int bg = WHITE;


void
set_pixel(int x, int y, int col)
{	
	uint8_t fbyte = col + (col * 16);

    writeb(fbyte, FRAME_BFR + (x) + y * (WIDTH));
    writeb(fbyte, FRAME_BFR + (x) + y * (WIDTH) + WIDTH/2);
}


void
temu_putc_at(int c, int tx, int ty, int col)
{
    for (int dy = 0;  dy < FONT_CHAR_HEIGHT; ++dy) {
        for (int dx = 0;  dx < FONT_CHAR_WIDTH; ++dx) {
            int px = tx + dx;
            int py = ty + dy;
            if (0 <= px && px < WIDTH && 0 <= py && py < HEIGHT) {
                if(FONT_BIT(c, dy, dx)){
                  set_pixel(px, py, col);
                }
            }
        }
    }
}

void
clear_screen()
{
  memset(FRAME_BFR, (bg + (bg * 16)), FRAME_BFR_SIZE);

  return;
}

void
clear_chars()
{
	for(int i = 0; i < WIDTH_CHARS * HEIGHT_CHARS; i++){
		screen_chars[i] = ' ';
	}
}

void
draw_chars()
{	
	for(int j = 0; j < HEIGHT_CHARS; j++){
		for(int i = 0; i < WIDTH_CHARS; i++){
			if(screen_chars[i + j * WIDTH_CHARS] != '\n'){
				temu_putc_at(screen_chars[i + j * WIDTH_CHARS], i * CHAR_X_DIST, j * CHAR_Y_DIST, fg);
			} else {
				break;
			}
		}
	
	}
}


void
scroll_chars()
{	
	for(int j = 1; j < HEIGHT_CHARS; j++){
		for(int i = 0; i < WIDTH_CHARS; i++){
			screen_chars[i + j * WIDTH_CHARS - WIDTH_CHARS] = screen_chars[i + j * WIDTH_CHARS];
				
		}
	
	}

	for(int i = 0; i < WIDTH_CHARS; i++){
		screen_chars[i + (HEIGHT_CHARS - 1) * WIDTH_CHARS] = ' ';	
	}
}


void 
scroll_if_required()
{
	if(cursor_y > HEIGHT_CHARS - 1){
		cursor_x = 0;
		cursor_y = HEIGHT_CHARS - 1;
		scroll_chars();
		scroll_required = 1;
		redraw_required = 1;
	}
}


void
temu_tick()
{
	static cursor_tick = 0;
	static last_cursor_x = 0;
	static last_cursor_y = 0;

  	if(cursor_tick == 0){
  	  temu_putc_at(0xFF, cursor_x * CHAR_X_DIST, cursor_y * CHAR_Y_DIST, fg);
	  last_cursor_x = cursor_x;
	  last_cursor_y = cursor_y;
  	  cursor_tick = 1;
  	} else {
		char c = screen_chars[last_cursor_x + last_cursor_y * WIDTH_CHARS];
		temu_putc_at(0xFF, last_cursor_x * CHAR_X_DIST, last_cursor_y * CHAR_Y_DIST, bg);
		if(c != ' ' && c != '\n'){
			temu_putc_at(c, last_cursor_x * CHAR_X_DIST, last_cursor_y * CHAR_Y_DIST, fg);
		}

  	  cursor_tick = 0;
  	}
	

  	if(redraw_required)
  	{
		clear_screen();
		draw_chars();
		scroll_required = 0;
		redraw_required = 0;
  	}
}


void
video_temu_putc(int c)
{
	switch(c){
		case '\n':
			for(int x = cursor_x; x < WIDTH_CHARS; x++)
				screen_chars[x + cursor_y * WIDTH_CHARS] = ' ';

			screen_chars[cursor_x + cursor_y * WIDTH_CHARS] = '\n';
			cursor_x = 0;
			cursor_y++;
			scroll_if_required();
			
			break;
		case '\t':
			cursor_x = cursor_x + TAB_WIDTH;
			if((cursor_x + 1) > WIDTH_CHARS){
				cursor_x = 0;
				cursor_y++;
				scroll_if_required();
			};
		case 0x100: // backspace
		case 8:
			cursor_x--;
			if(cursor_x < 0)
				cursor_x = 0;
			// speed up deleting
			temu_putc_at(0xFF, cursor_x * CHAR_X_DIST, cursor_y * CHAR_Y_DIST, bg);
			screen_chars[cursor_x + cursor_y * WIDTH_CHARS] = ' ';
			break;
		default:
			// speed up output of regular characters
			if(scroll_required == 0)
				temu_putc_at(c, cursor_x * CHAR_X_DIST, cursor_y * CHAR_Y_DIST, fg);
			screen_chars[cursor_x + cursor_y * WIDTH_CHARS] = c;
			cursor_x++;
			if((cursor_x + 1) > WIDTH_CHARS){
				cursor_x = 0;
				cursor_y++;
				scroll_if_required();
			};
			break;
	}

	return;
}


void video_init(void) {
	
	#ifdef DEBUG
	printf("video_init\n");
	#endif

	reset_sii();

	clear_chars();

	clear_screen();
}
