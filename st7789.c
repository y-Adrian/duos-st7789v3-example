#include "st7789.h"
#include "data.h"
#include "display_bus.h"
#include "picture_data.h"

#if ST7789_DISPLAY_LANDSCAPE
#define TFT_COLUMN_OFFSET 0
#define TFT_LINE_OFFSET   35
#define TFT_MADCTL        0x60
#else
#define TFT_COLUMN_OFFSET 35
#define TFT_LINE_OFFSET   0
#define TFT_MADCTL        0x00
#endif

void st7789_delay_us(unsigned int us)
{
  volatile unsigned int i;
  while (us--) {
      for (i = 0; i < 30; i++);   // 稍慢一点更稳
  }
}

void st7789_delay_ms(unsigned int ms)
{
    while (ms--) {
        st7789_delay_us(1000);
    }
}

void st7789_write_cmd(uint8_t cmd)
{
    display_bus_write_cmd(cmd);
}

void st7789_write_data(uint8_t data)
{
    display_bus_write_data(data);
}

void st7789_write_data_buf(const uint8_t *data, unsigned int len)
{
    display_bus_write_data_buf(data, len);
}

void st7789_set_window(uint16_t x_start, uint16_t y_start,
                       uint16_t x_end, uint16_t y_end)
{
    uint16_t x1 = x_start + TFT_COLUMN_OFFSET;
    uint16_t x2 = x_end   + TFT_COLUMN_OFFSET;
    uint16_t y1 = y_start + TFT_LINE_OFFSET;
    uint16_t y2 = y_end   + TFT_LINE_OFFSET;

    st7789_write_cmd(0x2A);
    st7789_write_data(x1 >> 8);
    st7789_write_data(x1);
    st7789_write_data(x2 >> 8);
    st7789_write_data(x2);

    st7789_write_cmd(0x2B);
    st7789_write_data(y1 >> 8);
    st7789_write_data(y1);
    st7789_write_data(y2 >> 8);
    st7789_write_data(y2);

    st7789_write_cmd(0x2C);
}

void st7789_fill_rgb565(uint16_t color)
{
    uint8_t line[ST7789_WIDTH * 2];
    unsigned int i;
    unsigned int row;

    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    for (i = 0; i < sizeof(line); i += 2) {
        line[i] = color >> 8;
        line[i + 1] = color;
    }

    for (row = 0; row < ST7789_HEIGHT; row++) {
        st7789_write_data_buf(line, sizeof(line));
    }
}

void st7789_clear(void)
{
    st7789_fill_rgb565(ST7789_WHITE);
}

void st7789_draw_rgb565_image(uint16_t x, uint16_t y,
                              uint16_t width, uint16_t height,
                              const uint8_t *data)
{
    if (data == 0 || width == 0 || height == 0) {
        return;
    }

    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    if ((uint32_t)x + width > ST7789_WIDTH ||
        (uint32_t)y + height > ST7789_HEIGHT) {
        return;
    }

    st7789_set_window(x, y, x + width - 1, y + height - 1);
    st7789_write_data_buf(data, (unsigned int)width * height * 2);
}

void st7789_draw_vertical_gradient(void)
{
    uint8_t line[ST7789_WIDTH * 2];
    unsigned int x;
    unsigned int y;

    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    for (y = 0; y < ST7789_HEIGHT; y++) {
        uint8_t r = (uint8_t)((y * 31) / (ST7789_HEIGHT - 1));
        uint8_t g = (uint8_t)(((ST7789_HEIGHT - 1 - y) * 63) / (ST7789_HEIGHT - 1));
        uint8_t b = (uint8_t)((y * 31) / (ST7789_HEIGHT - 1));
        uint16_t color = (uint16_t)((r << 11) | (g << 5) | b);

        for (x = 0; x < ST7789_WIDTH; x++) {
            line[x * 2] = color >> 8;
            line[x * 2 + 1] = color;
        }

        st7789_write_data_buf(line, sizeof(line));
    }
}

void st7789_draw_checkerboard(uint16_t tile_size)
{
    uint8_t line[ST7789_WIDTH * 2];
    unsigned int x;
    unsigned int y;

    if (tile_size == 0) {
        tile_size = 16;
    }

    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    for (y = 0; y < ST7789_HEIGHT; y++) {
        for (x = 0; x < ST7789_WIDTH; x++) {
            uint16_t color;

            if (((x / tile_size) + (y / tile_size)) & 1) {
                color = ST7789_CYAN;
            } else {
                color = ST7789_MAGENTA;
            }

            line[x * 2] = color >> 8;
            line[x * 2 + 1] = color;
        }

        st7789_write_data_buf(line, sizeof(line));
    }
}

