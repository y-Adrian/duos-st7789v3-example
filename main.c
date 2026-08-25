#include "display_bus.h"
#include "st7789.h"

#include <stdio.h>

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
        printf("RED\n");
        fflush(stdout);
        st7789_fill_rgb565(ST7789_RED);
        st7789_delay_ms(2000);

        printf("GREEN\n");
        fflush(stdout);
        st7789_fill_rgb565(ST7789_GREEN);
        st7789_delay_ms(2000);

        printf("BLUE\n");
        fflush(stdout);
        st7789_fill_rgb565(ST7789_BLUE);
        st7789_delay_ms(2000);
    }

    return 0;
}
