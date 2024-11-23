//////////////////////////////////
// EXAMPLE
// Uses htcw_gfx with an esp32
// to demonstrate animation,
// DMA enabled draws, and 
// "blt spans"
//////////////////////////////////

// If indicated, use spans to manipulate the bitmap
// Spans can be faster in many circumstances than
// using point() and fill() methods because they
// give you pointer access to the bitmap data
// for direct manipulation:
#define USE_SPANS
// If defined, the FPS will be displayed on the screen
// (robs performance, except w/ DMA enabled, which can
// draw while transfering)
#define SHOW_FPS
// If defined, DMA will be used to improve performance
// with the use of two transfer buffers. The idea is
// we draw to one while the hardware is sending the 
// other in order to maximize utilization and throughput
#define USE_DMA

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#endif
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <memory.h>
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
// include the windows 3.1 raster font
// converted to a header using
// https://honeythecodewitch.com/gfx/converter
// (C++ mode, no arduino)
#define VGA_8X8_IMPLEMENTATION
#include "assets/vga_8x8.h"
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
m5core2_power power(esp_i2c<1, 21, 22>::instance);
#endif

static uint16_t screen_lines = 0;

// declare a bitmap type for our frame buffer type (RGB565 or rgb 16-bit color)
using fb_t = bitmap<rgb_pixel<16>>;
// get a color pseudo-enum for our bitmap type
using color_t = color<typename fb_t::pixel_type>;

