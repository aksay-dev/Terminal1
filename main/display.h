#pragma once

#include "lvgl.h"

/* Инициализация RGB дисплея ST7701S и привязка к LVGL. */
void display_init(void);

/* Установить яркость подсветки 0..100 (%) */
void display_set_brightness(uint8_t percent);


