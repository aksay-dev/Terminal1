/**
 * @file lv_conf.h
 * @brief LVGL v9.4 Configuration for ESP32-S3 Hello World Demo
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM 0

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 0

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/* Enable complex drawing features */
#define LV_DRAW_SW_COMPLEX 1

/*==================
 *   FONT USAGE
 *=================*/

/* Montserrat fonts - включаем нужные размеры для демо */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
 *  WIDGET USAGE
 *================*/
#define LV_USE_LABEL     1
#define LV_USE_BUTTON    1
#define LV_USE_IMAGE     1

/*-----------
 * Themes
 *----------*/
#define LV_USE_THEME_DEFAULT 1

/*-----------
 * Layouts
 *----------*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*-------------
 * Logging
 *-----------*/
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN  /* Changed from INFO to WARN to reduce spam */
    #define LV_LOG_PRINTF 1
#endif

#endif /* LV_CONF_H */

