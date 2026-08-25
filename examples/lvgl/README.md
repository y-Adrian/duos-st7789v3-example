# LVGL ST7789V3 Flush Port

This directory contains a small LVGL flush adapter for the verified Duo S ST7789V3 display driver.

It is intentionally not compiled by the top-level `Makefile`. Copy or reference these files from your LVGL project when integrating the display.

## Files

| File | Description |
| --- | --- |
| `lvgl_st7789_port.c` | LVGL flush callback implementation |
| `lvgl_st7789_port.h` | Public init and flush callback declarations |

The adapter uses the reusable driver API from the repository root:

```c
display_bus_init();
display_bus_backlight_on();
st7789_init();
st7789_set_window();
st7789_write_data_buf();
```

## Color Format

The adapter assumes LVGL renders in RGB565:

```text
Resolution: 170x320
Color:      RGB565 / 16-bit
```

For LVGL 8, configure:

```c
#define LV_COLOR_DEPTH 16
```

For LVGL 9, configure the display color format as RGB565 when creating the display.

The adapter writes ST7789 pixel data as high byte first:

```text
RGB565 high byte, then RGB565 low byte
```

## LVGL 8 Usage

Example wiring:

```c
#include "lvgl.h"
#include "lvgl_st7789_port.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[170 * 40];
static lv_color_t buf2[170 * 40];
static lv_disp_drv_t disp_drv;

void display_init(void)
{
    lvgl_st7789_port_init();

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 170 * 40);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 170;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = lvgl_st7789_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
```

## LVGL 9 Usage

Example wiring:

```c
#include "lvgl.h"
#include "lvgl_st7789_port.h"

static uint8_t buf1[170 * 40 * 2];
static uint8_t buf2[170 * 40 * 2];

void display_init(void)
{
    lv_display_t *display;

    lvgl_st7789_port_init();

    display = lv_display_create(170, 320);
    lv_display_set_flush_cb(display, lvgl_st7789_flush_cb);
    lv_display_set_buffers(display,
                           buf1,
                           buf2,
                           sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
}
```

## Notes

- Keep the top-level RGB refresh test as the hardware baseline.
- Verify `DISPLAY_BUS=spidev` works before enabling LVGL.
- A partial draw buffer such as `170x40` is a good starting point for this small screen.
- If colors look swapped, check LVGL RGB565 byte-swap/color-format settings before changing the ST7789 init sequence.
