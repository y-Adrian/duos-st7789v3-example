TARGET=st7789
DISPLAY_BUS ?= software
DISPLAY_ORIENTATION ?= landscape
SPI_DEV ?= /dev/spidev0.0
SPI_SPEED_HZ ?= 12000000

ifeq (,$(TOOLCHAIN_PREFIX))
$(error TOOLCHAIN_PREFIX is not set)
endif

ifeq (,$(CFLAGS))
$(error CFLAGS is not set)
endif

ifeq (,$(LDFLAGS))
$(error LDFLAGS is not set)
endif

CC = $(TOOLCHAIN_PREFIX)gcc
SYSROOT = /home/adrian/pocket/duo-sdk/host-tools/gcc/riscv64-linux-musl-aarch64/sysroot
CFLAGS += -I$(SYSROOT)/usr/include
CFLAGS += -DSPI_DEV=\"$(SPI_DEV)\"
CFLAGS += -DSPI_SPEED_HZ=$(SPI_SPEED_HZ)

ifeq ($(DISPLAY_ORIENTATION),landscape)
CFLAGS += -DST7789_DISPLAY_LANDSCAPE=1
else ifeq ($(DISPLAY_ORIENTATION),portrait)
CFLAGS += -DST7789_DISPLAY_LANDSCAPE=0
else
$(error DISPLAY_ORIENTATION must be landscape or portrait)
endif

ifeq ($(DISPLAY_BUS),spidev)
CFLAGS += -DDISPLAY_BUS_SPIDEV
else ifneq ($(DISPLAY_BUS),software)
$(error DISPLAY_BUS must be software or spidev)
endif

LDFLAGS += -L$(SYSROOT)/lib
LDFLAGS += -L$(SYSROOT)/usr/lib
LDFLAGS += -lwiringx

SOURCE = main.c st7789.c display_bus.c data.c picture_data.c
OBJS = $(patsubst %.c,%.o,$(SOURCE))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	@rm *.o -rf
	@rm $(OBJS) -rf
	@rm $(TARGET)
