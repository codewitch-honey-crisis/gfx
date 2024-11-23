//////////////////////////////////
// EXAMPLE
// Uses htcw_gfx with an esp32 to
// demonstrate the vector canvas, 
// and DMA enabled draws
//////////////////////////////////

// If defined the clock face is only drawn once on startup
// and then stored in a bitmap. It is copied over on
// every iteration, instead of being redrawn every time
#define BUFFER_CLOCK_FACE
// If defined, the FPS will be displayed on the screen
// (robs performance)
#define SHOW_FPS
// If defined, DMA will be used to improve performance
// with the use of two transfer buffers
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
#include <time.h>
// graphics library codewitch-honey-crisis/htcw_gfx
#include <gfx.hpp>
#ifdef SHOW_FPS
// include the windows 3.1 raster font
// converted to a header using
// https://honeythecodewitch.com/gfx/converter
// (C++ mode, no arduino)
#define VGA_8X8_IMPLEMENTATION
#include "assets/vga_8x8.h"
#endif // SHOW_FPS
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

// declare a bitmap type for our frame buffer type (RGB565 or rgb 16-bit color)
using fb_t = bitmap<rgb_pixel<16>>;
// get a color pseudo-enum for our bitmap type
using color_t = color<typename fb_t::pixel_type>;

// screen settings
constexpr static const typename fb_t::pixel_type screen_color = color_t::black;
// clock settings
constexpr static const  uint16_t face_border_width = 2;
constexpr static const  vector_pixel face_border_color = color<vector_pixel>::black;
constexpr static const  vector_pixel face_color = color<vector_pixel>::white;
constexpr static const  vector_pixel tick_border_color = color<vector_pixel>::gray;
constexpr static const  vector_pixel tick_color = color<vector_pixel>::gray;
constexpr static const  uint16_t tick_border_width = 2;
constexpr static const  vector_pixel minute_color = color<vector_pixel>::black;
constexpr static const  vector_pixel minute_border_color = color<vector_pixel>::gray;
constexpr static const  uint16_t minute_border_width = 2;
constexpr static const  vector_pixel hour_color = color<vector_pixel>::black;
constexpr static const  vector_pixel hour_border_color = color<vector_pixel>::gray;
constexpr static const  uint16_t hour_border_width = 2;
constexpr static const  vector_pixel second_color = color<vector_pixel>::red.opacity(.5);
constexpr static const  vector_pixel second_border_color = color<vector_pixel>::red;
constexpr static const  uint16_t second_border_width = 2;

// lcd data
// This works out to be 32KB - the max DMA transfer size
static const size_t lcd_transfer_buffer_size = (128*128*(LCD_BIT_DEPTH)+7)/8;
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

using color_t = color<typename fb_t::pixel_type>;
#ifdef SHOW_FPS
// prepare our font data by wrapping it with a stream
static const_buffer_stream fps_font_stm(vga_8x8,sizeof(vga_8x8));
// feed the stream to the font 
// note that win fonts are very fast
static win_font fps_font(fps_font_stm,0);
#endif
// compute thetas for a rotation
static void update_transform(float rotation, float& ctheta, float& stheta) {
    float rads = gfx::math::deg2rad(rotation); // rotation * (3.1415926536f / 180.0f);
    ctheta = cosf(rads);
    stheta = sinf(rads);
}
// transform a point given some thetas, a center and an offset
static pointf transform_point(float ctheta, float stheta, pointf center, pointf offset, float x, float y) {
    float rx = (ctheta * (x - (float)center.x) - stheta * (y - (float)center.y) + (float)center.x) + offset.x;
    float ry = (stheta * (x - (float)center.x) + ctheta * (y - (float)center.y) + (float)center.y) + offset.y;
    return {(float)rx, (float)ry};
}
// declare the canvas for our clock.
// the dimensions can be set any time before
// initialize() is called. Once initialized,
// the dimensions cannot be changed
static_assert(LCD_WIDTH>=128 && LCD_HEIGHT>=128);
static canvas clock_canvas({128,128});
// understanding the canvas is easy if you know SVG, since 
// all the commands have a rough SVG corollary.
gfx_result draw_clock_face() {
    
    constexpr static const float rot_step = 360.0f / 12.0f;
    pointf offset(0, 0);
    pointf center(0, 0);

    float rotation(0);
    float ctheta, stheta;
    ssize16 size = (ssize16)clock_canvas.dimensions();
    rectf b = gfx::sizef(size.width, size.height).bounds();
    b.inflate_inplace(-face_border_width - 1, -face_border_width - 1);
    float w = b.width();
    float h = b.height();
    if(w>h) w= h;
    rectf sr(0, w / 30, w / 30, w / 5);
    sr.center_horizontal_inplace(b);
    center = gfx::pointf(w * 0.5f + face_border_width + 1, w * 0.5f + face_border_width + 1);
    clock_canvas.fill_color(face_color);
    clock_canvas.stroke_color(face_border_color);
    clock_canvas.stroke_width(face_border_width);
    clock_canvas.circle(center, center.x - 1);
    gfx_result res = clock_canvas.render();
    if(res!=gfx_result::success) {
        return res;
    }
    bool toggle = false;
    clock_canvas.stroke_color(tick_border_color);
    clock_canvas.fill_color(tick_color);
    clock_canvas.stroke_width(tick_border_width);
    
    for (float rot = 0; rot < 360.0f; rot += rot_step) {
        rotation = rot;
        update_transform(rotation, ctheta, stheta);
        toggle = !toggle;
        if (toggle) {
            clock_canvas.move_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y1));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y1));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2));
            clock_canvas.close_path();
        } else {
            clock_canvas.move_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y1));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y1));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2 - sr.height() * 0.5f));
            clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2 - sr.height() * 0.5f));
            clock_canvas.close_path();
        }
        res=clock_canvas.render();
        if(res!=gfx_result::success) {
            return res;
        }
    }    
    return gfx_result::success;
}

