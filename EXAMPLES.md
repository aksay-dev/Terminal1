# Примеры расширения Hello World Demo

## Пример 1: Добавить обработчик нажатия кнопки

Замените создание кнопки в `main.c`:

```c
/* Счетчик нажатий */
static int button_clicks = 0;
static lv_obj_t *lbl_counter = NULL;

/* Обработчик события кнопки */
static void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        button_clicks++;
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "Clicks: %d", button_clicks);
        lv_label_set_text(lbl_counter, buf);
        ESP_LOGI("BTN", "Button clicked %d times", button_clicks);
    }
}

void app_main(void) 
{
    // ... инициализация дисплея и создание лейблов ...

    /* Кнопка с обработчиком */
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);

    /* Счетчик кликов */
    lbl_counter = lv_label_create(scr);
    lv_label_set_text(lbl_counter, "Clicks: 0");
    lv_obj_set_style_text_color(lbl_counter, lv_color_hex(0xffaa00), LV_PART_MAIN);
    lv_obj_align(lbl_counter, LV_ALIGN_BOTTOM_MID, 0, -110);
}
```

## Пример 2: Добавить анимацию

```c
/* Анимация изменения цвета */
static void color_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFF0000 + v * 256), LV_PART_MAIN);
}

void app_main(void) 
{
    // ... создание UI ...

    /* Создаем анимацию для главного заголовка */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, lbl_hello);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)color_anim_cb);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 3000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_playback_time(&a, 1500);
    lv_anim_start(&a);
}
```

## Пример 3: Добавить часы реального времени

```c
static lv_obj_t *lbl_clock = NULL;
static uint32_t seconds_elapsed = 0;

/* Таймер для обновления часов */
static void clock_timer_cb(lv_timer_t *timer)
{
    seconds_elapsed++;
    uint32_t hours = (seconds_elapsed / 3600) % 24;
    uint32_t minutes = (seconds_elapsed / 60) % 60;
    uint32_t secs = seconds_elapsed % 60;
    
    char time_str[16];
    lv_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
                (int)hours, (int)minutes, (int)secs);
    lv_label_set_text(lbl_clock, time_str);
}

void app_main(void) 
{
    // ... инициализация ...

    /* Создаем лейбл часов */
    lbl_clock = lv_label_create(scr);
    lv_label_set_text(lbl_clock, "00:00:00");
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xffff00), LV_PART_MAIN);
    lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -20, 20);

    /* Создаем таймер LVGL для обновления каждую секунду */
    lv_timer_create(clock_timer_cb, 1000, NULL);
}
```

## Пример 4: Добавить слайдер яркости

Включите в `lv_conf.h`:
```c
#define LV_USE_SLIDER 1
```

Затем в `main.c`:
```c
/* Обработчик изменения слайдера */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    display_set_brightness((uint8_t)value);
    ESP_LOGI("SLIDER", "Brightness set to %d%%", (int)value);
}

void app_main(void) 
{
    // ... создание UI ...

    /* Лейбл "Brightness" */
    lv_obj_t *lbl_bright = lv_label_create(scr);
    lv_label_set_text(lbl_bright, "Brightness:");
    lv_obj_align(lbl_bright, LV_ALIGN_TOP_LEFT, 20, 20);

    /* Слайдер яркости */
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 200);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 20, 50);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
```

## Пример 5: Создать несколько экранов (страниц)

