include broadway.mk

CFLAGS += -Werror
DEFINES = -DLACKS_SYS_TYPES_H -DLACKS_ERRNO_H -DLACKS_STDLIB_H -DLACKS_STRING_H -DLACKS_STRINGS_H -DLACKS_UNISTD_H
DEFINES += -DMSPACES
LDSCRIPT = mini.ld
LIBS = -lgcc

TARGET = ppcboot.elf

OBJS = realmode.o crt0.o main.o string.o sync.o time.o printf.o input.o \
	exception.o exception_2200.o malloc.o gecko.o video_low.o \
	ipc.o mini_ipc.o nandfs.o ff.o diskio.o fat.o font.o console.o \
	fs_hmac.o sha1.o otp.o es.o wad.o rijndael.o aes.o

include common.mk

upload: $(TARGET)
	@$(WIIDEV)/bin/bootmii -p $<

copy: $(TARGET)
	sudo cp ppcboot.elf /media/usbhd-sdb1/bootmii/
	sync; sync; sync
	sudo umount /media/usbhd-sdb1

.PHONY: upload

