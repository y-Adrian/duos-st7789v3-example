#include "st7789.h"
#include "data.h"
#include "display_bus.h"
#include "stdint.h"
#include <unistd.h>
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

void TFT_SEND_CMD(unsigned char cmd)
{
    display_bus_write_cmd(cmd);
}

void TFT_SEND_DATA(unsigned char data)
{
    display_bus_write_data(data);
}

void TFT_SEND_DATA_BUF(const unsigned char *data, unsigned int len)
{
    display_bus_write_data_buf(data, len);
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
    uint8_t line[170 * 2];
    unsigned int i;
    unsigned int row;

    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);

    for (i = 0; i < sizeof(line); i += 2) {
        line[i] = color >> 8;
        line[i + 1] = color;
    }

    for (row = 0; row < TFT_LINE_NUMBER; row++) {
        TFT_SEND_DATA_BUF(line, sizeof(line));
    }
}

void TFT_clear(void)
{
    TFT_full(0xFFFF);
}

// ST7789V3 初始化
void TFT_init(void)
{
    display_bus_reset_assert();
    delay_ms(100);
    display_bus_reset_release();
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

int main(void)
{
    point = &picture_tab[0];

    if (display_bus_init() != 0) {
        printf("display bus init failed\n");
        return -1;
    }

    display_bus_backlight_on();
    printf("Backlight ON\n");

    TFT_init();
    fflush(stdout);

    // 反复刷小区域纯色，方便观察是否稳定
    while (1) {
        printf("RED\n");   fflush(stdout); TFT_full(RED);   delay_ms(2000);
        printf("GREEN\n"); fflush(stdout); TFT_full(GREEN); delay_ms(2000);
        printf("BLUE\n");  fflush(stdout); TFT_full(BLUE);  delay_ms(2000);
    }

    return 0;
}
