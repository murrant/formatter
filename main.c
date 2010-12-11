/*
        BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
        Requires mini.

Copyright (C) 2008, 2009        Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009              Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2008, 2009        Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009        Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009              John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"
#include "string.h"
#include "ipc.h"
#include "mini_ipc.h"
#include "nandfs.h"
#include "otp.h"
#include "fat.h"
#include "malloc.h"
#include "diskio.h"
#include "printf.h"
#include "video_low.h"
#include "input.h"
#include "console.h"
#include "sha1.h"
#include "es.h"
#include "wad.h"
#include "aes.h"
#include "nandfs.h"

#include "string.h"

#define SETTINGS_NORESTORE 0
#define SETTINGS_RESTORESERIAL 1
#define SETTINGS_RESTOREFILE 2

#define MINIMUM_MINI_VERSION 0x00010001
#define TITLEID1(titleId)           ((u32)((titleId) >> 32))
#define TITLEID2(titleId)           ((u32)(titleId))

seeprom_t seeprom;

#ifdef MSPACES
mspace mem2space;
#endif

static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

static char ascii(char s) {
	if(s < 0x20) return '.';
	if(s > 0x7E) return '.';
	return s;
}

void hexdump(void *d, int len) {
	u8 *data;
	int i, off;
	data = (u8*)d;
	for (off=0; off<len; off += 16) {
		printf("%08x  ",off);
		for(i=0; i<16; i++)
		if((i+off)>=len) printf("   ");
		else printf("%02x ",data[off+i]);

		printf(" ");
		for(i=0; i<16; i++)
		if((i+off)>=len) printf(" ");
		else printf("%c",ascii(data[off+i]));
		printf("\n");
	}
}
	
void testOTP(void)
{
	printf("reading OTP...\n");
	printf("OTP:\n");
	otp_init();
	hexdump(&otp, sizeof(otp_t));

	printf("reading SEEPROM...\n");
	getseeprom(&seeprom);
	printf("read SEEPROM!\n");
	printf("SEEPROM:\n");
	hexdump(&seeprom, sizeof(seeprom));
}

s32 testAES(void)
{
	printf("testAES: ");
	static u8 somedata[128] ALIGNED(64);
	static u8 data2[128] ALIGNED(64);
	static u8 data3[128] ALIGNED(64);
	u8 key[16], iv[16];
	/*printf("somedata:\n");
	hexdump(somedata, 128);
	printf("key:\n");
	hexdump(key, 16);
	printf("iv:\n");
	hexdump(iv, 16);
	printf("\n");*/

	memset(data2, 0, 128);
	aes_reset();
	aes_set_key(key);
	aes_set_iv(iv);
	aes_encrypt(somedata, data2, 128/16, 0);

/*	hexdump(data2, 128);

	printf("...\n");
	printf("iv:\n");
	hexdump(iv, 16);
	printf("--\n");*/
	
	aes_reset();
	aes_set_key(key);
	aes_set_iv(iv);
	aes_decrypt(data2, data3, 128/16, 0);
    s32 ret = memcmp(somedata, data3, 128) ? -1 : 0;
    printf("%d\n", ret); 
	if(ret) return ret;

	memset(data3, 0, 128);
	my_aes_set_key(key);
	my_aes_decrypt(iv, data2, data3, 128);

    ret = memcmp(somedata, data3, 128) ? -1 : 0;
    printf("aes test 2: %d\n", ret); 
    
	return ret;
}

u32 lolcrypt(u8 *stuff)
{
	u32 key = 0x73b5dbfa;
	while(*stuff) {
		*stuff ^= (key & 0xff);
		stuff++;
		key = ((key<<1) | (key>>31));
	}
	return key;
}

