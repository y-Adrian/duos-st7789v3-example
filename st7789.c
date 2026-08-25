#include "st7789.h"
#include "data.h"
#include "stdint.h"
#include <unistd.h>
#include <wiringx.h>
#include "stdio.h"

#define TFT_COLUMN_NUMBER 170
#define TFT_LINE_NUMBER   320
#define TFT_COLUMN_OFFSET 35
#define TFT_LINE_OFFSET   0

#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF

#define PIC_LEN 120
#define PIC_HIG 120

#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  3 // 26
#define SPI_DC_PIN   5 // 22
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

const unsigned char *point;
void delay_us(unsigned int us)
{
  volatile unsigned int i;
  while (us--) {
      for (i = 0; i < 30; i++);   // 稍慢一点更稳
  }
}

void delay_ms(unsigned int ms)
{
    while (ms--) {
        delay_us(1000);
    }
}

void SPI_SendByte(unsigned char Value)
{
    unsigned char i;
    for (i = 0; i < 8; i++) {
        if (Value & 0x80)
            digitalWrite(SPI_SDO_PIN, HIGH);
        else
            digitalWrite(SPI_SDO_PIN, LOW);

        delay_us(3);
        digitalWrite(SPI_SCK_PIN, HIGH);
        delay_us(3);
        digitalWrite(SPI_SCK_PIN, LOW);
        delay_us(2);
        Value <<= 1;
    }
}

void TFT_SEND_CMD(unsigned char cmd)
{
  SPI_DC_0;
  SPI_CS_0;
  SPI_SendByte(cmd);
  SPI_CS_1;
}

void TFT_SEND_DATA(unsigned char data)
{
    SPI_DC_1;
    SPI_CS_0;
    SPI_SendByte(data);
    SPI_CS_1;
}

void TFT_SET_ADD(unsigned short x_start, unsigned short y_start,
                 unsigned short x_end, unsigned short y_end)
{
    unsigned short x1 = x_start + TFT_COLUMN_OFFSET;
    unsigned short x2 = x_end   + TFT_COLUMN_OFFSET;
    unsigned short y1 = y_start + TFT_LINE_OFFSET;
    unsigned short y2 = y_end   + TFT_LINE_OFFSET;

    TFT_SEND_CMD(0x2A);
    TFT_SEND_DATA(x1 >> 8);
    TFT_SEND_DATA(x1);
    TFT_SEND_DATA(x2 >> 8);
    TFT_SEND_DATA(x2);

    TFT_SEND_CMD(0x2B);
    TFT_SEND_DATA(y1 >> 8);
    TFT_SEND_DATA(y1);
    TFT_SEND_DATA(y2 >> 8);
    TFT_SEND_DATA(y2);

    TFT_SEND_CMD(0x2C);
}

// 先只刷小区域，验证是否稳定
void TFT_full(unsigned int color)
{
    unsigned int i, j;

    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);
    for (i = 0; i < TFT_COLUMN_NUMBER; i++) {
        for (j = 0; j < TFT_LINE_NUMBER; j++) {
            TFT_SEND_DATA(color >> 8);
            TFT_SEND_DATA(color);
        }
    }
}

void TFT_clear(void)
{
    TFT_full(0xFFFF);
}

