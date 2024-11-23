#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#ifdef TTGO_T1
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_DMA
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 19
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 16
#define LCD_PIN_NUM_RST 23
#define LCD_PIN_NUM_BCKL 4
#define LCD_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 135
#define LCD_VRES 240
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 40
#define LCD_GAP_Y 52
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y true
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // TTGO_T1

#ifdef M5STACK_CORE2
#include <esp_lcd_panel_ili9342.h>
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_DMA
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 23
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 15
#define LCD_PANEL esp_lcd_new_panel_ili9342
#define LCD_HRES 320
#define LCD_VRES 240
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY false
#endif // M5STACK_CORE2

#ifdef M5STACK_FIRE
#include <esp_lcd_panel_ili9342.h>
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_DMA
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 23
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 14
#define LCD_PIN_NUM_DC 27
#define LCD_PIN_NUM_RST 33
#define LCD_PIN_NUM_BCKL 32
#define LCD_PANEL esp_lcd_new_panel_ili9342
#define LCD_HRES 240
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // M5STACK_FIRE

#ifdef ESP32_S3_DEVKIT_C1
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_DMA
#define LCD_BCKL_ON_LEVEL 0
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 13
#define LCD_PIN_NUM_CLK 12
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 11
#define LCD_PIN_NUM_RST 9
#define LCD_PIN_NUM_BCKL 10
#define LCD_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 240
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y true
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // ESP32_S3_DEVKIT_C1

#ifndef LCD_PIN_NUM_RST
#define LCD_PIN_NUM_RST -1
#endif
#ifndef LCD_WIDTH
#ifdef LCD_SWAP_XY
#if LCD_SWAP_XY
#define LCD_WIDTH LCD_VRES
#define LCD_HEIGHT LCD_HRES
#else
#define LCD_WIDTH LCD_HRES
#define LCD_HEIGHT LCD_VRES
#endif
#else
#define LCD_WIDTH LCD_HRES
#define LCD_HEIGHT LCD_VRES
#endif
#endif
#ifndef LCD_BIT_DEPTH
#define LCD_BIT_DEPTH 16
#endif
#endif // LCD_CONFIG_H