#### [← Back to index](index.md)

<a name="4"></a>

# 4. Fonts

Fonts can be used to render textual information to a draw destination. Fonts in GFX come in two major varieties: Truetype/Opentype (.ttf/.otf) and Win 3.1 Raster (.fon). They may be streamed from files (only Truetype/Opentype), loaded to RAM from a file, or another stream or embedded in program flash as a header file. The different types of fonts have different characteristics and requirements so they operate a bit differently from one another, even though they are similar.

<a name="4.1"></a>

## 4.1 Truetype/Opentype Vector

Truetype/Opentype fonts are represented by the `open_font` class. Rather than operating from RAM, these fonts are often streamed from storage due to their size. They only use memory temporarily during the rendering process. Unlike raster, these fonts produce crisp, clean edges and can be scaled to any size. Unlike raster, they require a lot of resources to render. Furthermore they support Unicode.

<a name="4.1.1"></a>

### 4.1.1 Layout

Truetype can be difficult to lay out due to the nature of vector fonts, and further complicated by the sometimes low quality of font files you can find for free online which sometimes have bad metrics in them. It can often require some fiddling and tweaking to get it just right. 

But first, you need to scale down the font, because at their native scale, the fonts are very large.

You can use the font's `float scale(float pixel_height) const` to find the scale for a font to be displayed at the given height in pixels:

```cpp
// make characters 40 pixels high
float scale = myfont.scale(40);
```

You can then use the `open_font`'s `measure_text()` to determine the area needed to display the text:

```cpp
// returns the size needed to display the text
ssize16 measure_text(
    // the total size of the potential
    // layout area
    ssize16 dest_size,
    // an offset for the font into 
    // destination area
    spoint16 offset,
    // the text to measure
    const char* text,
    // the scale of font to use
    float scale,
    // the tab width (scaled)
    float scaled_tab_width=0.0,
    // the text encoding
    gfx_encoding encoding=gfx_encoding::utf8,
    // a cache object used
    // to speed character
    // lookups
    open_font_cache* cache = nullptr) const;
```
You use it like this:
```cpp
ssize16 text_size = myfont.measure_text(
    (ssize16)screen.dimensions(),
    {0,0},
    "hello world!",
    scale);
```
Sometimes, the measurement will not be exact at larger sizes, and the final size must be tweaked by hand slightly.

Also note the offset `spoint16`. Vector fonts can overhang outside of their bounding area. Because of this, it can be useful to offset the start of the font draw by a certain amount in order to allow space for the overhang.

Later we'll cover drawing text using these fonts with `draw`.

<a name="4.1.2"></a>

### 4.1.2 Storage considerations

It's easiest to load a truetype font from something like SPIFFS (ESP32) or an SD card. However, doing so comes with a significant performance penalty.

Another option is to embed the font as a header generated using fontgen ([section 9.2](tools.md#9.2)) and then simply reference that.

Finally, for devices with enough RAM, you can read the entire font stream into RAM and then work off of that, but these font files are typically at least 250kB.

Read a font from a (file) stream like this:
```cpp
// the following line varies
// depending on platform:
file_stream fs("/spiffs/Maziro.ttf");
open_font maziro;
// should check the error result here:
open_font::open(&fs,&maziro);
// maziro now contains the font
```
Embed a font in a header using fontgen and the use it like this:
```cpp
#include "Maziro.h"
...
const open_font& maziro = Maziro_ttf;
// maziro now contains the font
```
Storing in RAM is the same as reading from a file except you use a buffer_stream instead of a file_stream, and point it to the TTF you loaded from a file into memory.

<a name="4.1.3"></a>

### 4.1.3 Performance considerations

Storage primarily dictates performance. Working from something like SPIFFS or SD will be pretty slow. Working from an embedded header is much faster. Working from a stream loaded into extended RAM such as PSRAM can improve performance dramatically on systems that can handle it, such as an ESP32 WROVER.

Rendering TrueType on IoT ties up a lot of resources. You should use this text sparingly, animating as infrequently as possible. You should ideally use this on draw destinations that support alpha-blending/are also draw sources or use a non-transparent background in order to facilitate anti-aliasing. You can use a font cache to speed up glyph lookups, although it will only help in situations with slow storage.

Declare a cache like this:
```cpp
open_font_cache fcache;
```
You can then pass it to `draw::text<>()` and `open_font`'s `measure_text()` in order to speed up glyph lookups. You can use `open_font`'s `cache()` method to preload the cache with a string. Use the cache's `clear()` method to reclaim the memory used by the cache and erase its contents. The cache is cleared when it goes out of scope via RAII.

<a name="4.2"></a>

## 4.2 Win 3.1 Raster

Windows 3.1 used 16-bit raster .fon files. So does GFX. Using them is a little simpler than Truetype/Opentype vector fonts and quite a bit faster. However, they lack features like scaling, anti-aliasing and Unicode support. That said, raster fonts are a staple of IoT. 

<a name="4.2.1"></a>

### 4.2.1 Layout

It's pretty easy to lay out a raster font. Once you have a font open you can use `measure_text()` to compute the dimensions of some text:
```cpp
// measures the size of the text 
// within the destination size
ssize16 measure_text(
    // the total size of the layout 
    // area
    ssize16 dest_size,
    // the text to measure
    const char* text,
    // the width of a tab
    // in characters
    unsigned int tab_width=4) const;
```
This function works similarly to the `open_font` function with the same name in that it gives you a `ssize16` structure that contains the width and height of the final text area. You use it like this:
```
ssize16 text_size = myfont.measure_text(screen.dimensions(), text);
```

<a name="4.2.2"></a>

### 4.2.2 Storage considerations

Raster fonts either reside in RAM or program space. You can load .FON files from a file via SPIFFS (ESP32) or an SD library in which case they will be kept in a RAM buffer, or you can embed them as a header in which case they remain in program flash space. Generally raster files should be embedded as a header on account of nobody actually using .FON files for modern content that's likely to appear on something such as an SD card. That being said, if you ever need it, it's not a problem.

Read a font from a (file) stream like this:
```cpp
// the following line varies
// depending on platform:
file_stream fs("/spiffs/Bm437_Acer_VGA_8x8.FON");
font acer8x8;
// should check the error result here:
font::read(&fs,&acer8x8);
// acer8x8 now contains the font
```
Embed a font in a header using fontgen and the use it like this:
```
#include "Bm437_Acer_VGA_8x8.h"
...
const font& acer8x8 = Bm437_Acer_VGA_8x8_FON;
// acer8x8 now contains the font
```

<a name="4.2.3"></a>

### 4.2.3 Performance considerations

Raster fonts are generally much faster than TrueType fonts and faster still when loaded into RAM versus being included in a header. They are also more quickly drawn when they do not have a transparent background.

[→ Drawing](drawing.md)

[← Images](images.md)

