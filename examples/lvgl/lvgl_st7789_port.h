#ifndef LVGL_ST7789_PORT_H
#define LVGL_ST7789_PORT_H

#include "lvgl.h"

int lvgl_st7789_port_init(void);

#if LVGL_VERSION_MAJOR >= 9
void lvgl_st7789_flush_cb(lv_display_t *display,
                          const lv_area_t *area,
                          uint8_t *px_map);
#else
void lvgl_st7789_flush_cb(lv_disp_drv_t *drv,
                          const lv_area_t *area,
                          lv_color_t *color_p);
#endif

#endif
