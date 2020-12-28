TARGET=main

# Uncomment needed toolchain configuration line
#TOOLCHAIN_PREFIX=~/Toolchains/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux/gcc-arm-none-eabi-9-2019-q4-major/bin/arm-none-eabi
TOOLCHAIN_PREFIX=avr

CC=$(TOOLCHAIN_PREFIX)-gcc
LD=$(TOOLCHAIN_PREFIX)-gcc
CP=$(TOOLCHAIN_PREFIX)-objcopy
SZ=$(TOOLCHAIN_PREFIX)-size

# upload port
PORT = /dev/ttyACM*
# upload baud rate
BAUD = 19200
# MCU clock
DEFS = -DF_CPU=9600000
# Cortex core
MCU = attiny13
MCFLAGS = -mmcu=$(MCU)


################## User Sources ####################
SRCS = main.c

################## Includes ########################
INCLS = -I.

################ Compiler Flags ######################
CFLAGS = -Wall -Wextra -Warray-bounds
CFLAGS += $(MCFLAGS)

############# CFLAGS for Optimization ##################
CFLAGS += -Os
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
#CFLAGS += -v #verbose

################ Objcopy Flags ######################
CPFLAGS = -j .text -j .data

################### Recipe to make all (build and burn) ####################
.PHONY: all
all: build

################### Recipe to build ####################
.PHONY: build
build: $(TARGET).hex


################### Recipe to burn ####################
.PHONY: burn
burn:
	@echo "[Flashing] $(TARGET).hex to port $(PORT) at baud $(BAUD)"
	@avrdude -c stk500v1 -v -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$(TARGET).hex -U lfuse:w:0x7a:m -U hfuse:w:0xff:m
		
################### Recipe to make .elf ####################
$(TARGET).elf: $(SRCS)
	@echo "[Compiling] $(SRCS)"
	@$(CC) $(INCLS) $(DEFS) $(CFLAGS) $^ -o $@
	


################### Recipe to make .hex ####################
$(TARGET).hex: $(TARGET).elf
	@echo "[Building] $@"
	@$(CP) $(CPFLAGS) -O ihex $^ $@
	@$(SZ) -C --mcu=$(MCU) $(TARGET).elf

################### Recipe to clean all ####################
.PHONY: clean
clean:
	@echo "[Cleaning] $(TARGET).hex $(TARGET).elf"
	@rm -f $(TARGET).hex $(TARGET).elf
