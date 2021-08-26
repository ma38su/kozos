PREFIX  = /usr/local
ARCH    = h8300-elf
BINDIR  = $(PREFIX)/bin
ADDNAME = $(ARCH)-

AR      = $(BINDIR)/$(ADDNAME)ar
AS      = $(BINDIR)/$(ADDNAME)as
CC      = $(BINDIR)/$(ADDNAME)gcc
LD      = $(BINDIR)/$(ADDNAME)ld
NM      = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB  = $(BINDIR)/$(ADDNAME)ranlib
STRIP   = $(BINDIR)/$(ADDNAME)strip

H8WRITE_SERDEV = /dev/ttyUSB0

OBJS  = startup.o main.o interrupt.o \
        lib.o serial.o kozos.o syscall.o \
        test08_1.o

TARGET = kozos

CFLAGS = -Wall -mh -nostdinc -nostdlib -fno-builtin \
         -I. -Os -DKOZOS
#CFLAGS += -g

LFLAGS = -static -T ld.scr -L.

.SUFFIXES: .c .o
.SUFFIXES: .s .o

all :		$(TARGET)

$(TARGET) :	$(OBJS)
		$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LFLAGS)
		cp $(TARGET) $(TARGET).elf
		$(STRIP) $(TARGET)

.c.o : $<
		$(CC) -c $(CFLAGS) $<

.s.o : $<
		$(CC) -c $(CFLAGS) $<

clean :
		rm -f $(OBJS) $(TARGET) $(TARGET).elf

run:
	sudo chmod o+rw $(H8WRITE_SERDEV)
	cu -l $(H8WRITE_SERDEV) -s 9600

load:
	sudo chmod o+rw $(H8WRITE_SERDEV)
	../../tools/kz_xmodem/kz_xmodem $(TARGET) $(H8WRITE_SERDEV)

