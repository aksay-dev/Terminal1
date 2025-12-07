/**
 * @file touch.h
 * @brief GT911 Touchscreen driver for ESP-IDF
 * 
 * Портировано из Arduino проекта 4.0_LvglWidgets
 * Использует ту же логику что и TAMC_GT911 библиотека
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* GT911 Configuration - как в Arduino проекте */
#define TOUCH_GT911_SDA     19
#define TOUCH_GT911_SCL     45
#define TOUCH_GT911_INT     -1  /* Not used */
#define TOUCH_GT911_RST     -1  /* Not used */

/* Coordinate mapping - как в Arduino проекте */
#define TOUCH_MAP_X1        480
#define TOUCH_MAP_X2        0
#define TOUCH_MAP_Y1        480
#define TOUCH_MAP_Y2        0

#define TOUCH_MAX_X         480
#define TOUCH_MAX_Y         480

/* Touch coordinates (updated by driver) */
extern int16_t touch_last_x;
extern int16_t touch_last_y;

/**
 * @brief Initialize GT911 touchscreen
 * @return true on success
 */
bool touch_init(void);

/**
 * @brief Check if touch data is available
 * @return true if touch data ready
 */
bool touch_has_signal(void);

/**
 * @brief Check if screen is currently touched
 * @return true if touched, updates touch_last_x/y
 */
bool touch_touched(void);

/**
 * @brief Check if touch was released
 * @return true if released
 */
bool touch_released(void);

