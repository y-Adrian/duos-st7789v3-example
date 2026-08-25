#include "display_bus.h"

#include <stdio.h>
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

static void bus_delay_us(unsigned int us)
{
    volatile unsigned int i;

    while (us--) {
        for (i = 0; i < 30; i++);
    }
}

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

int display_bus_init(void)
{
    int pins[] = {
        SPI_RST_PIN,
        SPI_DC_PIN,
        SPI_CS_PIN,
        BL_PIN,
        SPI_SCK_PIN,
        SPI_SDO_PIN,
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
    digitalWrite(SPI_SCK_PIN, LOW);
    digitalWrite(SPI_SDO_PIN, LOW);

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
    software_spi_send_byte(cmd);
    SPI_CS_1;
}

void display_bus_write_data(uint8_t data)
{
    SPI_DC_1;
    SPI_CS_0;
    software_spi_send_byte(data);
    SPI_CS_1;
}

void display_bus_write_data_buf(const uint8_t *data, unsigned int len)
{
    unsigned int i;

    SPI_DC_1;
    SPI_CS_0;
    for (i = 0; i < len; i++) {
        software_spi_send_byte(data[i]);
    }
    SPI_CS_1;
}
