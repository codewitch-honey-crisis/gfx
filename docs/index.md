##### honey the codewitch
# [GFX](https://honeythecodewitch.com/gfx)


GFX is a feature rich, cross platform graphics library geared for IoT devices. Unlike many graphics libraries, GFX is not tied intrinsically to the devices it can operate with - you can provide drivers for it, or [use one of the many available drivers for common devices.](https://github.com/codewitch-honey-crisis/gfx_libraries) GFX is equally suited for TFT/LCD, OLED and e-paper displays. Some notable features of GFX include Truetype font support, Win 3.1 FON support, JPG support, an X11 color palette, alpha blending, arbitrary binary pixel formats and color models, automatic conversion between them, and automatic color matching for paletted/indexed color displays.

GFX is copyright (C) 2021-2022 by honey the codewitch. GFX is licensed under the MIT license.

---
Get started using GFX by referencing one of the drivers for it from your PlatformIO project's INI file:

For C++17 (Arduino framework >= 2.0.3 - preferred)
```ini
[env:esp32-ILI9341]
platform = espressif32
board = node32s
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_deps = 
	codewitch-honey-crisis/htcw_ili9341
lib_ldf_mode = deep
build_unflags=-std=gnu++11
build_flags=-std=gnu++17
```

For C++14 (Arduino framework < 2.0.3)
```ini
[env:esp32-ILI9341]
platform = espressif32
board = node32s
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_deps = 
	codewitch-honey-crisis/htcw_ili9341
lib_ldf_mode = deep
build_unflags=-std=gnu++11
build_flags=-std=gnu++14
```
Above the last 4 entries starting at `lib_deps` are what include GFX and make it available, in this case by way of including the htcw_ili9341 driver library for GFX, which will also include GFX in your project. The build flags are to update C++ to GNU C++17 or GNU C++14 which is necessary for GFX to compile.

Finally include the appropriate header:
```cpp
#include <gfx.hpp> // for GCC C++17
```
or
```cpp
#include <gfx_cpp14.hpp> // for GCC C++14
```
and then access the `gfx` namespace, optionally importing it:
```cpp
using namespace gfx;
```
___

## Contents

1. [Pixels](pixels.md)
    - 1.1 [Declaring common pixels](pixels.md#1.1)
    - 1.2 [Channels](pixels.md#1.2)
    - 1.3 [Colors](pixels.md#1.3)
    - 1.4 [Color models](pixels.md#1.4)
    - 1.5 [The alpha channel](pixels.md#1.5)
    - 1.6 [Indexed pixels](pixels.md#1.6)
    - 1.7 [Declaring custom pixels](pixels.md#1.7)
    - 1.8 [Converting pixels](pixels.md#1.8)
    - 1.9 [Metadata](pixels.md#1.9)
2. [Draw targets](draw_targets.md)
    - 2.1 [Sources and destinations](draw_targets.md#2.1)
    - 2.2 [Bitmaps](draw_targets.md#2.2)
         - 2.2.1 [Standard bitmaps](draw_targets.md#2.2.1)
         - 2.2.2 [Const bitmaps](draw_targets.md#2.2.2)
         - 2.2.3 [Large bitmaps](draw_targets.md#2.2.3)
    - 2.3 [Drivers](draw_targets.md#2.3)
    - 2.4 [Viewports](draw_targets.md#2.4)
    - 2.5 [Sprites](draw_targets.md#2.5)
    - 2.6 [Custom](draw_targets.md#2.6)
        - 2.6.1 [Common members](draw_targets.md#2.6.1)
        - 2.6.2 [Capabilities](.draw_targets.md#2.6.2)
        - 2.6.3 [Draw source members](draw_targets.md#2.6.3)
        - 2.6.4 [Draw destination members](draw_targets.md#2.6.4)
        - 2.6.5 [Initialization](draw_targets.md#2.6.5)
3. [Images](images.md)
4. [Fonts](fonts.md)
    - 4.1 [Truetype/Opentype Vector](fonts.md#4.1)
      - 4.1.1 [Layout](fonts.md#4.1.1)
      - 4.1.2 [Storage considerations](fonts.md#4.1.2)
      - 4.1.3 [Performance considerations](fonts.md#4.1.3)
    - 4.2 [Win 3.1 Raster](fonts.md#4.2)
      - 4.2.1 [Layout](fonts.md#4.2.1)
      - 4.2.2 [Storage considerations](fonts.md#4.2.2)
      - 4.2.3 [Performance considerations](fonts.md#4.2.3)
5. [Drawing](drawing.md)
    - 5.1 [Basic drawing elements](drawing.md#5.1)
    - 5.2 [Bitmaps and draw Sources](drawing.md#5.2)
    - 5.3 [Text](drawing.md#5.3)
    - 5.4 [Images](drawing.md#5.4)
    - 5.5 [Sprites](drawing.md#5.5)
    - 5.6 [Icons](drawing.md#5.6)
    - 5.7 [Alpha blending](drawing.md#5.7)
      - 5.7.1 [Performance considerations](drawing.md#5.7.1)
      - 5.7.2 [Draw target considerations](drawing.md#5.7.2)
    - 5.8 [Suspend and resume](drawing.md#5.8)
    - 5.9 [Batching](drawing.md#5.9)
6. [Positioning](positioning.md)
    - 6.1 [Points](positioning.md#6.1)
    - 6.2 [Sizes](positioning.md#6.2)
    - 6.3 [Rectangles](positioning.md#6.3)
    - 6.4 [Paths](positioning.md#6.4)
7. [Streams](streams.md)
8. [Performance](performance.md)
   - 8.1 [Blting](performance.md#8.1)
   - 8.2 [Copying](performance.md#8.2)
   - 8.3 [Batching](performance.md#8.3)
   - 8.4 [RLE transmission](performance.md#8.4)
   - 8.5 [Asynchronous drawing](performance.md#8.5)
9. [Tools](tools.md)
   - 9.1 [Bingen](tools.md#9.1)
   - 9.2 [Fontgen](tools.md#9.2)
10. [Addendum](addendum.md)
    - 10.1 [Arduino TFT Bus](addendum.md#10.1)

[Pixels](pixels.md) →