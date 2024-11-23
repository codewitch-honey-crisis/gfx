//////////////////////////////////
// EXAMPLE
// Uses htcw_gfx with an m5 stack
// core2 to demonstrate images,
// fonts, and DMA enabled draws
//////////////////////////////////
// If defined, DMA will be used to improve performance
// with the use of two transfer buffers
#define USE_DMA


#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#endif
#include <memory.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
// ili9342 panel driver codewitch-honey-crisis/htcw_esp_lcd_panel_ili9342

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
// config for various devices
#include "lcd_config.h"
#ifdef M5STACK_CORE2
// cross platform i2c init codewitch-honey-crisis/htcw_esp_i2c
#include <esp_i2c.hpp>
// core2 power management codewitch-honey-crisis/htcw_m5core2_power
#include <m5core2_power.hpp>
#endif
// graphics library codewitch-honey-crisis/htcw_gfx
#include <gfx.hpp>
#define CAT_PNG_IMPLEMENTATION
#include "assets/cat_png.h"
#define SHARETECH_REGULAR_TTF_IMPLEMENTATION
#include "assets/ShareTech_Regular_ttf.h"

// import the htcw_gfx graphics library namespace
using namespace gfx;

// import the appropriate namespace for our other libraries
#ifdef ARDUINO
namespace arduino {}
using namespace arduino;
#else
namespace esp_idf {}
using namespace esp_idf;
#endif
#ifdef M5STACK_CORE2
// declare the power management driver
m5core2_power power(esp_i2c<1,21,22>::instance);
#endif
// declare a bitmap type for our frame buffer type (RGB565)
using fb_t = bitmap<rgb_pixel<16>>;
// get a color pseudo-enum for our bitmap type
using color_t = color<typename fb_t::pixel_type>;

// lcd data
// This is the max DMA transfer size
static const size_t lcd_transfer_buffer_size = (LCD_HRES*LCD_VRES*(LCD_BIT_DEPTH)+7)/8;
// for sending data to the display
static uint8_t *lcd_transfer_buffer = nullptr;
#ifdef USE_DMA
static uint8_t *lcd_transfer_buffer2 = nullptr;
#endif
// 0 = no flushes in progress, otherwise flushing
static volatile int lcd_flushing = 0;
static esp_lcd_panel_handle_t lcd_handle = nullptr;
#ifdef USE_DMA
static int lcd_dma_sel = 0;
#endif
static uint8_t *lcd_current_buffer() {
#ifdef USE_DMA
    return lcd_dma_sel ? lcd_transfer_buffer2 : lcd_transfer_buffer;
#else
    return lcd_transfer_buffer;
#endif

}
static void lcd_switch_buffers() {
#ifdef USE_DMA
    lcd_dma_sel++;
    if (lcd_dma_sel > 1) {
        lcd_dma_sel = 0;
    }
#endif
}
// indicates the LCD DMA transfer is complete
static bool lcd_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                            esp_lcd_panel_io_event_data_t *edata,
                            void *user_ctx) {
    lcd_flushing = 0;
    return true;
}
// waits for there to be a free buffer
static void lcd_wait_dma() { 
#ifdef USE_DMA
    while (lcd_flushing > 1);
#else
    while (lcd_flushing);
#endif
 }
// indicates we're about to start drawing
static void lcd_begin_draw() {
    lcd_wait_dma();
    lcd_flushing = lcd_flushing + 1;
}
// flush a bitmap to the display
static void lcd_flush_bitmap(int x1, int y1, int x2, int y2,
                             const void *bitmap) {
    //printf("Flush: (%d,%d)-(%d,%d)\n",x1,y1,x2,y2);
    // adjust end coordinates for a quirk of Espressif's API (add 1 to each)
    esp_lcd_panel_draw_bitmap(lcd_handle, x1, y1, x2 + 1, y2 + 1,
                              (void *)bitmap);
}
// initialize the screen using the esp panel API
// htcw_gfx no longer has intrinsic display driver support
// for performance and flash size reasons
// here we use the ESP LCD Panel API for it
static void lcd_panel_init() {
#ifdef LCD_PIN_NUM_BCKL
    if(LCD_PIN_NUM_BCKL>-1) {
        gpio_set_direction((gpio_num_t)LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)4, LCD_BCKL_OFF_LEVEL);
    }