int main(void)
{
	int vmode = -1;
#ifdef MSPACES
	mem2space = create_mspace_with_base((void *)0x90000000, 64*1024*1024, 0);
#endif
	exception_init();
	dsp_reset();

	// clear interrupt mask
	write32(0x0c003004, 0);

	ipc_initialize();
	ipc_slowping();

	gecko_init();
        input_init();
	init_fb(vmode);

	VIDEO_Init(vmode);
	VIDEO_SetFrameBuffer(get_xfb());
	VISetupEncoder();

	u32 version = ipc_getvers();
	u16 mini_version_major = version >> 16 & 0xFFFF;
	u16 mini_version_minor = version & 0xFFFF;
	printf("Mini version: %d.%0d\n", mini_version_major, mini_version_minor);

	if (version < MINIMUM_MINI_VERSION) {
		printf("Sorry, this version of MINI (armboot.bin)\n"
			"is too old, please update to at least %d.%0d.\n", 
			(MINIMUM_MINI_VERSION >> 16), (MINIMUM_MINI_VERSION & 0xFFFF));
		for (;;) 
			; // better ideas welcome!
	}

//    print_str_noscroll(112, 112, "ohai, world!\n");
	gfx_printf("Formatter!");
	gfx_printf("I'm about to delete EVERYTHING and");
	gfx_printf("install the wads in the wad folder.");
	gfx_printf("If you do not want to proceed, hold");
	gfx_printf("the POWER button on your Wii now.");

	testOTP();

	hexdump(otp.nand_hmac, 20);
	hexdump(otp.nand_key, 16);


	if(testAES()) return 0;

	printf("test: %x\n", my_atoi_hex("00002a4e", 8));

	struct nandfs_fp fp;
	nandfs_initialize();

#if 0
	nandfs_open(&fp, "/shared1/content.map");
#ifdef MSPACES
	u8 *stuff = (u8 *) mspace_memalign(mem2space, 32, fp.size);
#else
	u8 *stuff = (u8 *) memalign(32, fp.size);
#endif
	nandfs_read(stuff, fp.size, 1, &fp);
	hexdump(stuff, fp.size);
	return 0;
#endif

	FATFS fs;
	f_mount(0, NULL);
	disk_initialize(0);
	f_mount(0, &fs);
	FIL fatf;
	DIR fatd;
	FILINFO fati;


	char settingTxt[0x300] __attribute__((aligned(32)));
	memset(settingTxt, 0, 0x300);
	s32 actulen;
	u32 actulentwo;
	int restoreSetting;

	nandfs_open(&fp, "/title/00000001/00000002/data/setting.txt");
	actulen = nandfs_read((u8 *)settingTxt, 256, 1, &fp);
	f_open(&fatf, "/setting.txt", FA_WRITE);
	f_write(&fatf, settingTxt, 256, &actulentwo);
	f_close(&fatf);
	//nandfs_close(&fp);
	gfx_printf("Saved your setting.txt to SD:");
	gfx_printf("If you want to use it after backup");
	gfx_printf("press A/RESET now.");
	gfx_printf("Otherwise, press START/EJECT.");
	u32 padinput = input_wait();
	if (padinput & PAD_A) { // This catches GPIO_RESET too
/*		gfx_printf("Use the serial number: press A/RESET");
		gfx_printf("Use the whole file, press START/EJECT");
		padinput = input_wait();
		if (padinput & PAD_A) { // and use the serial number
			restoreSetting = SETTINGS_RESTORESERIAL;
			gfx_printf("Restoring serial only.");
		} else {*/
			restoreSetting = SETTINGS_RESTOREFILE;
			gfx_printf("Restoring entire file.");
//		}
	} else {
		restoreSetting = SETTINGS_NORESTORE;
		gfx_printf("Generating setting.txt.");
	}
	gfx_printf("LAST CHANCE: Turn off Wii to abort,");
	gfx_printf("or press any button to continue.");
	input_wait();
	int i;
	for (i = 0; i <= CONSOLE_LINES; ++i)
		scroll();
	gfx_printf("DUMMIE'D");
//goto lol;
	es_format();
	
	nandfs_walk();

	int region = 0;
	static char pathname[1024];
	printf("diropen: %d\n", f_opendir(&fatd, "wad"));
	printregion("USA");
	while(1) {
		printf("readdir: %d\n", f_readdir(&fatd, &fati));
		if(fati.fname[0] == 0) break;
		sprintf(pathname, "/wad/%s", fati.fname);
		FRESULT ret = f_open(&fatf, pathname, FA_READ);
		printf("open %s: %d\n", pathname, ret);
		if(ret) continue;
//		gfx_printf("Installing %s\n", pathname);
//		gfx_printf("fsize: %d\n", fatf.fsize);
		printwhichwad(pathname, fatf.fsize);

		if(wad_install(&fatf)) break;
		u64 ttid = get_titleid(&fatf);
//		de_printf("TITLEID1: %u TITLEID2: %u              ", TITLEID1(ttid), TITLEID2(ttid));
		if(TITLEID1(ttid) == 1 && TITLEID2(ttid) == 2) {
			switch (get_revision(&fatf)) {
				case 97: case 193: case 225: case 257: case 289: case 353: case 385: case 417: case 449: case 481: case 513:
					region = 0; // USA
					break;
				case 130: case 162: case 194: case 226: case 258: case 290: case 354: case 386: case 418: case 450: case 482: case 514:
					region = 1; // EUR
					break;
				case 128: case 192: case 224: case 256: case 288: case 352: case 384: case 416: case 448: case 480: case 512: 
					region = 2; // JPN
					break;
			}
		}
		switch (region) {
			case 0:
				printregion("USA");
				break;
			case 1:
				printregion("EUR");
				break;
			case 2:
				printregion("JPN");
				break;
		}
	}

	printf("create: %d\n", nandfs_create("/title/00000001/00000002/data/setting.txt", 0, 0, NANDFS_ATTR_FILE, 3, 3, 3));
	s32 ret = nandfs_open(&fp, "/title/00000001/00000002/data/setting.txt");
	printf("open 2: %d\n", ret);

	char video[5] = "NTSC";
	char gameRegion[3] = "US";
	char area[4] = "USA";
	switch (region) {
		case 1:
			strcpy(gameRegion, "EU");
			strcpy(video, "PAL");
			strcpy(area, "EUR");
			break;
		case 2:
			strcpy(gameRegion, "JP");
			strcpy(video, "PAL");
			strcpy(area, "JPN");
			break;
	}
	if (restoreSetting == SETTINGS_NORESTORE) {
		u32 serno = 104170609;
		sprintf(settingTxt, "AREA=%s\r\nMODEL=RVL-001(%s)\r\nDVD=0\r\nMPCH=0x7FFE\r\nCODE=LU\r\nSERNO=%d\r\nVIDEO=%s\r\nGAME=%s\r\n", area, area, serno, video, gameRegion);

		lolcrypt((u8 *)settingTxt);
	} else if (restoreSetting == SETTINGS_RESTORESERIAL) { // restore serial.
		// thanks tona
		char fulltext[9] = "123456789";
		char *line = (char *)settingTxt;
		char *delim, *end;
		int slen;
		int nlen = strlen("SERNO");
		
		lolcrypt((u8 *)settingTxt);
		while(line < (settingTxt+0x100) ) {
			delim = strchr(line, '=');
			if(delim && ((delim - line) == nlen) && !memcmp("SERNO", line, nlen)) {
				delim++;
				end = strchr(line, '\r');
					if(end) {
					slen = end - delim;
					if(slen < 9) {
						memcpy(fulltext, delim, slen);
						fulltext[slen] = 0;
						return slen;
					} else {
					}
				}
			}
			
			// skip to line end
			while(line < (settingTxt+0x100) && *line++ != '\n');
		}
	} else { // restore whole file. - already loaded into settingTxt
	}

	nandfs_write((u8 *)settingTxt, sizeof(settingTxt), 1, &fp);

	printf("iplsave: %d\n", nandfs_create("/title/00000001/00000002/data/iplsave.bin", 0, 0, NANDFS_ATTR_FILE, 3, 3, 3));

	nandfs_writemeta();

	printf("Final listing:\n");
	nandfs_walk();

	boot2_run(1, 2);
//		gfx_printf("Done\n");
//lol:
	printdone();

	return 0;
}

