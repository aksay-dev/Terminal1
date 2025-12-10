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

/* GT911 Configuration - пины из Arduino-примера */
#define TOUCH_GT911_SDA     19
#define TOUCH_GT911_SCL     45
#define TOUCH_GT911_INT     -1  /* Не используем IRQ */
#define TOUCH_GT911_RST     -1  /* Не используем аппаратный сброс */

/* Координатное пространство (разрешение панели) */
#define TOUCH_MAX_X         480
#define TOUCH_MAX_Y         480

/* Повороты, совместимые с Arduino Touch_GT911 */
#define TOUCH_ROT_LEFT      0
#define TOUCH_ROT_INVERTED  1
#define TOUCH_ROT_RIGHT     2
#define TOUCH_ROT_NORMAL    3

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

/**
 * @brief Установить ориентацию экрана (как в Arduino Touch_GT911)
 */
void touch_set_rotation(uint8_t rot);

/**
 * @brief Обновить разрешение панели в контроллере (запись в конфиг GT911)
 */
bool touch_set_resolution(uint16_t width, uint16_t height);

/**
 * @brief Прочитать до 5 точек
 * @param xs массив из 5 значений X
 * @param ys массив из 5 значений Y
 * @param count запишется количество точек
 * @return true если есть валидные касания
 */
bool touch_read_points(int16_t *xs, int16_t *ys, uint8_t *count);