#endif
    // configure the SPI bus
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.sclk_io_num = LCD_PIN_NUM_CLK;
    buscfg.mosi_io_num = LCD_PIN_NUM_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    // declare enough space for the transfer buffers + 8 bytes SPI DMA overhead
    buscfg.max_transfer_sz = lcd_transfer_buffer_size + 8;

    // Initialize the SPI bus on VSPI (SPI3)
    spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config;
    memset(&io_config, 0, sizeof(io_config));
    io_config.dc_gpio_num = LCD_PIN_NUM_DC;
    io_config.cs_gpio_num = LCD_PIN_NUM_CS;
    io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.spi_mode = 0;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = lcd_flush_ready;
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config,
                             &io_handle);

    lcd_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config;
    memset(&panel_config, 0, sizeof(panel_config));
    panel_config.reset_gpio_num = LCD_PIN_NUM_RST;
    if(LCD_COLOR_SPACE==ESP_LCD_COLOR_SPACE_RGB) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
#else
        panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
#endif
    } else {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;
#else
        panel_config.color_space = ESP_LCD_COLOR_SPACE_BGR;
#endif
    }
    panel_config.bits_per_pixel = LCD_BIT_DEPTH;

    // Initialize the LCD configuration
    if (ESP_OK !=
        LCD_PANEL(io_handle, &panel_config, &lcd_handle)) {
        printf("Error initializing LCD panel.\n");
        while (1) vTaskDelay(5);
    }

    // Reset the display
    esp_lcd_panel_reset(lcd_handle);

    // Initialize LCD panel
    esp_lcd_panel_init(lcd_handle);
    //  Swap x and y axis (Different LCD screens may need different options)
    esp_lcd_panel_swap_xy(lcd_handle, LCD_SWAP_XY);
    esp_lcd_panel_set_gap(lcd_handle, LCD_GAP_X, LCD_GAP_Y);
    esp_lcd_panel_mirror(lcd_handle, LCD_MIRROR_X, LCD_MIRROR_Y);
    esp_lcd_panel_invert_color(lcd_handle, LCD_INVERT_COLOR);
    // Turn on the screen
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_lcd_panel_disp_on_off(lcd_handle, true);
#else
    esp_lcd_panel_disp_off(lcd_handle, false);
#endif
#ifdef LCD_PIN_NUM_BCKL
    // Turn on backlight (Different LCD screens may need different levels)
    if(LCD_PIN_NUM_BCKL>-1) gpio_set_level((gpio_num_t)4, LCD_BCKL_ON_LEVEL);
#endif

    // initialize the transfer buffers
    lcd_transfer_buffer = (uint8_t *)malloc(lcd_transfer_buffer_size);
    if (lcd_transfer_buffer == nullptr) {
        puts("Out of memory initializing primary transfer buffer");
        while (1) vTaskDelay(5);
    }
    memset(lcd_transfer_buffer, 0, lcd_transfer_buffer_size);
#ifdef USE_DMA
    // initialize the transfer buffers
    lcd_transfer_buffer2 = (uint8_t *)malloc(lcd_transfer_buffer_size);
    if (lcd_transfer_buffer2 == nullptr) {
        puts("Out of memory initializing transfer buffer 2");
        while (1) vTaskDelay(5);
    }
    memset(lcd_transfer_buffer2, 0, lcd_transfer_buffer_size);
#endif
}

// prepare the font data in a stream
const_buffer_stream fnt_stm(ShareTech_Regular_ttf,sizeof(ShareTech_Regular_ttf));