// ST7789V3 初始化
void TFT_init(void)
{
    SPI_RST_0;
    delay_ms(100);
    SPI_RST_1;
    delay_ms(150);

    TFT_SEND_CMD(0x11);         // Sleep Out
    delay_ms(150);

    TFT_SEND_CMD(0x36);         // Memory Access Control
    TFT_SEND_DATA(0x00);        // 方向（当前使用）

    TFT_SEND_CMD(0x3A);
    TFT_SEND_DATA(0x05);        // 16-bit color

    TFT_SEND_CMD(0xB2);
    TFT_SEND_DATA(0x0C);
    TFT_SEND_DATA(0x0C);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x33);
    TFT_SEND_DATA(0x33);

    TFT_SEND_CMD(0xB7);
    TFT_SEND_DATA(0x35);

    TFT_SEND_CMD(0xBB);
    TFT_SEND_DATA(0x1A);

    TFT_SEND_CMD(0xC0);
    TFT_SEND_DATA(0x2C);

    TFT_SEND_CMD(0xC2);
    TFT_SEND_DATA(0x01);

    TFT_SEND_CMD(0xC3);
    TFT_SEND_DATA(0x0F);

    TFT_SEND_CMD(0xC4);
    TFT_SEND_DATA(0x20);

    TFT_SEND_CMD(0xC6);
    TFT_SEND_DATA(0x0F);

    TFT_SEND_CMD(0xD0);
    TFT_SEND_DATA(0xA4);
    TFT_SEND_DATA(0xA1);

    // Positive Gamma
    TFT_SEND_CMD(0xE0);
    TFT_SEND_DATA(0xD0); TFT_SEND_DATA(0x04); TFT_SEND_DATA(0x0D);
    TFT_SEND_DATA(0x11); TFT_SEND_DATA(0x13); TFT_SEND_DATA(0x2B);
    TFT_SEND_DATA(0x3F); TFT_SEND_DATA(0x54); TFT_SEND_DATA(0x4C);
    TFT_SEND_DATA(0x18); TFT_SEND_DATA(0x0D); TFT_SEND_DATA(0x0B);
    TFT_SEND_DATA(0x1F); TFT_SEND_DATA(0x23);

    // Negative Gamma
    TFT_SEND_CMD(0xE1);
    TFT_SEND_DATA(0xD0); TFT_SEND_DATA(0x04); TFT_SEND_DATA(0x0C);
    TFT_SEND_DATA(0x11); TFT_SEND_DATA(0x13); TFT_SEND_DATA(0x2C);
    TFT_SEND_DATA(0x3F); TFT_SEND_DATA(0x44); TFT_SEND_DATA(0x51);
    TFT_SEND_DATA(0x2F); TFT_SEND_DATA(0x1F); TFT_SEND_DATA(0x1F);
    TFT_SEND_DATA(0x20); TFT_SEND_DATA(0x23);

    TFT_SEND_CMD(0x21);         // Inversion On
    TFT_SEND_CMD(0x29);         // Display On
    delay_ms(100);
}

void display_char16_16(unsigned int x, unsigned int y, unsigned long color, unsigned char word_serial_number)
{
    unsigned int column;
    unsigned char tm = 0, temp = 0, xxx = 0;

    TFT_SET_ADD(x, y, x + 15, y + 15);
    for (column = 0; column < 32; column++) {
        temp = chines_word[word_serial_number][xxx];
        for (tm = 0; tm < 8; tm++) {
            if (temp & 0x01) {
                TFT_SEND_DATA(color >> 8);
                TFT_SEND_DATA(color);
            } else {
                TFT_SEND_DATA(0xFF);
                TFT_SEND_DATA(0xFF);
            }
            temp >>= 1;
        }
        xxx++;
    }
}

void Picture_display(const unsigned char *ptr_pic)
{
    unsigned long number;
    TFT_SET_ADD(0, 0, PIC_LEN - 1, PIC_HIG - 1);
    for (number = 0; number < PIC_NUM; number++) {
        TFT_SEND_DATA(*ptr_pic++);
    }
}

static void DEV_GPIO_Init(void)
{
    int pins[] = {SPI_RST_PIN, SPI_DC_PIN, SPI_CS_PIN, BL_PIN, SPI_SCK_PIN, SPI_SDO_PIN};
    for (int i = 0; i < 6; i++) {
        if (wiringXValidGPIO(pins[i]) != 0) {
            printf("Invalid GPIO %d\n", pins[i]);
        } else {
            printf("GPIO %d OK\n", pins[i]);
            pinMode(pins[i], PINMODE_OUTPUT);
        }
    }
    SPI_CS_1;
    digitalWrite(SPI_SCK_PIN, LOW);
}

int wiringx_init(void)
{
    if (wiringXSetup("milkv_duos", NULL) == -1) {
        wiringXGC();
        return -1;
    }
    DEV_GPIO_Init();
    return 0;
}

int main(void)
{
    point = &picture_tab[0];

    if (wiringx_init() != 0) {
        printf("wiringX init failed\n");
        return -1;
    }

    BL_1;
    printf("Backlight ON\n");

    TFT_init();
    printf("TFT_init done (0x36=0x60, small area test)\n");
    fflush(stdout);

    // 反复刷小区域纯色，方便观察是否稳定
    while (1) {
        printf("RED\n");   fflush(stdout); TFT_full(RED);   delay_ms(2000);
        printf("GREEN\n"); fflush(stdout); TFT_full(GREEN); delay_ms(2000);
        printf("BLUE\n");  fflush(stdout); TFT_full(BLUE);  delay_ms(2000);
    }

    return 0;
}