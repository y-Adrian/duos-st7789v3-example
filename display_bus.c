#include "display_bus.h"

#ifdef DISPLAY_BUS_SPIDEV
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wiringx.h>

#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  3
#define SPI_DC_PIN   5
#define SPI_CS_PIN   24
#define BL_PIN       15

#define SPI_RST_0    digitalWrite(SPI_RST_PIN, LOW)
#define SPI_RST_1    digitalWrite(SPI_RST_PIN, HIGH)
#define SPI_DC_0     digitalWrite(SPI_DC_PIN, LOW)
#define SPI_DC_1     digitalWrite(SPI_DC_PIN, HIGH)
#define SPI_CS_0     digitalWrite(SPI_CS_PIN, LOW)
#define SPI_CS_1     digitalWrite(SPI_CS_PIN, HIGH)
#define BL_0         digitalWrite(BL_PIN, LOW)
#define BL_1         digitalWrite(BL_PIN, HIGH)

#ifndef SPI_DEV
#define SPI_DEV "/dev/spidev0.0"
#endif

#ifndef SPI_SPEED_HZ
#define SPI_SPEED_HZ 12000000
#endif

#ifdef DISPLAY_BUS_SPIDEV
static int spi_fd = -1;
#endif

static void bus_delay_us(unsigned int us)
{
    volatile unsigned int i;

    while (us--) {
        for (i = 0; i < 30; i++);
    }
}

#ifndef DISPLAY_BUS_SPIDEV
static void software_spi_send_byte(uint8_t value)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {
        digitalWrite(SPI_SDO_PIN, (value & 0x80) ? HIGH : LOW);

        bus_delay_us(3);
        digitalWrite(SPI_SCK_PIN, HIGH);
        bus_delay_us(3);
        digitalWrite(SPI_SCK_PIN, LOW);
        bus_delay_us(2);

        value <<= 1;
    }
}
#endif

#ifdef DISPLAY_BUS_SPIDEV
static int spidev_write_bytes(const uint8_t *data, unsigned int len)
{
    ssize_t ret;

    if (spi_fd < 0) {
        printf("spidev is not open\n");
        return -1;
    }

    while (len > 0) {
        ret = write(spi_fd, data, len);
        if (ret < 0) {
            printf("write %s failed: %s\n", SPI_DEV, strerror(errno));
            return -1;
        }
        if (ret == 0) {
            printf("write %s returned 0\n", SPI_DEV);
            return -1;
        }

        data += ret;
        len -= (unsigned int)ret;
    }

    return 0;
}

static int spidev_init(void)
{
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = SPI_SPEED_HZ;

    spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        printf("open %s failed: %s\n", SPI_DEV, strerror(errno));
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        printf("set SPI mode failed: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        printf("set SPI bits failed: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        printf("set SPI speed failed: %s\n", strerror(errno));
        return -1;
    }

    printf("spidev bus: %s, mode=0x%x, bits=%u, speed=%u Hz\n",
           SPI_DEV, mode, bits, speed);

    return 0;
}
#endif

int display_bus_init(void)
{
    int pins[] = {
        SPI_RST_PIN,
        SPI_DC_PIN,
        SPI_CS_PIN,
        BL_PIN,
#ifndef DISPLAY_BUS_SPIDEV
        SPI_SCK_PIN,
        SPI_SDO_PIN,
#endif
    };
    unsigned int i;

    if (wiringXSetup("milkv_duos", NULL) == -1) {
        wiringXGC();
        return -1;
    }

    for (i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        if (wiringXValidGPIO(pins[i]) != 0) {
            printf("Invalid GPIO %d\n", pins[i]);
        } else {
            printf("GPIO %d OK\n", pins[i]);
            pinMode(pins[i], PINMODE_OUTPUT);
        }
    }

    SPI_CS_1;
#ifndef DISPLAY_BUS_SPIDEV
    digitalWrite(SPI_SCK_PIN, LOW);
    digitalWrite(SPI_SDO_PIN, LOW);
#endif

#ifdef DISPLAY_BUS_SPIDEV
    if (spidev_init() != 0) {
        return -1;
    }
#endif

    return 0;
}

void display_bus_backlight_on(void)
{
    BL_1;
}

void display_bus_backlight_off(void)
{
    BL_0;
}

void display_bus_reset_assert(void)
{
    SPI_RST_0;
}

void display_bus_reset_release(void)
{
    SPI_RST_1;
}

void display_bus_write_cmd(uint8_t cmd)
{
    SPI_DC_0;
    SPI_CS_0;
#ifdef DISPLAY_BUS_SPIDEV
    spidev_write_bytes(&cmd, 1);
#else
    software_spi_send_byte(cmd);
#endif
    SPI_CS_1;
}

void display_bus_write_data(uint8_t data)
{
    SPI_DC_1;
    SPI_CS_0;
#ifdef DISPLAY_BUS_SPIDEV
    spidev_write_bytes(&data, 1);
#else
    software_spi_send_byte(data);
#endif
    SPI_CS_1;
}

void display_bus_write_data_buf(const uint8_t *data, unsigned int len)
{
#ifndef DISPLAY_BUS_SPIDEV
    unsigned int i;
#endif

    SPI_DC_1;
    SPI_CS_0;
#ifdef DISPLAY_BUS_SPIDEV
    spidev_write_bytes(data, len);
#else
    for (i = 0; i < len; i++) {
        software_spi_send_byte(data[i]);
    }
#endif
    SPI_CS_1;
}