// lcd data
// This works out to be 32KB - the max DMA transfer size
static const size_t lcd_transfer_buffer_size = 32*1024;
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

    // Initialize the SPI bus
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
// get a color psuedo enumeration for our bitmap's same pixel type
using color_t = color<typename fb_t::pixel_type>;
// we upscale and produce 2x2 squares for each fire "pixel"
constexpr static const size_t fire_buffer_width = (LCD_WIDTH / 4);
constexpr static const size_t fire_buffer_height = ((LCD_HEIGHT / 4) + 6);
#ifdef USE_SPANS
#define PAL_TYPE uint16_t
// store preswapped uint16_ts for performance
#define RGB(r,g,b) (bits::swap(fb_t::pixel_type(r,g,b)))
#else
// store rgb_pixel<16> instances
#define PAL_TYPE fb_t::pixel_type
#define RGB(r,g,b) fb_t::pixel_type(r,g,b)
#endif
// color palette for flames
static PAL_TYPE fire_palette[] = {
    RGB(0, 0, 0),    RGB(0, 0, 3),    RGB(0, 0, 3),
    RGB(0, 0, 3),    RGB(0, 0, 4),    RGB(0, 0, 4),
    RGB(0, 0, 4),    RGB(0, 0, 5),    RGB(1, 0, 5),
    RGB(2, 0, 4),    RGB(3, 0, 4),    RGB(4, 0, 4),
    RGB(5, 0, 3),    RGB(6, 0, 3),    RGB(7, 0, 3),
    RGB(8, 0, 2),    RGB(9, 0, 2),    RGB(10, 0, 2),
    RGB(11, 0, 2),   RGB(12, 0, 1),   RGB(13, 0, 1),
    RGB(14, 0, 1),   RGB(15, 0, 0),   RGB(16, 0, 0),
    RGB(16, 0, 0),   RGB(16, 0, 0),   RGB(17, 0, 0),
    RGB(17, 0, 0),   RGB(18, 0, 0),   RGB(18, 0, 0),
    RGB(18, 0, 0),   RGB(19, 0, 0),   RGB(19, 0, 0),
    RGB(20, 0, 0),   RGB(20, 0, 0),   RGB(20, 0, 0),
    RGB(21, 0, 0),   RGB(21, 0, 0),   RGB(22, 0, 0),
    RGB(22, 0, 0),   RGB(23, 1, 0),   RGB(23, 1, 0),
    RGB(24, 2, 0),   RGB(24, 2, 0),   RGB(25, 3, 0),
    RGB(25, 3, 0),   RGB(26, 4, 0),   RGB(26, 4, 0),
    RGB(27, 5, 0),   RGB(27, 5, 0),   RGB(28, 6, 0),
    RGB(28, 6, 0),   RGB(29, 7, 0),   RGB(29, 7, 0),
    RGB(30, 8, 0),   RGB(30, 8, 0),   RGB(31, 9, 0),
    RGB(31, 9, 0),   RGB(31, 10, 0),  RGB(31, 10, 0),
    RGB(31, 11, 0),  RGB(31, 11, 0),  RGB(31, 12, 0),
    RGB(31, 12, 0),  RGB(31, 13, 0),  RGB(31, 13, 0),
    RGB(31, 14, 0),  RGB(31, 14, 0),  RGB(31, 15, 0),
    RGB(31, 15, 0),  RGB(31, 16, 0),  RGB(31, 16, 0),
    RGB(31, 17, 0),  RGB(31, 17, 0),  RGB(31, 18, 0),
    RGB(31, 18, 0),  RGB(31, 19, 0),  RGB(31, 19, 0),
    RGB(31, 20, 0),  RGB(31, 20, 0),  RGB(31, 21, 0),
    RGB(31, 21, 0),  RGB(31, 22, 0),  RGB(31, 22, 0),
    RGB(31, 23, 0),  RGB(31, 24, 0),  RGB(31, 24, 0),
    RGB(31, 25, 0),  RGB(31, 25, 0),  RGB(31, 26, 0),
    RGB(31, 26, 0),  RGB(31, 27, 0),  RGB(31, 27, 0),
    RGB(31, 28, 0),  RGB(31, 28, 0),  RGB(31, 29, 0),
    RGB(31, 29, 0),  RGB(31, 30, 0),  RGB(31, 30, 0),
    RGB(31, 31, 0),  RGB(31, 31, 0),  RGB(31, 32, 0),
    RGB(31, 32, 0),  RGB(31, 33, 0),  RGB(31, 33, 0),
    RGB(31, 34, 0),  RGB(31, 34, 0),  RGB(31, 35, 0),
    RGB(31, 35, 0),  RGB(31, 36, 0),  RGB(31, 36, 0),
    RGB(31, 37, 0),  RGB(31, 38, 0),  RGB(31, 38, 0),
    RGB(31, 39, 0),  RGB(31, 39, 0),  RGB(31, 40, 0),
    RGB(31, 40, 0),  RGB(31, 41, 0),  RGB(31, 41, 0),
    RGB(31, 42, 0),  RGB(31, 42, 0),  RGB(31, 43, 0),
    RGB(31, 43, 0),  RGB(31, 44, 0),  RGB(31, 44, 0),
    RGB(31, 45, 0),  RGB(31, 45, 0),  RGB(31, 46, 0),
    RGB(31, 46, 0),  RGB(31, 47, 0),  RGB(31, 47, 0),
    RGB(31, 48, 0),  RGB(31, 48, 0),  RGB(31, 49, 0),
    RGB(31, 49, 0),  RGB(31, 50, 0),  RGB(31, 50, 0),
    RGB(31, 51, 0),  RGB(31, 52, 0),  RGB(31, 52, 0),
    RGB(31, 52, 0),  RGB(31, 52, 0),  RGB(31, 52, 0),
    RGB(31, 53, 0),  RGB(31, 53, 0),  RGB(31, 53, 0),
    RGB(31, 53, 0),  RGB(31, 54, 0),  RGB(31, 54, 0),
    RGB(31, 54, 0),  RGB(31, 54, 0),  RGB(31, 54, 0),
    RGB(31, 55, 0),  RGB(31, 55, 0),  RGB(31, 55, 0),
    RGB(31, 55, 0),  RGB(31, 56, 0),  RGB(31, 56, 0),
    RGB(31, 56, 0),  RGB(31, 56, 0),  RGB(31, 57, 0),
    RGB(31, 57, 0),  RGB(31, 57, 0),  RGB(31, 57, 0),
    RGB(31, 57, 0),  RGB(31, 58, 0),  RGB(31, 58, 0),
    RGB(31, 58, 0),  RGB(31, 58, 0),  RGB(31, 59, 0),
    RGB(31, 59, 0),  RGB(31, 59, 0),  RGB(31, 59, 0),
    RGB(31, 60, 0),  RGB(31, 60, 0),  RGB(31, 60, 0),
    RGB(31, 60, 0),  RGB(31, 60, 0),  RGB(31, 61, 0),
    RGB(31, 61, 0),  RGB(31, 61, 0),  RGB(31, 61, 0),
    RGB(31, 62, 0),  RGB(31, 62, 0),  RGB(31, 62, 0),
    RGB(31, 62, 0),  RGB(31, 63, 0),  RGB(31, 63, 0),
    RGB(31, 63, 1),  RGB(31, 63, 1),  RGB(31, 63, 2),
    RGB(31, 63, 2),  RGB(31, 63, 3),  RGB(31, 63, 3),
    RGB(31, 63, 4),  RGB(31, 63, 4),  RGB(31, 63, 5),
    RGB(31, 63, 5),  RGB(31, 63, 5),  RGB(31, 63, 6),
    RGB(31, 63, 6),  RGB(31, 63, 7),  RGB(31, 63, 7),
    RGB(31, 63, 8),  RGB(31, 63, 8),  RGB(31, 63, 9),
    RGB(31, 63, 9),  RGB(31, 63, 10), RGB(31, 63, 10),
    RGB(31, 63, 10), RGB(31, 63, 11), RGB(31, 63, 11),
    RGB(31, 63, 12), RGB(31, 63, 12), RGB(31, 63, 13),
    RGB(31, 63, 13), RGB(31, 63, 14), RGB(31, 63, 14),
    RGB(31, 63, 15), RGB(31, 63, 15), RGB(31, 63, 15),
    RGB(31, 63, 16), RGB(31, 63, 16), RGB(31, 63, 17),
    RGB(31, 63, 17), RGB(31, 63, 18), RGB(31, 63, 18),
    RGB(31, 63, 19), RGB(31, 63, 19), RGB(31, 63, 20),
    RGB(31, 63, 20), RGB(31, 63, 21), RGB(31, 63, 21),
    RGB(31, 63, 21), RGB(31, 63, 22), RGB(31, 63, 22),
    RGB(31, 63, 23), RGB(31, 63, 23), RGB(31, 63, 24),
    RGB(31, 63, 24), RGB(31, 63, 25), RGB(31, 63, 25),
    RGB(31, 63, 26), RGB(31, 63, 26), RGB(31, 63, 26),
    RGB(31, 63, 27), RGB(31, 63, 27), RGB(31, 63, 28),
    RGB(31, 63, 28), RGB(31, 63, 29), RGB(31, 63, 29),
    RGB(31, 63, 30), RGB(31, 63, 30), RGB(31, 63, 31),
    RGB(31, 63, 31)};