// our current "time" used to position the clock hands
static time_t current_time = 0;

#ifdef BUFFER_CLOCK_FACE
// the bitmap used to hold the clock face
fb_t clock_face_bmp;
#endif
// the rectangle where the clock lives
srect16 clock_rect = ((srect16)clock_canvas.bounds()).center(ssize16(LCD_WIDTH,LCD_HEIGHT).bounds());

#ifdef ARDUINO
// entry point (arduino)
void setup() {
    Serial.begin(115200);
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
    printf("Before LCD init\nfree mem: %0.2fKB. Largest free block: %0.2fKB\n",((float)esp_get_free_heap_size())/1024.f,((float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))/1024.f);
    // initialize the LCD
    lcd_panel_init();
#ifdef SHOW_FPS
    // initialize our font
    fps_font.initialize();
#endif   
    uint16_t screen_lines = 0;
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
    // clear the screen to white
    size16 xfer_dim(LCD_WIDTH,screen_lines);
    
    fb_t(xfer_dim,lcd_transfer_buffer).fill(xfer_dim.bounds(),screen_color);
#ifdef USE_DMA
    fb_t(xfer_dim,lcd_transfer_buffer2).fill(xfer_dim.bounds(),screen_color);
#endif
    for(int y = 0;y<LCD_HEIGHT;y+=screen_lines) {
        lcd_begin_draw();
        lcd_flush_bitmap(0,y,LCD_WIDTH-1,y+screen_lines-1,lcd_current_buffer());
        lcd_switch_buffers();
    }
    // initialize the canvas
    clock_canvas.initialize();
    // get the style
    canvas_style clock_style = clock_canvas.style();
    // set some of the defaults to what we want
    // you can set these individually off of the canvas
    clock_style.fill_paint_type = paint_type::solid;
    clock_style.stroke_paint_type = paint_type::solid;
    clock_style.stroke_width = 1;
    clock_canvas.style(clock_style);
    printf("After init\nfree mem: %0.2fKB. Largest free block: %0.2fKB\n",((float)esp_get_free_heap_size())/1024.f,((float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))/1024.f);
#ifndef BUFFER_CLOCK_FACE 
    // clear the transfer buffers to the screen color
    fb_t(clock_canvas.dimensions(),lcd_transfer_buffer).fill(clock_canvas.bounds(),screen_color);
#ifdef USE_DMA
    fb_t(clock_canvas.dimensions(),lcd_transfer_buffer2).fill(clock_canvas.bounds(),screen_color);
#endif
#endif
    // ESP-IDF compat shim
#ifndef ARDUINO
    TaskHandle_t loop_handle;
    xTaskCreate(loop_task,"loop_task",4096,nullptr,20,&loop_handle);
#endif
}
void loop() {
    // timestamp for start
    uint32_t ms = millis();
#ifdef BUFFER_CLOCK_FACE
    static bool clock_face_init=false;
    if(!clock_face_init) {
        clock_face_init = true;
        // if indicated, we only draw the clock face once
        // to a buffer. We then copy that buffer over
        // each iteration instead of redrawing the face.
        // trades memory for speed.
        clock_face_bmp = create_bitmap<fb_t::pixel_type>(clock_canvas.dimensions());
        clock_face_bmp.fill(clock_face_bmp.bounds(),screen_color);

        // attach/bind the initialized canvas to the face bitmap
        draw::canvas(clock_face_bmp,clock_canvas);
        
        gfx_result res = draw_clock_face();
        if(res!=gfx_result::success) {
            puts("ERROR");
            printf("free mem: %0.2fKB. Largest free block: %0.2fKB\n",((float)esp_get_free_heap_size())/1024.f,((float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))/1024.f);
            puts("");
            while(1) vTaskDelay(5);
        }
    }
#endif
    lcd_begin_draw();
    // bind the canvas to the current transfer bitmap
    fb_t xfer_bmp(clock_canvas.dimensions(),lcd_current_buffer());
    draw::canvas(xfer_bmp,clock_canvas);
#ifdef BUFFER_CLOCK_FACE
    draw::bitmap(xfer_bmp,xfer_bmp.bounds(),clock_face_bmp,clock_face_bmp.bounds());
#else
    gfx_result res = draw_clock_face();
    if(res!=gfx_result::success) {
        puts("ERROR");
        printf("free mem: %0.2fKB. Largest free block: %0.2fKB\n",((float)esp_get_free_heap_size())/1024.f,((float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))/1024.f);
        puts("");
        
        while(1) vTaskDelay(5);
    }
#endif
    pointf offset(0, 0);
    pointf center(0, 0);
    time_t time = current_time;
    float rotation(0);
    float ctheta, stheta;
    ssize16 size = (ssize16)clock_canvas.dimensions();
    rectf b = gfx::sizef(size.width, size.height).bounds();
    b.inflate_inplace(-face_border_width - 1, -face_border_width - 1);
    float w = b.width();
    float h = b.height();
    if(w>h) w= h;
    rectf sr(0, w / 30, w / 30, w / 5);
    sr.center_horizontal_inplace(b);
    center = gfx::pointf(w * 0.5f + face_border_width + 1, w * 0.5f + face_border_width + 1);
    sr = gfx::rectf(0, w / 40, w / 16, w / 2);
    sr.center_horizontal_inplace(b);
    // create a path for the minute hand:
    rotation = (fmodf(time / 60.0f, 60) / 60.0f) * 360.0f;
    update_transform(rotation, ctheta, stheta);
    clock_canvas.move_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y1));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y2 + (w / 20)));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2));
    clock_canvas.close_path();
    clock_canvas.fill_color(minute_color);
    clock_canvas.stroke_color(minute_border_color);
    clock_canvas.stroke_width(minute_border_width);
    clock_canvas.render(); // render the path
    // create a path for the hour hand
    sr.y1 += w / 8;
    rotation = (fmodf(time / (3600.0f), 12.0f) / (12.0f)) * 360.0f;
    update_transform(rotation, ctheta, stheta);
    clock_canvas.move_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y1));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y2 + (w / 20)));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2));
    clock_canvas.close_path();
    clock_canvas.fill_color(hour_color);
    clock_canvas.stroke_color(hour_border_color);
    clock_canvas.stroke_width(hour_border_width);
    clock_canvas.render(); // render the path
    // create a path for the second hand
    sr.y1 -= w / 8;
    rotation = ((time % 60) / 60.0f) * 360.0f;
    update_transform(rotation, ctheta, stheta);
    clock_canvas.move_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y1));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x2, sr.y2));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1 + sr.width() * 0.5f, sr.y2 + (w / 20)));
    clock_canvas.line_to(transform_point(ctheta, stheta, center, offset, sr.x1, sr.y2));
    clock_canvas.close_path();
    clock_canvas.fill_color(second_color);
    clock_canvas.stroke_color(second_border_color);
    clock_canvas.stroke_width(second_border_width);
    clock_canvas.render();
    // increment the face "time"
    ++current_time;
    
    // send what we just drew to the display
    lcd_flush_bitmap(clock_rect.x1,clock_rect.y1,clock_rect.x2,clock_rect.y2,xfer_bmp.begin());   
    lcd_switch_buffers();
    
    // The following keeps track of statistics
    static int frames = 0;
    static char szfps[64] = {0}; // the string to hold the stats
    static uint32_t fps_ts = 0;
    static uint32_t total_ms = 0;
