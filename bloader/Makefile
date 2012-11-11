LIBNAME	= ../library/lpcusb/usbstack
APPNAME = bloader
TARGET = bloader

IMGEXT		= hex
FLASH_TOOL 	= LPC21
RUN_MODE	= ROM_RUN
SUBMDL   	= LPC2138

# ---------------------------------------------------------------------------
# Flash-Programming support using lpc21isp by Martin Maurer 
# only for Philips LPC and Analog ADuC ARMs
#
# Settings and variables:
MSG_LPC21_RESETREMINDER = You may have to bring the target in bootloader-mode now.
LPC21ISP = lpc21isp
LPC21ISP_PORT = /dev/ttyUSB0
LPC21ISP_BAUD = 38400
LPC21ISP_XTAL = 12000
LPC21ISP_FLASHFILE = $(TARGET).hex
LPC21ISP_CONTROL = -control
# ---------------------------------------------------------------------------

# Tool definitions
CC      = arm-elf-gcc
LD      = arm-elf-ld -v
AR      = arm-elf-ar
AS      = arm-elf-as
CP      = arm-elf-objcopy
OD		= arm-elf-objdump
RM		= rm

# Tool flags
CFLAGS  = -I./ -I../ -I../library/lpcusb -c -W -Wall -Os -g -mcpu=arm7tdmi
ASFLAGS = -ahls -mapcs-32 -o crt0.o
LFLAGS  =  -nostartfiles --warn-common  -mcpu=arm7tdmi
CPFLAGS = -O ihex
ODFLAGS	= -x --syms

LINKFILE	= lpc2148-rom.ld

CSRCS	= startup.c printf.c console.c sys.c 
OBJS 	= crt0.o $(CSRCS:.c=.o)


all: depend $(TARGET)

bloader: $(OBJS) main_msc.o msc_bot.o msc_scsi.o blockdev.o iap.o logo.o $(LIBNAME).a

$(TARGET):
	@ echo "Building $@ bloader..."
	$(CC) -T $(LINKFILE) $(LFLAGS) $^ -o $@.elf -Wl,-Map,$@.map
	$(CP) $(CPFLAGS) $@.elf $@.hex
	$(OD) $(ODFLAGS) $@.elf > $@.dmp

crt0.o: crt0.S
	@ echo ".assembling"
	$(CC) -c $(AFLAGS) -Wa,-ahlms=crt0.lst crt0.S -o crt0.o

clean:
	rm -f *.hex *.elf *.o *.lst *.dmp *.map .depend
	
# Program the device.  - lpc21isp will not work for SAM7
program: $(TARGET)
	@echo
	@echo $(MSG_LPC21_RESETREMINDER)
	$(LPC21ISP) $(LPC21ISP_CONTROL) $(LPC21ISP_DEBUG) $(LPC21ISP_FLASHFILE) $(LPC21ISP_PORT) $(LPC21ISP_BAUD) $(LPC21ISP_XTAL)

# recompile if the Makefile changes
$(OBJS): Makefile

# dependency checking
depend: $(CSRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend || rm -f .depend

# phony targets
.PHONY: clean

-include .depend