static uint8_t fire_buffer[fire_buffer_height][fire_buffer_width];  // frame buffer, quarter
                                                 // resolution w/extra lines
// prepare our font data by wrapping it with a stream
static const_buffer_stream fps_font_stm(vga_8x8,sizeof(vga_8x8));
// feed the stream to the font 
// note that win fonts are very fast
static win_font fps_font(fps_font_stm,0);

// paint the current SCREEN_LINES scanlines of the fire
static void fire_paint(int y_pos, const char* sz) {
    // wrap the current transfer buffer with a bitmap object (lightweight)
    bitmap<fb_t::pixel_type> xfer_bmp({LCD_WIDTH,screen_lines},lcd_current_buffer());
    // tell the lcd subsystem we're about to write to the transfer buffer
    lcd_begin_draw();
    // compute the bounds
    rect16 bounds = rect16(point16(0,y_pos), xfer_bmp.dimensions());
    for (int y = bounds.y1; y <= bounds.y2; y += 2) {
#ifdef USE_SPANS
        // must use rgb_pixel<16>
        static_assert(helpers::is_same<rgb_pixel<16>,fb_t::pixel_type>::value,"USE_SPANS only works with RGB565");
        // get the spans for the current two rows
        gfx_span row1 = xfer_bmp.span(point16(bounds.x1,y-y_pos)),
            row2 = xfer_bmp.span(point16(bounds.x1,y-y_pos+1));
        // get the pointers
        uint16_t *prow1 = (uint16_t*)row1.data;
        uint16_t *prow2 = (uint16_t*)row2.data;
#endif
        for (int x = bounds.x1; x <= bounds.x2; x += 2) {
            // compute the color from the fire buffer
            int i = y >> 2; // quarter res x and y
            int j = x >> 2;
            // fill a 2x2 grid at the location offset by y_pos
            PAL_TYPE px = fire_palette[fire_buffer[i][j]];
#ifdef USE_SPANS
            *(prow1++)=px;
            *(prow1++)=px;
            *(prow2++)=px;
            *(prow2++)=px;
#else
            xfer_bmp.fill(rect16(point16(x,y-y_pos),size16(2,2)),px);
#endif
        }
    }
    // if we have text, draw it
    if(sz!=nullptr) {
        text_info ti(sz,fps_font,4,text_encoding::utf8);
        draw::text(xfer_bmp,srect16(10,10,LCD_WIDTH-1,screen_lines-1),ti,color<decltype(xfer_bmp)::pixel_type>::yellow);
    }
    // send the bitmap to the display
    lcd_flush_bitmap(0, y_pos, LCD_WIDTH-1, y_pos + screen_lines-1, lcd_current_buffer());
    // change buffers
    lcd_switch_buffers();
}