```c
static lv_obj_t *screen_main = NULL;
static lv_obj_t *screen_settings = NULL;

/* Переключение на экран настроек */
static void goto_settings_cb(lv_event_t *e)
{
    lv_screen_load_anim(screen_settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

/* Возврат на главный экран */
static void goto_main_cb(lv_event_t *e)
{
    lv_screen_load_anim(screen_main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}

void app_main(void) 
{
    // ... инициализация ...

    /* === ГЛАВНЫЙ ЭКРАН === */
    screen_main = lv_screen_active();
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    lv_obj_t *lbl_main = lv_label_create(screen_main);
    lv_label_set_text(lbl_main, "Main Screen");
    lv_obj_set_style_text_font(lbl_main, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_center(lbl_main);

    /* Кнопка "Settings" */
    lv_obj_t *btn_settings = lv_button_create(screen_main);
    lv_obj_set_size(btn_settings, 150, 50);
    lv_obj_align(btn_settings, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_event_cb(btn_settings, goto_settings_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_btn = lv_label_create(btn_settings);
    lv_label_set_text(lbl_btn, "Settings");
    lv_obj_center(lbl_btn);

    /* === ЭКРАН НАСТРОЕК === */
    screen_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings, lv_color_hex(0x2e1a1a), LV_PART_MAIN);

    lv_obj_t *lbl_settings = lv_label_create(screen_settings);
    lv_label_set_text(lbl_settings, "Settings");
    lv_obj_set_style_text_font(lbl_settings, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(lbl_settings, LV_ALIGN_CENTER, 0, -50);

    /* Кнопка "Back" */
    lv_obj_t *btn_back = lv_button_create(screen_settings);
    lv_obj_set_size(btn_back, 150, 50);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_event_cb(btn_back, goto_main_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
}
```

## Пример 6: Добавить график (chart)

Включите в `lv_conf.h`:
```c
#define LV_USE_CHART 1
```

Затем в `main.c`:
```c
static lv_obj_t *chart = NULL;
static lv_chart_series_t *ser1 = NULL;

/* Таймер для обновления графика */
static void chart_timer_cb(lv_timer_t *timer)
{
    /* Добавляем случайное значение */
    lv_chart_set_next_value(chart, ser1, lv_rand(10, 90));
}

void app_main(void) 
{
    // ... инициализация ...

    /* Создаем график */
    chart = lv_chart_create(scr);
    lv_obj_set_size(chart, 300, 200);
    lv_obj_center(chart);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    /* Добавляем серию данных */
    ser1 = lv_chart_add_series(chart, lv_color_hex(0x00ff88), LV_CHART_AXIS_PRIMARY_Y);

    /* Заполняем начальными данными */
    for (int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, ser1, lv_rand(10, 90));
    }

    /* Таймер для обновления графика каждую секунду */
    lv_timer_create(chart_timer_cb, 1000, NULL);
}
```

## Пример 7: Добавить Arc (круговой индикатор)

Включите в `lv_conf.h`:
```c
#define LV_USE_ARC 1
```

```c
static lv_obj_t *arc = NULL;
static int16_t arc_value = 0;

/* Анимация вращения arc */
static void arc_timer_cb(lv_timer_t *timer)
{
    arc_value = (arc_value + 5) % 360;
    lv_arc_set_value(arc, arc_value);
}

void app_main(void) 
{
    // ... инициализация ...

    /* Создаем круговой индикатор */
    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 200, 200);
    lv_arc_set_range(arc, 0, 360);
    lv_arc_set_value(arc, 0);
    lv_obj_center(arc);
    
    /* Стиль */
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00ff88), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 15, LV_PART_MAIN);

    /* Таймер для анимации */
    lv_timer_create(arc_timer_cb, 50, NULL);
}
```

## Комбинирование примеров

Вы можете объединить несколько примеров в одном проекте:
1. Часы в верхнем правом углу
2. Слайдер яркости слева
3. График в центре
4. Кнопка переключения экранов внизу

## Дополнительные ресурсы

**Официальные примеры LVGL v9:**
- https://github.com/lvgl/lvgl/tree/master/examples

**Интерактивные примеры:**
- https://docs.lvgl.io/master/examples.html

**Виджеты:**
- Chart: https://docs.lvgl.io/master/widgets/chart.html
- Slider: https://docs.lvgl.io/master/widgets/slider.html
- Arc: https://docs.lvgl.io/master/widgets/arc.html
- Button: https://docs.lvgl.io/master/widgets/button.html

**Анимации:**
- https://docs.lvgl.io/master/overview/animation.html

**События:**
- https://docs.lvgl.io/master/overview/event.html

