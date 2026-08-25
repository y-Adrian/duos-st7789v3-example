#ifndef DISPLAY_BUS_H
#define DISPLAY_BUS_H

#include <stdint.h>

int display_bus_init(void);
void display_bus_backlight_on(void);
void display_bus_backlight_off(void);
void display_bus_reset_assert(void);
void display_bus_reset_release(void);
void display_bus_write_cmd(uint8_t cmd);
void display_bus_write_data(uint8_t data);
void display_bus_write_data_buf(const uint8_t *data, unsigned int len);

#endif
