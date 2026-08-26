#include "display_bus.h"
#include "st7789.h"
#include "data.h"

#include <stdio.h>

static void show_color_bars(void)
{
    unsigned int x;
    unsigned int y;

    printf("COLOR BARS\n");
    fflush(stdout);

    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    for (y = 0; y < ST7789_HEIGHT; y++) {
        uint16_t color;

        if (y < ST7789_HEIGHT / 3) {
            color = ST7789_RED;
        } else if (y < (ST7789_HEIGHT * 2) / 3) {
            color = ST7789_GREEN;
        } else {
            color = ST7789_BLUE;
        }

        for (x = 0; x < ST7789_WIDTH; x++) {
            st7789_write_data(color >> 8);
            st7789_write_data(color);
        }
    }
}

static void show_centered_picture(void)
{
    uint16_t x = (ST7789_WIDTH - PICTURE_TAB_WIDTH) / 2;
    uint16_t y = (ST7789_HEIGHT - PICTURE_TAB_HEIGHT) / 2;

    printf("RGB565 PICTURE\n");
    fflush(stdout);

    st7789_fill_rgb565(ST7789_BLACK);
    st7789_draw_rgb565_image(x, y, PICTURE_TAB_WIDTH, PICTURE_TAB_HEIGHT,
                             picture_tab);
}

int main(void)
{
    if (display_bus_init() != 0) {
        printf("display bus init failed\n");
        return -1;
    }

    display_bus_backlight_on();
    printf("Backlight ON\n");

    st7789_init();
    fflush(stdout);

    while (1) {
        show_color_bars();
        st7789_delay_ms(2000);

        printf("GRADIENT\n");
        fflush(stdout);
        st7789_draw_vertical_gradient();
        st7789_delay_ms(2000);

        printf("CHECKERBOARD\n");
        fflush(stdout);
        st7789_draw_checkerboard(16);
        st7789_delay_ms(2000);

        show_centered_picture();
        st7789_delay_ms(2000);
    }

    return 0;
}
