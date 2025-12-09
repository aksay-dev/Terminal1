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
static void update_arc_color(lv_obj_t *arc, int val);
static void arc_event_cb(lv_event_t *e);
static lv_obj_t *s_lbl_temp = NULL;

void app_main(void) 
{
    ESP_LOGI(TAG, "=== Hello World LVGL v9.4 Demo ===");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());

    /* Инициализация дисплея ST7701 RGB 480x480 */
    display_init();
    ESP_LOGI(TAG, "Display initialized successfully");

    /* ========== Термостат с круговым слайдером ========== */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x111827), LV_PART_MAIN); /* тёмный фон */
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Заголовок */
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "Thermostat");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x00d1ff), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 24);

    /* Подзаголовок */
    lv_obj_t *lbl_sub = lv_label_create(scr);
    lv_label_set_text(lbl_sub, "Adjust with the ring");
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0x9ca3af), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(lbl_sub, LV_ALIGN_TOP_MID, 0, 56);

    /* Центральный индикатор температуры */
    s_lbl_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(s_lbl_temp, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_lbl_temp, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(s_lbl_temp, LV_ALIGN_CENTER, 0, -10);

    /* Доп. подпись */
    lv_obj_t *lbl_units = lv_label_create(scr);
    lv_label_set_text(lbl_units, "Target");
    lv_obj_set_style_text_color(lbl_units, lv_color_hex(0x9ca3af), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_units, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(lbl_units, LV_ALIGN_CENTER, 0, 50);

    /* Дуга как круговой слайдер */
    lv_obj_t *arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 300, 300);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_range(arc, 15, 30); /* 15..30 °C */
    lv_arc_set_value(arc, 22);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 20, LV_PART_KNOB);
    lv_obj_set_style_radius(arc, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_PART_INDICATOR | LV_PART_KNOB);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB); /* убрать стандартный фон knobs */
    lv_obj_set_style_bg_color(arc, lv_color_hex(0x111827), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_MAIN);
    update_arc_color(arc, 22);
    lv_obj_add_event_cb(arc, arc_event_cb, LV_EVENT_VALUE_CHANGED, s_lbl_temp);

    /* Обновляем текст сразу */
    lv_label_set_text_fmt(s_lbl_temp, "%d°C", (int)lv_arc_get_value(arc));

    /* Подсказка внизу */
    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "Rotate the ring to set temperature");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x6b7280), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -12);

    /* Устанавливаем яркость подсветки на 80% */
    display_set_brightness(80);

    ESP_LOGI(TAG, "UI created successfully. Entering main loop...");
    ESP_LOGI(TAG, "Demo is running. Check your display!");

    /* Основной цикл уже реализован через esp_timer */
}

static void update_arc_color(lv_obj_t *arc, int val)
{
    lv_color_t c = lv_color_hex(0x00d1ff); /* default cool */
    if (val >= 26) {
        c = lv_color_hex(0xff7a3d);       /* warm */
    } else if (val >= 22) {
        c = lv_color_hex(0x4ade80);       /* comfort */
    }
    lv_obj_set_style_arc_color(arc, c, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, c, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, c, LV_PART_KNOB);
}

static void arc_event_cb(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    int v = lv_arc_get_value(arc);
    lv_label_set_text_fmt(label, "%d°C", v);
    update_arc_color(arc, v);
}