#ifdef ARDUINO
// entry point (arduino)
void setup() {
    Serial.begin(115200);
    setCpuFrequencyMhz(240);
#else
// arduino compat shims:
static uint32_t millis() { return pdTICKS_TO_MS(xTaskGetTickCount()); }
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
extern "C" void app_main() {
#endif
#ifdef M5STACK_CORE2
    // initialize the AXP192 in the core 2
    power.initialize();
#endif
    // initialize the LCD
    lcd_panel_init();

    fps_font.initialize();

    // compute the number of transfer buffer lines
    // evenly divisible and less than or equal to
    // the max transfer buffer size
    for(int i = 2;i<=LCD_HEIGHT;++i) {
        if(0==(LCD_HEIGHT%i)) {
            screen_lines = LCD_HEIGHT/i;
            if(fb_t::sizeof_buffer({LCD_WIDTH,screen_lines})<=lcd_transfer_buffer_size) {
                break;
            }
        }
    }
    assert(screen_lines>0);
    printf("Transfer lines: %d\n",(int)screen_lines);
    // ESP-IDF compat shim
#ifndef ARDUINO
    TaskHandle_t loop_handle;
    xTaskCreate(loop_task,"loop_task",4096,nullptr,20,&loop_handle);
#endif
}
void loop() {
    // timestamp for start
    uint32_t ms = millis();
    static char szfps[64]={0};
    // render the frame    
    unsigned int i, j, delta=0,y=0,adv=0;  // looping variables, counters, and data
    for (i = 1; i < fire_buffer_height; ++i) {
        for (j = 0; j < fire_buffer_width; ++j) {
            if (j == 0)
                fire_buffer[i - 1][j] =
                    (fire_buffer[i][j] + fire_buffer[i - 1][fire_buffer_width - 1] +
                        fire_buffer[i][j + 1] + fire_buffer[i + 1][j]) >>
                    2;
            else if (j == LCD_WIDTH / 4 - 1)
                fire_buffer[i - 1][j] =
                    (fire_buffer[i][j] + fire_buffer[i][j - 1] +
                        fire_buffer[i + 1][0] + fire_buffer[i + 1][j]) >>
                    2;
            else
                fire_buffer[i - 1][j] =
                    (fire_buffer[i][j] + fire_buffer[i][j - 1] +
                        fire_buffer[i][j + 1] + fire_buffer[i + 1][j]) >>
                    2;

            if (fire_buffer[i][j] > 11)
                fire_buffer[i][j] = fire_buffer[i][j] - 12;
            else if (fire_buffer[i][j] > 3)
                fire_buffer[i][j] = fire_buffer[i][j] - 4;
            else {
                if (fire_buffer[i][j] > 0) fire_buffer[i][j]--;
                if (fire_buffer[i][j] > 0) fire_buffer[i][j]--;
                if (fire_buffer[i][j] > 0) fire_buffer[i][j]--;
            }
        }
        adv+=4;
        if(adv>=screen_lines)  {
            if(y==0) {
#ifdef SHOW_FPS
                fire_paint(0,szfps);
#else
                fire_paint(0,nullptr);
#endif
            } else {
                fire_paint(y,nullptr);
            }
            y+=screen_lines;
            adv=0;
        }
    }
    for (j = 0; j < fire_buffer_width; j++) {
        if (rand() % 10 < 5) {
            delta = (rand() & 1) * 255;
        }
        fire_buffer[fire_buffer_height - 2][j] = delta;
        fire_buffer[fire_buffer_height - 1][j] = delta;
    }
    

    delta = 0;
    
    // The following keeps track of statistics
    static int frames = 0;
    static uint32_t fps_ts = 0;
    static uint32_t total_ms = 0;
    ++frames;

    if (ms >= fps_ts + 1000)
    {
        fps_ts = ms;
        
        if(frames==0) {
            snprintf(szfps, sizeof(szfps), "fps: < 1, total: %d ms",(int)total_ms);
        } else {
            snprintf(szfps, sizeof(szfps), "fps: %d, avg: %d ms", (int)frames,(int)total_ms/frames);
        }
    
        puts(szfps);
        frames = 0;
        total_ms = 0;
    }

    total_ms+=(millis()-ms);
}