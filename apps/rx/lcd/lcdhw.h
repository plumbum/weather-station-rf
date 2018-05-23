#ifndef _LCDHW_H_
#define _LCDHW_H_

#include <inttypes.h>

typedef int16_t lcd_coord_t;

typedef uint16_t lcd_color_t;

void lcd_init(lcd_color_t color);

void lcd_clr(lcd_color_t color);
void lcd_box(lcd_coord_t x1, lcd_coord_t y1, lcd_coord_t x2, lcd_coord_t y2, lcd_color_t color);

void lcd_char(char c, lcd_coord_t x, lcd_coord_t y, lcd_color_t fg, lcd_color_t bg);

#endif