#ifdef SHOW_FPS
    static bool fps_init=false;
    static size16 fps_area;
    static rect16 fps_bounds;
    text_info fps_txt_info;
    if(fps_init) {
        // draw the stats to the screen
        fps_txt_info=text_info(szfps,fps_font);
        lcd_begin_draw();
        xfer_bmp = fb_t(fps_bounds.dimensions(),lcd_current_buffer());
        xfer_bmp.fill(xfer_bmp.bounds(),screen_color);
        draw::text(xfer_bmp,xfer_bmp.bounds(),fps_txt_info,color_t::red);
        rect16 r = xfer_bmp.bounds().offset(fps_bounds.x1,fps_bounds.y1);
        lcd_flush_bitmap(r.x1,r.y1,r.x2,r.y2,xfer_bmp.begin());
        lcd_switch_buffers();
    }
#endif
    ++frames;

    if (ms > fps_ts + 1000)
    {
#ifdef SHOW_FPS
        fps_font.measure(LCD_WIDTH,fps_txt_info,&fps_area);
        fps_bounds = rect16(LCD_WIDTH-fps_area.width,LCD_HEIGHT-fps_area.height,LCD_WIDTH-1,LCD_HEIGHT-1);
#endif
        fps_ts = ms;
        
        if(frames==0) {
            snprintf(szfps, sizeof(szfps), "fps: < 1\ntotal: %d ms",(int)total_ms);
        } else {
            snprintf(szfps, sizeof(szfps), "fps: %d\navg: %d ms", (int)frames,(int)total_ms/frames);
        }
        puts(szfps);
        printf("free mem: %0.2fKB. Largest free block: %0.2fKB\n",((float)esp_get_free_heap_size())/1024.f,((float)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))/1024.f);
        frames = 0;
        total_ms = 0;
#ifdef SHOW_FPS
        fps_init = true;
#endif
    }

    total_ms+=(millis()-ms);   
}