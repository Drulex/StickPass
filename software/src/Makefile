# Makefile

# Hardware specific configuration
DEVICE     = attiny85
F_CPU      = 16500000
PROGRAMMER = usbtiny
AVRDUDE    = avrdude -c $(PROGRAMMER) -p $(DEVICE)


# Compiler flags
CFLAGS  = -Iusbdrv -I. -DDEBUG_LEVEL=0 -DTUNE_OSCCAL=0 -DCALIBRATE_OSCCAL=0 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections
CFLAGS += -std=gnu99 -Werror -mcall-prologues -fno-tree-scev-cprop -fno-split-wide-types
LDFLAGS = -Wl,-Map=main.map,--relax,--gc-sections

OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o osccalASM.o credentials.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

##### Start of rules definition #####

help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ....... to build main.hex"
	@echo "make program ... to flash fuses and firmware"
	@echo "make flash ..... to flash the firmware (use this on metaboard)"
	@echo "make fuse ...... to flash the fuses only"
	@echo "make clean ..... to delete objects and hex file"

hex: main.hex main.eep

program: flash fuse

fuse: main.hex
	$(AVRDUDE) -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m

# Rule for uploading firmware:
flash: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i

upload: main.hex
	$(AVRDUDE) -U flash:w:main.hex -U eeprom:w:main.eep -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m

# Rule for deleting dependent files
clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep main.elf *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

# Generic rule for compiling C files
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

# Since we don't want to ship the driver multipe times, we copy it into this project:

main.elf: $(OBJECTS)	# usbdrv dependency only needed because we copy it
	$(COMPILE) $(LDFLAGS) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size --format=avr --mcu=$(DEVICE) main.elf

main.eep: main.elf
	rm -f main.eep
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex main.elf main.eep

disasm:	main.elf
	avr-objdump -d main.elf
