#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

void draw_glyph(char c, uint32_t cell_x, uint32_t cell_y, uint8_t color);

#endif