/**
 * @file main.c
 * @brief Красивый экран термостата с круговым слайдером (GT911 + LVGL v9.4)
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "font/lv_font.h"
#include "lvgl.h"
#include "display.h"
#include "touch.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "THERMOSTAT";

static lv_obj_t *label_set = NULL;
static lv_obj_t *label_room = NULL;
static lv_obj_t *label_state = NULL;
static lv_obj_t *arc = NULL;
static lv_indev_t *touch_indev = NULL;

/* Температуры хранятся в десятых долях градуса, чтобы избежать float */
static int32_t setpoint = 225; /* 22.5 °C */
static int32_t room_temp = 215; /* 21.5 °C, будет анимироваться к setpoint */

static void update_labels(void)
{
    lv_label_set_text_fmt(label_set, "SET  %d.%d°C", (int16_t)setpoint / 10, (int16_t)setpoint % 10);
    lv_label_set_text_fmt(label_room, "ROOM %d.%d°C", (int16_t)room_temp / 10, (int16_t)room_temp % 10);

    int diff = setpoint - room_temp;
    const char *state = (diff > 5) ? "HEATING" : (diff < -5) ? "COOLING" : "HOLD";
    lv_label_set_text_fmt(label_state, "%s", state);
    lv_obj_set_style_text_color(label_state,
        (diff > 5) ? lv_color_hex(0xff7a3d) : (diff < -5) ? lv_color_hex(0x00d1ff) : lv_color_hex(0x4ade80),
        LV_PART_MAIN);
}

static void arc_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t v = lv_arc_get_value(arc); /* диапазон арки в десятых градуса */
        setpoint = v;
        update_labels();
    }
}

static void room_temp_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    /* Простая динамика: стремимся к setpoint */
    if (room_temp < setpoint) room_temp++;
    else if (room_temp > setpoint) room_temp--;
    update_labels();
}

/* Touch → LVGL input */
static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int16_t xs[5], ys[5];
    uint8_t cnt = 0;
    bool pressed = touch_read_points(xs, ys, &cnt);
    if (pressed && cnt > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = xs[0];
        data->point.y = ys[0];
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void app_main(void) 
{
    ESP_LOGI(TAG, "=== Thermostat UI ===");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());

    /* Инициализация дисплея ST7701 RGB 480x480 */
    display_init();
    ESP_LOGI(TAG, "Display initialized successfully");

    /* Инициализация тачпанели GT911 */
    if (!touch_init()) {
        ESP_LOGE(TAG, "Failed to initialize touch panel!");
    } else {
        touch_set_resolution(480, 480);
        touch_set_rotation(TOUCH_ROT_NORMAL);
    }

    /* UI */
    lv_obj_t *scr = lv_screen_active();

    /* Фон с мягким градиентом */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0d1b2a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Заголовок */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Smart Thermostat");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00d1ff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    /* Арка (круговой слайдер) */
    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 380, 380);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 20);
    lv_arc_set_range(arc, 150, 300); /* 15.0 - 30.0 °C в десятых */
    lv_arc_set_value(arc, setpoint);
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_arc_set_angles(arc, 135, 405);
    lv_obj_set_style_arc_width(arc, 32, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 32, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1e293b), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1e293b), LV_PART_KNOB);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x4ade80), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_add_event_cb(arc, arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Текущая/уставка */
    label_set = lv_label_create(scr);
    lv_obj_set_style_text_font(label_set, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_set, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label_set, LV_ALIGN_CENTER, 0, -80);

    label_room = lv_label_create(scr);
    lv_obj_set_style_text_font(label_room, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_room, lv_color_hex(0x94a3b8), LV_PART_MAIN);
    lv_obj_align(label_room, LV_ALIGN_CENTER, 0, -40);

    /* Статус (нагрев/охлаждение/поддержание) */
    label_state = lv_label_create(scr);
    lv_obj_set_style_text_font(label_state, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_state, lv_color_hex(0x4ade80), LV_PART_MAIN);
    lv_obj_align(label_state, LV_ALIGN_CENTER, 0, 60);

    /* Нижняя подпись */
    lv_obj_t *footer = lv_label_create(scr);
    lv_label_set_text(footer, "Touch to set temperature");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x64748b), LV_PART_MAIN);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -24);

    /* Input device для LVGL */
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touchpad_read_cb);

    /* Таймер для плавного изменения «комнатной» температуры */
    lv_timer_create(room_temp_timer_cb, 300, NULL);

    /* Устанавливаем яркость подсветки */
    display_set_brightness(90);

    /* Первичное обновление UI */
    update_labels();

    ESP_LOGI(TAG, "Thermostat UI ready. Rotate arc (touch) to change setpoint.");
}
