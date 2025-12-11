idf_component_register(SRCS "touch.c" "main.c" "display.c"
                       INCLUDE_DIRS "."
                       REQUIRES lvgl esp_lcd esp_timer)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)