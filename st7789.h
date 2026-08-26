#ifndef __ST7789_H
#define __ST7789_H

#include <stdint.h>

#define ST7789_WIDTH   170
#define ST7789_HEIGHT  320

#define ST7789_RED     0xF800
#define ST7789_GREEN   0x07E0
#define ST7789_BLUE    0x001F
#define ST7789_WHITE   0xFFFF
#define ST7789_BLACK   0x0000
#define ST7789_YELLOW  0xFFE0
#define ST7789_CYAN    0x07FF
#define ST7789_MAGENTA 0xF81F

void st7789_delay_us(unsigned int us);
void st7789_delay_ms(unsigned int ms);
void st7789_write_cmd(uint8_t cmd);
void st7789_write_data(uint8_t data);
void st7789_write_data_buf(const uint8_t *data, unsigned int len);
void st7789_set_window(uint16_t x_start, uint16_t y_start,
                       uint16_t x_end, uint16_t y_end);
void st7789_fill_rgb565(uint16_t color);
void st7789_clear(void);
void st7789_draw_rgb565_image(uint16_t x, uint16_t y,
                              uint16_t width, uint16_t height,
                              const uint8_t *data);
void st7789_draw_vertical_gradient(void);
void st7789_draw_checkerboard(uint16_t tile_size);
void st7789_init(void);
void st7789_display_char16_16(unsigned int x, unsigned int y,
                              unsigned long color,
                              unsigned char word_serial_number);
void st7789_picture_display(const unsigned char *ptr_pic);

/* Legacy names kept for compatibility with the original sample. */
void delay_us(unsigned int _us_time);
void delay_ms(unsigned int _ms_time);
void TFT_SEND_CMD(unsigned char o_command);
void TFT_SEND_DATA(unsigned char o_data);
void TFT_SEND_DATA_BUF(const unsigned char *data, unsigned int len);
void TFT_SET_ADD(unsigned short int x_start,unsigned short int y_start,unsigned short int x_end,unsigned short int y_end);
void TFT_clear(void);
void TFT_full(unsigned int color);
void TFT_init(void);
void display_char16_16(unsigned int x,unsigned int y,unsigned long color,unsigned char word_serial_number);
void Picture_display(const unsigned char *ptr_pic);

#endif
