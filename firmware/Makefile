###############################################################################
# Makefile for ISTAtrol heating valve controller
###############################################################################

## General Flags
PROJECT = firmware

MCU = attiny2313

F_CPU = 12800000

BUILDDIR = build

AVRDUDE = avrdude
AVRDUDEFLAGS = -c stk500v2 -p $(MCU) -P /dev/ttyACM0
AVRDUDEFLAGSFAST = $(AVRDUDEFLAGS) -B 1
AVRDUDEFLAGSSLOW = $(AVRDUDEFLAGS) -B 5

TARGET = $(PROJECT).hex
CC = avr-gcc

## Options common to compile, link and assembly rules.
COMMON = -mmcu=$(MCU) -save-temps=obj

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -DF_CPU=$(F_CPU)
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
CFLAGS += -Winline
CFLAGS += -std=gnu99
CFLAGS += -Os
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -fno-move-loop-invariants
CFLAGS += -fno-tree-scev-cprop
CFLAGS += -ffunction-sections
CFLAGS += -finline-functions-called-once

## Assembly specific flags.
ASMFLAGS = $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags.
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,--as-needed
LDFLAGS += -Wl,--gc-sections

## Intel Hex file production flags.
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Include Directories.
INCLUDES = -I. -I"usbdrv" -I"libs-device"

## Objects that must be built in order to link.
OBJECTS = main.o usbdrv.o usbdrvasm.o osccal.o
BUILDOBJECTS = $(addprefix $(BUILDDIR)/,$(OBJECTS))

## Objects explicitly added by the user.
LINKONLYOBJECTS =


## Build
all: $(BUILDDIR) $(TARGET) $(BUILDDIR)/$(PROJECT).elf \
     $(BUILDDIR)/$(PROJECT).eep $(BUILDDIR)/$(PROJECT).lss size

## Program
program: $(PROJECT).hex
	$(AVRDUDE) $(AVRDUDEFLAGSFAST) -U flash:w:$^

## Compile
.SUFFIXES:
.SUFFIXES: .c .S .o
$(shell mkdir -p $(BUILDDIR))

$(BUILDDIR)/main.o: main.c usbdrv/usbdrv.h
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $@

$(BUILDDIR)/usbdrvasm.o: usbdrv/usbdrvasm.S usbdrv/usbdrv.h usbconfig.h
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $< -o $@

$(BUILDDIR)/usbdrv.o: usbdrv/usbdrv.c usbdrv/usbdrv.h usbconfig.h
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $@

$(BUILDDIR)/osccal.o: libs-device/osccal.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $@

## Link
.SUFFIXES: .elf .eep .lss .hex
$(BUILDDIR)/%.elf: $(BUILDOBJECTS)
	$(CC) $(LDFLAGS) $(BUILDOBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $@

%.hex: $(BUILDDIR)/%.elf
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS) $< $@

$(BUILDDIR)/%.eep: $(BUILDDIR)/%.elf
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

$(BUILDDIR)/%.lss: $(BUILDDIR)/%.elf
	avr-objdump -h -S $< > $@

size: $(BUILDDIR)/$(PROJECT).elf
	@echo
	avr-size -C --mcu=$(MCU) $(BUILDDIR)/$(PROJECT).elf

## Fuses
fuses:
	@echo "To Be Defined" $(AVRDUDE) $(AVRDUDEFLAGSFAST)

## Clean target.
.PHONY: clean
clean:
	-rm -rf $(BUILDDIR) $(TARGET)
