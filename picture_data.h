#ifndef PICTURE_DATA_H_
#define PICTURE_DATA_H_

#include <stdint.h>

#define PICTURE_TAB_WIDTH  320
#define PICTURE_TAB_HEIGHT 170
#define PICTURE_TAB_SIZE   (PICTURE_TAB_WIDTH * PICTURE_TAB_HEIGHT * 2)

extern const uint8_t picture_tab[PICTURE_TAB_SIZE];

#endif
