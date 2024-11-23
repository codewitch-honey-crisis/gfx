//////////////////////////////////
// EXAMPLE
// Uses htcw_gfx with an esp32
// to demonstrate rendering SVGs,
// advanced canvas features, and
// DMA enabled draws
//////////////////////////////////

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
#define SVG_SVG_IMPLEMENTATION
#include "assets/svg_svg.h"
#define BUNGEE_IMPLEMENTATION
#include "assets/bungee.h"

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

// If indicated, use spans to manipulate the bitmap
// Spans can be faster in many circumstances than
// using point() and fill() methods because they
// give you pointer access to the bitmap data
// for direct manipulation:
#define USE_SPANS

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
static uint8_t *lcd_transfer_buffer2 = nullptr;
// 0 = no flushes in progress, otherwise flushing
static volatile int lcd_flushing = 0;
static esp_lcd_panel_handle_t lcd_handle = nullptr;
static int lcd_dma_sel = 0;
static uint8_t *lcd_current_buffer() {
    return lcd_dma_sel ? lcd_transfer_buffer2 : lcd_transfer_buffer;
}
static void lcd_switch_buffers() {
    lcd_dma_sel++;
    if (lcd_dma_sel > 1) {
        lcd_dma_sel = 0;
    }
}
// indicates the LCD DMA transfer is complete
static bool lcd_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                            esp_lcd_panel_io_event_data_t *edata,
                            void *user_ctx) {
    lcd_flushing = 0;
    return true;
}
// waits for there to be a free buffer
static void lcd_wait_dma() { while (lcd_flushing > 1); }
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
    // initialize the transfer buffers
    lcd_transfer_buffer2 = (uint8_t *)malloc(lcd_transfer_buffer_size);
    if (lcd_transfer_buffer2 == nullptr) {
        puts("Out of memory initializing transfer buffer 2");
        while (1) vTaskDelay(5);
    }
    memset(lcd_transfer_buffer2, 0, lcd_transfer_buffer_size);
}

using color_t = color<typename fb_t::pixel_type>;

#ifdef ARDUINO
// entry point (arduino)
void setup() {
    Serial.begin(115200);
#else
// Arduino compat shim
static void* ps_malloc(size_t size) {
    return heap_caps_malloc(size,MALLOC_CAP_SPIRAM);
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
    // declare a canvas for the screen
    // the dimensions can be set any time before
    // initialize() is called. Once initialized,
    // the dimensions cannot be changed
    canvas scr_canvas({LCD_WIDTH,LCD_HEIGHT});
    // the transfer bitmaps
    fb_t xfr_bmp;
    
    // the bitmap used to hold the texture
    fb_t txt_bmp;
    
    // the offscreen bitmap used to hold the canvas drawing
    fb_t scr_bmp;

    // create our offscreen bitmap
    scr_bmp = create_bitmap<decltype(scr_bmp)::pixel_type>({LCD_WIDTH,LCD_HEIGHT});
    // if we run out of memory, try PSRAM:
    if(scr_bmp.begin()==nullptr) {
        scr_bmp = create_bitmap<decltype(scr_bmp)::pixel_type>({LCD_WIDTH,LCD_HEIGHT},::ps_malloc);
        if(scr_bmp.begin()==nullptr) {
            puts("This program requires PSRAM for a screen of this size");
            while(1) vTaskDelay(5);
        }
    }
    // prepare the svg data array into a stream
    const_buffer_stream svg_stm(svg_svg,sizeof(svg_svg));
    
    // get the dimensions of the document
    sizef svg_dim;
    canvas::svg_dimensions(svg_stm,&svg_dim);
    // create the texture bitmap
    txt_bmp = create_bitmap<decltype(txt_bmp)::pixel_type >(size16(32,32*svg_dim.aspect_ratio()));
    if(txt_bmp.begin()==nullptr) {
        puts("Out of memory initializing texture bitmap");
        while(1) vTaskDelay(5);
    }
    // fill the offscreen bitmap with hot pink.
    scr_bmp.fill(scr_bmp.bounds(),color_t::hot_pink);
    
    // initialize the transfer bitmap
    xfr_bmp=fb_t({LCD_WIDTH,screen_lines},lcd_transfer_buffer);
    // fill the transfer bitmap with yellow.
    // here we're using the color enum for the same 
    // pixel type as decalared on txt_bmp
    txt_bmp.fill(txt_bmp.bounds(),color<decltype(txt_bmp)::pixel_type>::yellow);
    xfr_bmp=fb_t({LCD_WIDTH,screen_lines},lcd_transfer_buffer2);
    txt_bmp.fill(txt_bmp.bounds(),color<decltype(txt_bmp)::pixel_type>::yellow);
    // create a canvas to render to the texture
    canvas txt_canvas(txt_bmp.dimensions());
    txt_canvas.initialize();
    // link the canvas to the texture bitmap
    draw::canvas(txt_bmp,txt_canvas);
    // scale the document to the dimensions of the texture
    // respecting the aspect ratio
    matrix xfrm = matrix::create_fit_to(svg_dim,
        rectf(pointf::zero(),
            sizef(txt_bmp.dimensions().width,
                txt_bmp.dimensions().height)));
    // render the document using the transform we
    // just created
    txt_canvas.render_svg(svg_stm,xfrm);
    // we don't need this canvas anymore. free it up
    txt_canvas.deinitialize();
    // initialize the screen canvas
    scr_canvas.initialize();
    // link it to the offscreen bitmap
    draw::canvas(scr_bmp,scr_canvas);
    // prepare the ttf array into a stream
    const_buffer_stream ttf_stm(bungee,sizeof(bungee));
    // create some text information
    canvas_text_info ti;
    ti.ttf_font = &ttf_stm;
    ti.ttf_font_face = 0;
    ti.font_size = LCD_HEIGHT/2;
    ti.text_sz("SVG!");
    // we don't like rotating bitmaps. it looks ugly. 
    // if we want to rotate them, we should have done 
    // it during the render on txt_canvas above
    // this counters the rotation we give to the font
    // leading to an identity transform (untrasnformed)
    // texture:
    xfrm = matrix::create_rotate(math::deg2rad(15));
    texture txt;
    // create the texture from txt_bmp
    texture::create_from(txt_bmp,&txt);
    txt.transform = xfrm;
    // we want to tile it
    txt.type = texture_type::tiled;
    // rotate the text counter clockwise 15 degrees
    xfrm = matrix::create_rotate(math::deg2rad(-15));
    scr_canvas.transform(xfrm);
    // set the texture used for the fill operation
    scr_canvas.fill_texture(txt);
    // create a path for the text at the specified location
    scr_canvas.text({-(LCD_WIDTH/11),LCD_HEIGHT-(LCD_HEIGHT/6)},ti);
    // render it
    scr_canvas.render();
    // transfer our offscreen bitmap to the display.
    for(int y = 0;y<LCD_HEIGHT;y+=screen_lines) {
        lcd_begin_draw();
        xfr_bmp = fb_t({LCD_WIDTH,screen_lines},lcd_current_buffer());
        draw::bitmap(xfr_bmp,xfr_bmp.bounds(),scr_bmp,rect16(0,y,LCD_WIDTH-1,y+screen_lines-1));
        lcd_flush_bitmap(0,y,LCD_WIDTH-1,y+screen_lines-1,xfr_bmp.begin());
        lcd_switch_buffers();
    }
}
void loop() {

}