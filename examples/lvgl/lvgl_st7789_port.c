#include "lvgl_st7789_port.h"

#include "../../display_bus.h"
#include "../../st7789.h"

#include <stdint.h>

int lvgl_st7789_port_init(void)
{
    if (display_bus_init() != 0) {
        return -1;
    }

    display_bus_backlight_on();
    st7789_init();

    return 0;
}

static void write_rgb565_area(const lv_area_t *area, const uint16_t *pixels)
{
    uint8_t line[ST7789_WIDTH * 2];
    int32_t x;
    int32_t y;
    int32_t width = area->x2 - area->x1 + 1;
    int32_t height = area->y2 - area->y1 + 1;

    if (width <= 0 || height <= 0) {
        return;
    }

    st7789_set_window((uint16_t)area->x1,
                      (uint16_t)area->y1,
                      (uint16_t)area->x2,
                      (uint16_t)area->y2);

    for (y = 0; y < height; y++) {
        const uint16_t *src = pixels + (y * width);

        for (x = 0; x < width; x++) {
            uint16_t color = src[x];

            line[(x * 2) + 0] = color >> 8;
            line[(x * 2) + 1] = color & 0xff;
        }

        st7789_write_data_buf(line, (unsigned int)(width * 2));
    }
}

#if LVGL_VERSION_MAJOR >= 9
void lvgl_st7789_flush_cb(lv_display_t *display,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    write_rgb565_area(area, (const uint16_t *)px_map);
    lv_display_flush_ready(display);
}
#else
void lvgl_st7789_flush_cb(lv_disp_drv_t *drv,
                          const lv_area_t *area,
                          lv_color_t *color_p)
{
    write_rgb565_area(area, (const uint16_t *)color_p);
    lv_disp_flush_ready(drv);
}
#endif
