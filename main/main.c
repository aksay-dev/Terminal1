/**
 * @file main.c
 * @brief Простой демо "Hello, World!" для ESP32-S3 с LVGL v9.4
 * 
 * Демонстрирует вывод текста на экран 480x480 через RGB интерфейс
 * Основан на проекте 4.0_LvglWidgets
 */

#include <stdio.h>
#include "font/lv_font.h"
#include "lvgl.h"
#include "display.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "HELLO_WORLD";

void app_main(void) 
{
    ESP_LOGI(TAG, "=== Hello World LVGL v9.4 Demo ===");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());

    /* Инициализация дисплея ST7701 RGB 480x480 */
    display_init();
    ESP_LOGI(TAG, "Display initialized successfully");

    /* ========== Тест цветов: три квадрата ========== */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN); /* чёрный фон */
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Красный квадрат */
    lv_obj_t *rect_red = lv_obj_create(scr);
    lv_obj_set_size(rect_red, 120, 120);
    lv_obj_set_style_bg_color(rect_red, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect_red, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rect_red, 0, LV_PART_MAIN);
    lv_obj_align(rect_red, LV_ALIGN_LEFT_MID, 60, 0);

    /* Зелёный квадрат */
    lv_obj_t *rect_green = lv_obj_create(scr);
    lv_obj_set_size(rect_green, 120, 120);
    lv_obj_set_style_bg_color(rect_green, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect_green, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rect_green, 0, LV_PART_MAIN);
    lv_obj_align(rect_green, LV_ALIGN_CENTER, 0, 0);

    /* Синий квадрат */
    lv_obj_t *rect_blue = lv_obj_create(scr);
    lv_obj_set_size(rect_blue, 120, 120);
    lv_obj_set_style_bg_color(rect_blue, lv_color_hex(0x0000FF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect_blue, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rect_blue, 0, LV_PART_MAIN);
    lv_obj_align(rect_blue, LV_ALIGN_RIGHT_MID, -60, 0);

    /* Устанавливаем яркость подсветки на 80% */
    display_set_brightness(80);

    ESP_LOGI(TAG, "UI created successfully. Entering main loop...");
    ESP_LOGI(TAG, "Demo is running. Check your display!");

    /* Основной цикл уже реализован через esp_timer */
}
