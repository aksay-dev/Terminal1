/**
 * @file main.c
 * @brief Простой демо "Hello, World!" для ESP32-S3 с LVGL v9.4
 * 
 * Демонстрирует вывод текста на экран 480x480 через RGB интерфейс
 * Основан на проекте 4.0_LvglWidgets
 */

#include <stdio.h>
#include "lvgl.h"
#include "display.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "HELLO_WORLD";

/* Таймер для LVGL обработчика */
static void lvgl_timer_cb(void *arg)
{
    (void)arg;
    lv_timer_handler();
}

void app_main(void) 
{
    ESP_LOGI(TAG, "=== Hello World LVGL v9.4 Demo ===");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());

    /* Инициализация дисплея ST7701 RGB 480x480 */
    display_init();
    ESP_LOGI(TAG, "Display initialized successfully");

    /* Периодический вызов lv_timer_handler() каждые 5 мс */
    const esp_timer_create_args_t args = {
        .callback = &lvgl_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_timer"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 5 * 1000)); /* 5 мс */

    /* ========== Создание UI ========== */
    
    /* Получаем активный экран */
    lv_obj_t *scr = lv_screen_active();
    
    /* Устанавливаем темный градиентный фон */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Создаем главный заголовок "Hello, World!" */
    lv_obj_t *lbl_hello = lv_label_create(scr);
    lv_label_set_text(lbl_hello, "Hello, World!");
    lv_obj_set_style_text_color(lbl_hello, lv_color_hex(0x00ff88), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hello, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(lbl_hello, LV_ALIGN_CENTER, 0, -80);

    /* Создаем подзаголовок */
    lv_obj_t *lbl_subtitle = lv_label_create(scr);
    lv_label_set_text(lbl_subtitle, "LVGL v9.4 on ESP32-S3");
    lv_obj_set_style_text_color(lbl_subtitle, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_subtitle, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(lbl_subtitle, LV_ALIGN_CENTER, 0, 0);

    /* Информация о разрешении */
    lv_obj_t *lbl_info = lv_label_create(scr);
    lv_label_set_text(lbl_info, "Resolution: 480 x 480\nST7701 RGB Display");
    lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_info, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_info, LV_ALIGN_CENTER, 0, 80);

    /* Добавляем кнопку для интерактивности */
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0077ff), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Press Me!");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(btn_label);

    /* Устанавливаем яркость подсветки на 80% */
    display_set_brightness(80);

    ESP_LOGI(TAG, "UI created successfully. Entering main loop...");
    ESP_LOGI(TAG, "Demo is running. Check your display!");

    /* Основной цикл уже реализован через esp_timer */
}
