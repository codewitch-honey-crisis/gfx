#### [← Back to index](index.md)

<a name="10"></a>

# 10. Addendum

This document contains information that doesn't fit elsewhere or isn't directly related to the core GFX library, but is nevertheless essential.

<a name="10.1"></a>

## 10.1 Arduino TFT Bus

The Arduino drivers typically use the TFT I/O bus framework [htcw_tft_io](https://github.com/codewitch-honey-crisis/htcw_tft_io)

This bus framework currently supports SPI, 8-bit parallel and I2C. It is separate from the drivers, that way the same driver can be used over different kinds of busses. For example, the ILI9341 comes in SPI and parallel varieties. With this framework, you use the same ILI9341 driver, but connect it to the appropriate bus to fit your situation and hardware.

On supported hardware, like the ESP32 the SPI bus supports assignable pins and DMA.

To access the assignable pins for SPI you use `tft_spi_ex<>`. Otherwise you may use `tft_spi<>` to use the default or fixed pins. Again, not every platform has the assignable pin option.

On supported hardware, like the ESP32 the I2C bus supports assignable pins.

To access the assignable pins for I2C you use `tft_i2c_ex<>`. Otherwise you may use `tft_i2c<>` to use the default or fixed pins. Once again, not every platform has the assignable pin option.

The 8-bit parallel bus's pins must always be specified. To specify a parallel bus use `tft_p8<>`.

While each driver that uses it includes the bus framework for you, it is possible to include the bus framework by referencing `codewitch-honey-crisis/htcw_tft_io` in your platformio.ini. Either way, you can then use the following:

```cpp
#include <tft_io.hpp>
using namespace arduino;
```

Here's an example of declaring the default SPI bus:

```cpp
// LCD_HOST is the host # (often VSPI on the ESP32)
// PIN_NUM_CS is the CS line
using bus_type = tft_spi<LCD_HOST,PIN_NUM_CS>;
```

Here's a more advanced declaration on the ESP32:

```cpp
using bus_type = tft_spi_ex<LCD_HOST,
                            PIN_NUM_CS,
                            PIN_NUM_MOSI,
                            PIN_NUM_MISO,
                            PIN_NUM_CLK,
                            SPI_MODE0,
// some devices have no SDA read capability, so no read whatsoever.
#if defined(LCD_EPAPER) || defined(SSD1306) || defined(SSD1351) 
                            false
#else
                            PIN_NUM_MISO<0
#endif
#ifdef OPTIMIZE_DMA
                            ,(LCD_WIDTH*LCD_HEIGHT)*2+8
#endif
>;
```
This bears some explanation. First, you'll note pin assignments. After that, we explicity set the SPI Mode, because we're going to set the SDA Read.

SDA Read is a special mode that allows displays without a MISO line to be read from over the MOSI line. This defaults to *on* if no MISO line is specified, but some screens do not support reading at all. For those you should explicitly turn it off.

`OPTIMIZE_DMA` is a special define that indicates the presence of DMA capabilities on this platform. If it is available, you must compute the maximum size of the DMA buffer. Here we compute it based on the size of the frame buffer in a 16bpp display.

The parallel bus is declared similarly to `tft_spi_ex<>` sans DMA and SDA Read features, and adding a lot more pins:

```cpp
using bus_type = tft_p8<PIN_NUM_CS,
                        PIN_NUM_WR,
                        PIN_NUM_RD,
                        PIN_NUM_D0,
                        PIN_NUM_D1,
                        PIN_NUM_D2,
                        PIN_NUM_D3,
                        PIN_NUM_D4,
                        PIN_NUM_D5,
                        PIN_NUM_D6,
                        PIN_NUM_D7>;
```
It currently only works on the ESP32.

I2C is pretty straightforward. It is similar to SPI the way it is declared, having one with assignable pins (`tft_i2c_ex`) and one without (`tft_i2c`):

```cpp
// LCD_PORT is the I2C port number to use.
// On the ESP32 this will be 0 or 1.
using bus_type = tft_i2c_ex<LCD_PORT,
                            PIN_NUM_SDA,
                            PIN_NUM_SCL>;
```
Once you have a bus, you may pass it to a driver.

```cpp
// lib_deps = codewitch-honey-crisis/htcw_ili9341
#include <ili9341.hpp>
...
using lcd_type = ili9341<PIN_NUM_DC,
                        PIN_NUM_RST,
                        PIN_NUM_BKL,
                        bus_type,
                        LCD_ROTATION,
                        LCD_BKL_HIGH,
                        LCD_WRITE_SPEED_PERCENT,
                        LCD_READ_SPEED_PERCENT>;
```

Every driver's declaration will be somewhat different so we won't be going over the declaration in detail here, but the common thread is `bus_type` which the driver takes to establish a connection to the bus you declared. In this case, you'd use a parallel bus or SPI bus depending on the type of ILI9341 module you have. Modules containing the SSD1306 support the I2C bus or the SPI bus.

If you need to declare multiple SPI or I2C TFT devices you can declare more than one bus on the same SPI host or I2C port. You simply declare another bus, without the pin assignments. It will use the same pins as the first instance. It's up to you to make sure you initialize the first bus declaration first, usually by initializing the driver attached to it.

If you need access to the `SPIClass` instance or the `Wire` instance for your bus in order to share the bus, you can use `spi_container<SPI_HOST>::instance()` where `SPI_HOST` is the host - often `VSPI` on the ESP32. For I2C you use `i2c_container<I2C_PORT>::instance()`.


[← Tools](tools.md)

