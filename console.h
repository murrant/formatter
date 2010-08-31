/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009		bLAStY <blasty@bootmii.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#define CONSOLE_WIDTH 500
#define CONSOLE_LINES 10
#define CONSOLE_Y 100

#define CONSOLE_X  58
#define CONSOLE_CHAR_HEIGHT 16
#define CONSOLE_ROW_HEIGHT (CONSOLE_CHAR_HEIGHT + 1) 

void init_fb(int vmode);
void print_str(const char *str, size_t len);
void print_str_noscroll(int x, int y, char *str);
int console_printf(const char *fmt, ...);
u32 *get_xfb(void);
int gfx_printf(const char *fmt, ...);
extern unsigned char console_font_8x16[];

void scroll();

void printticket();
void printtmd();
void printregion(const char *reg);
void printdone();
int printwhichwad(const char *wadfn, int size);
int printinstall(int contentno, int totalcontent, u64 size);

#endif