void st7789_init(void)
{
    display_bus_reset_assert();
    st7789_delay_ms(100);
    display_bus_reset_release();
    st7789_delay_ms(150);

    st7789_write_cmd(0x11);         // Sleep Out
    st7789_delay_ms(150);

    st7789_write_cmd(0x36);         // Memory Access Control
    st7789_write_data(TFT_MADCTL);  // Display orientation

    st7789_write_cmd(0x3A);
    st7789_write_data(0x05);        // 16-bit color

    st7789_write_cmd(0xB2);
    st7789_write_data(0x0C);
    st7789_write_data(0x0C);
    st7789_write_data(0x00);
    st7789_write_data(0x33);
    st7789_write_data(0x33);

    st7789_write_cmd(0xB7);
    st7789_write_data(0x35);

    st7789_write_cmd(0xBB);
    st7789_write_data(0x1A);

    st7789_write_cmd(0xC0);
    st7789_write_data(0x2C);

    st7789_write_cmd(0xC2);
    st7789_write_data(0x01);

    st7789_write_cmd(0xC3);
    st7789_write_data(0x0F);

    st7789_write_cmd(0xC4);
    st7789_write_data(0x20);

    st7789_write_cmd(0xC6);
    st7789_write_data(0x0F);

    st7789_write_cmd(0xD0);
    st7789_write_data(0xA4);
    st7789_write_data(0xA1);

    // Positive Gamma
    st7789_write_cmd(0xE0);
    st7789_write_data(0xD0); st7789_write_data(0x04); st7789_write_data(0x0D);
    st7789_write_data(0x11); st7789_write_data(0x13); st7789_write_data(0x2B);
    st7789_write_data(0x3F); st7789_write_data(0x54); st7789_write_data(0x4C);
    st7789_write_data(0x18); st7789_write_data(0x0D); st7789_write_data(0x0B);
    st7789_write_data(0x1F); st7789_write_data(0x23);

    // Negative Gamma
    st7789_write_cmd(0xE1);
    st7789_write_data(0xD0); st7789_write_data(0x04); st7789_write_data(0x0C);
    st7789_write_data(0x11); st7789_write_data(0x13); st7789_write_data(0x2C);
    st7789_write_data(0x3F); st7789_write_data(0x44); st7789_write_data(0x51);
    st7789_write_data(0x2F); st7789_write_data(0x1F); st7789_write_data(0x1F);
    st7789_write_data(0x20); st7789_write_data(0x23);

    st7789_write_cmd(0x21);         // Inversion On
    st7789_write_cmd(0x29);         // Display On
    st7789_delay_ms(100);
}

void st7789_display_char16_16(unsigned int x, unsigned int y,
                              unsigned long color,
                              unsigned char word_serial_number)
{
    unsigned int column;
    unsigned char tm = 0, temp = 0, xxx = 0;

    st7789_set_window(x, y, x + 15, y + 15);
    for (column = 0; column < 32; column++) {
        temp = chines_word[word_serial_number][xxx];
        for (tm = 0; tm < 8; tm++) {
            if (temp & 0x01) {
                st7789_write_data(color >> 8);
                st7789_write_data(color);
            } else {
                st7789_write_data(0xFF);
                st7789_write_data(0xFF);
            }
            temp >>= 1;
        }
        xxx++;
    }
}

void st7789_picture_display(const unsigned char *ptr_pic)
{
    st7789_draw_rgb565_image(0, 0, PICTURE_TAB_WIDTH, PICTURE_TAB_HEIGHT, ptr_pic);
}

void delay_us(unsigned int us)
{
    st7789_delay_us(us);
}

void delay_ms(unsigned int ms)
{
    st7789_delay_ms(ms);
}

void TFT_SEND_CMD(unsigned char cmd)
{
    st7789_write_cmd(cmd);
}

void TFT_SEND_DATA(unsigned char data)
{
    st7789_write_data(data);
}

void TFT_SEND_DATA_BUF(const unsigned char *data, unsigned int len)
{
    st7789_write_data_buf(data, len);
}

void TFT_SET_ADD(unsigned short x_start, unsigned short y_start,
                 unsigned short x_end, unsigned short y_end)
{
    st7789_set_window(x_start, y_start, x_end, y_end);
}

void TFT_full(unsigned int color)
{
    st7789_fill_rgb565(color);
}

void TFT_clear(void)
{
    st7789_clear();
}

void TFT_init(void)
{
    st7789_init();
}

void display_char16_16(unsigned int x, unsigned int y,
                       unsigned long color,
                       unsigned char word_serial_number)
{
    st7789_display_char16_16(x, y, color, word_serial_number);
}

void Picture_display(const unsigned char *ptr_pic)
{
    st7789_picture_display(ptr_pic);
}