// for the offscreen bitmap
fb_t scr_bmp;
// for erasing the txt
fb_t txt_bmp;
// the font and caches
// caches are optional, 
// but increase performance
tt_font scr_fnt(fnt_stm,40);
font_draw_cache fnt_draw_cache;
font_measure_cache fnt_measure_cache;
text_info txt_inf;
srect16 txt_rect;
spoint16 txt_deltas(2,1);
#ifdef ARDUINO
// entry point (arduino)
void setup()
{
    Serial.begin(115200);
#else
// arduino compat shims:
static uint32_t millis() {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}
static void* ps_malloc(size_t size) {
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM);
}
void loop();
static void loop_task(void* arg) {
    while(1) {
        static int count = 0;
        loop();
        // tickle the watchdog periodically
        if (count++ == 4) {
            count = 0;
            vTaskDelay(5);
        }
    }
}
// entry point (esp-idf)
extern "C" void app_main()
{
#endif
#ifdef M5STACK_CORE2
    // initialize the AXP192 in the core 2
    power.initialize();
#endif
    // initialize the LCD
    lcd_panel_init();

    // the offscreen bitmap for the display (in PSRAM)
    scr_bmp = create_bitmap<typename fb_t::pixel_type>({LCD_WIDTH,LCD_HEIGHT});
    if(scr_bmp.begin()==nullptr) {
        scr_bmp = create_bitmap<typename fb_t::pixel_type>({LCD_WIDTH,LCD_HEIGHT},::ps_malloc);
        if(scr_bmp.begin()==nullptr) {
            puts("This program requires PSRAM for a screen this size");
            while(1) vTaskDelay(5);
        }
    }
    // prepare the image data in a stream
    const_buffer_stream img_stm(cat_png,sizeof(cat_png));
    // load the image
    png_image img(img_stm,true);
    draw::image(scr_bmp,scr_bmp.bounds(),img);
    img.deinitialize(); // we don't need it now

    // initialize the caches
    fnt_draw_cache.initialize();
    fnt_measure_cache.initialize();
    fnt_draw_cache.max_entries(10);
    fnt_measure_cache.max_entries(10);

    txt_inf=text_info("caturday!",scr_fnt,4,text_encoding::utf8,&fnt_measure_cache,&fnt_draw_cache);
    size16 fnt_area;
    scr_fnt.initialize();
    // measure the text so we can center it
    scr_fnt.measure(LCD_WIDTH,txt_inf,&fnt_area);
    // it's always better to center using srect16s vs rect16s
    // because it allows for text larger than the center area
    txt_rect = ((srect16)fnt_area.bounds()).center((srect16)scr_bmp.bounds());
    // give it a 1 px border
    txt_rect.inflate_inplace(1,1);
    // transfer our offscreen bitmap to the display using DMA.
    // basically the idea here is that we use both buffers,
    // drawing to one while sending the other to maximize
    // throughput:
    for(int y = 0;y<LCD_HEIGHT;y+=40) {
        lcd_begin_draw();
        fb_t xbmp({LCD_WIDTH,40},lcd_current_buffer());
        draw::bitmap(xbmp,xbmp.bounds(),scr_bmp,rect16(0,y,LCD_WIDTH-1,y+39));
        lcd_flush_bitmap(0,y,LCD_WIDTH-1,y+39,xbmp.begin());
        lcd_switch_buffers();
    }
    // make sure our buffer isn't bigger than our transfer buffer
    assert(fb_t::sizeof_buffer((size16)txt_rect.dimensions())<=lcd_transfer_buffer_size);
    
    
    // ESP-IDF compat shim
#ifndef ARDUINO
    TaskHandle_t loop_handle;
    xTaskCreate(loop_task,"loop_task",4096,nullptr,20,&loop_handle);
#endif
}
void loop()
{
    lcd_begin_draw();
    fb_t xbmp((size16)txt_rect.dimensions(),lcd_current_buffer());
    // fill the transfer buffer
    // with the screen data
    draw::bitmap(xbmp,xbmp.bounds(),scr_bmp,(rect16)txt_rect);
    // draw the text
    draw::text(xbmp,xbmp.bounds(),txt_inf,color_t::purple);
    // transfer to the display
    lcd_flush_bitmap(txt_rect.x1,txt_rect.y1,txt_rect.x2,txt_rect.y2,xbmp.begin());
    // select the next transfer buffer
    lcd_switch_buffers();
    // update the deltas
    if(txt_rect.x1+txt_deltas.x<0 || txt_rect.x2+txt_deltas.x>=LCD_WIDTH) {
        txt_deltas.x=-txt_deltas.x;
    }
    if(txt_rect.y1+txt_deltas.y<0 || txt_rect.y2+txt_deltas.y>=LCD_HEIGHT) {
        txt_deltas.y=-txt_deltas.y;
    }
    // move the text rectangle
    txt_rect.offset_inplace(txt_deltas);
    
     // for calculating the frames per second.
    static int frames = 0;
    ++frames;
    static uint64_t update_ts = 0;
    // once every second:
    if(millis()>update_ts+1000) {
        update_ts=millis();
        // print the frames per second and average frame time
        printf("%d FPS - frame time: %0.2fms\n",frames,1000.f/((float)frames));
        frames = 0;
    }
}
