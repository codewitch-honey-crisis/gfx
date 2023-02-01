#### [← Back to index](index.md)

<a name="5"></a>

# 5. Drawing

The `draw` class facilitates all high level drawing operations through a series of static template methods. It is the go to class for doing most of the significant things you can do with GFX, and mastering it is a key component of mastering GFX. The draw class primarily serves to manipulate draw targets on your behalf by doing this such as drawing lines and bitmaps on them.

All `draw` methods take a draw destination as their first parameter, and typically a positioning element within that destination (such as a rect or a path) as the second parameter. The parameters that follow depend on the operation. While the methods are template methods, all template arguments can be inferred from the method arguments so using `<...>` shouldn't be necessary. All drawing operations also take a pointer to an optional clipping rectangle that can be used to crop the draw to the indicated rectangular window. 

The `draw` class takes signed coordinates for its destination coordinates and unsigned coordinates for its source coordinates. The reason is so destinations can be drawn partially off the left and top of the screen, while it doesn't make sense to copy from a source outside of its bounds. That is the reason for the seeming inconsistency. It is by design.

When drawing, the colors and draw sources used can be specified using nearly any pixel format, with some restrictions applying to indexed pixels. You can even pass in colors that have an alpha channel to facilitate alpha blending. If you pass a color into a grayscale device, it will be converted to grayscale. Color models are also converted. Indexed colors are matched from their nearest equivelent.

<a name="5.1"></a>

## 5.1 Basic drawing elements

Basic drawing elements include points and lines as well as rectangles, elipses, arcs, rounded rectangles and polygons plus filled versions of the same. These are again, all accessed as static methods off the draw class.

```cpp
// draws a point at the specified 
// location and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result point(Destination& destination, 
        spoint16 location,
        PixelType color,
        const srect16* clip=nullptr);

// draws a line with the specified 
// start and end point and of the 
// specified color, with an optional 
// clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result line(Destination& destination, 
        const srect16& rect, 
        PixelType color,
        const srect16* clip=nullptr);

// draws a rectangle with the specified
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result rectangle(Destination& destination, 
        const srect16& rect,
        PixelType color,
        const srect16* clip=nullptr);

// draws a filled rectangle with the specified
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result filled_rectangle(Destination& destination, 
        const srect16& rect,
        PixelType color,
        const srect16* clip=nullptr);

// draws an ellipse with the specified
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result ellipse(Destination& destination,
        const srect16& rect, 
        PixelType color,
        const srect16* clip=nullptr);

// draws a filled ellipse with the specified
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result filled_ellipse(Destination& destination,
        const srect16& rect,
        PixelType color,
        const srect16* clip=nullptr);

// draws an arc with the specified 
// bounds and of the specified color, 
// with an optional clipping rectangle
// the orientation of the rectangle
// determines that of the arc.
template<typename Destination,typename PixelType>
static gfx_result arc(Destination& destination,
        const srect16& rect,
        PixelType color,
        const srect16* clip=nullptr);

// draws a filled arc with the specified 
// bounds and of the specified color, 
// with an optional clipping rectangle
// the orientation of the rectangle
// determines that of the arc.
template<typename Destination,typename PixelType>
static gfx_result filled_arc(Destination& destination,
        const srect16& rect,
        PixelType color,
        const srect16* clip=nullptr);

// draws a rounded rectangle with the specified
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result rounded_rectangle(Destination& destination,
        const srect16& rect,
        float ratio, 
        PixelType color,
        const srect16* clip=nullptr);

// draws a filled rounded rectangle with the specified 
// bounds and of the specified color, 
// with an optional clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result filled_rounded_rectangle(Destination& destination,
        const srect16& rect,
        const float ratio, 
        PixelType color,
        const srect16* clip=nullptr);

// draws a polygon with the specified
// path and color, with an optional 
// clipping rectangle
template<typename Destination,typename PixelType>
static gfx_result polygon(Destination& destination,
        const spath16& path, 
        PixelType color,
        const srect16* clip=nullptr);

// draws a filled polygon with the specified
// path and color, with an optional clipping
// rectangle
template<typename Destination,typename PixelType>
static gfx_result filled_polygon(Destination& destination,
        const spath16& path, 
        PixelType color,
        const srect16* clip=nullptr);
```
There are also `_async` versions of these methods, although their use is not recommended in most scenarios.

Here's an example of drawing some basic shapes:

```cpp
rgb_pixel<24> px;
spoint16 tpa[3];
const uint16_t sw =
    min(lcd.dimensions().width, lcd.dimensions().height) / 4;
for (int i = 0; i < 30; ++i) {
    px.channel<channel_name::R>((rand() % 256));
    px.channel<channel_name::G>((rand() % 256));
    px.channel<channel_name::B>((rand() % 256));
    srect16 sr(0, 0, rand() % sw + sw, rand() % sw + sw);
    sr.offset_inplace(rand() % (lcd.dimensions().width - sr.width()),
                        rand() % (lcd.dimensions().height - sr.height()));
    switch (rand() % 4) {
        case 0:
            draw::filled_rectangle(lcd, sr, px);
            break;
        case 1:
            draw::filled_rounded_rectangle(lcd, sr, .1, px);
            break;
        case 2:
            draw::filled_ellipse(lcd, sr, px);
            break;
        case 3:
            // draw a triangle
            tpa[0] = {int16_t(((sr.x2 - sr.x1) / 2) + sr.x1), sr.y1};
            tpa[1] = {sr.x2, sr.y2};
            tpa[2] = {sr.x1, sr.y2};
            spath16 path(3, tpa);
            draw::filled_polygon(lcd, path, px);
            break;
    }
}
```

<a name="5.2"></a>

## 5.2 Bitmaps and draw sources

The `draw::bitmap<>()` function takes all or a portion of a draw source and draws it to the destination, optionally flipping it horizontally or vertically, resizing it, clipping it, and perhaps converting the pixel formats or even alpha blending it, all depending on the arguments, the format of the source type and the format of the destination type.

That's quite a bit to keep track of, but you don't have to. The bottom line is GFX is written so you can *just draw* and it takes care of the complicated details like converting pixel formats, alpha blending, and choosing the most efficient way possible to transfer pixels.

All of this is handled by the aforementioned method:
```cpp
// draws a bitmap from the specified rectangle
// within a source to a location within a destination
// optionally resizing, flipping, drawing with a 
// transparent color (maskless sprite), and/or an
// optional clipping rectangle
template<typename Destination,typename Source>
static inline gfx_result bitmap(
        // the destination to draw to
        Destination& destination,
        // the destination rectangle to draw into
        const srect16& dest_rect,
        // the source to consume pixels from
        Source& source,
        // the source rectangle to fetch from
        const rect16& source_rect,
        // the resizing/cropping type
        bitmap_resize resize_type=bitmap_resize::crop,
        // a color that will not be drawn, if specified
        const typename Source::pixel_type* transparent_color=nullptr, 
        // a rectangle to use that will clip the 
        // draw within the destination
        const srect16* clip=nullptr)
```
There is also an `_async` method for this, and unlike the other async methods this one is more generally useful. While it has the same signature, when the destination and source support it, an asynchronous transfer will be started and control will be immediately returned from the method. During this operation the pixel buffer for the draw source should not be written to or discarded. Attempts to write to the destination will block until the asynchronous operation completes. If you must write to it again use `draw::wait_all_async(destination)` to wait for the operation to complete first. This is a very useful way to increase framerates if your bitmaps are large enough to overcome the extra overhead of transferring asynchronously. Basically while you're transfering the existing bitmap, you can draw to a new bitmap, then transfer that one. While you're transfering from that one, you draw to the first one, and then you repeat the process.

Here's an example of declaring a bitmap and a fixed buffer:

```cpp
constexpr static const size16 bmp_size(16, 16);
using bmp_type = bitmap<decltype(lcd)::pixel_type>;
using bmp_color = color<typename bmp_type::pixel_type>;
uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
bmp_type bmp(bmp_size, bmp_buf);
```

Now you can draw to it using `draw::xxxx(bmp,...)`

Finally, here's how you draw it to the center of the screen:

```cpp
draw::bitmap(lcd,bmp.bounds().center(lcd.bounds()),bmp,bmp.bounds());
```

<a name="5.3"></a>

## 5.3 Text

Text is drawn using one of the `draw::text<>()` overloads depending on which type of font is being used. This will draw text to a given area, wrapping as necessary, although word breaking is not performed.

For `open_font` vector fonts use this overload:
```cpp
template<typename Destination,typename PixelType>
static gfx_result text(
    // the destination to draw to
    Destination& destination,
    // the destination text area
    const srect16& dest_rect,
    // an offset into the text area
    spoint16 offset,
    // the text
    const char* text,
    // the font
    const open_font& font,
    // the scale, from font.scale(height)
    float scale,
    // the forecolor
    PixelType color,
    // the backcolor, if used
    PixelType backcolor=
        // black
        convert<rgb_pixel<3>,PixelType>(
            rgb_pixel<3>(0,0,0)),
    // indicates whether or not the 
    // backcolor is used. if true, it 
    // is not used.
    bool transparent_background = true,
    // make true to disable antialiasing
    // which can be useful for certain
    // displays such as those that do
    // not have grayscale or otherwise
    // have limited colors.
    bool no_antialiasing = false,
    // the tab width to scale
    float scaled_tab_width=0,
    // the encoding of the text
    gfx_encoding encoding=gfx_encoding::utf8,
    // an optional clipping rectangle
    srect16* clip=nullptr,
    // an optional font cache to speed up the draw
    open_font_cache* cache = nullptr)
```
or
```cpp
template<typename Destination,typename PixelType>
static gfx_result text(
    // the destination to draw to
    Destination& destination,
    // the destination text area
    const srect16& dest_rect,
    // the text info struct
    const open_text_info& info,
    // the forecolor
    PixelType color,
    // the backcolor, if used
    PixelType backcolor=
        // black
        convert<rgb_pixel<3>,PixelType>(
            rgb_pixel<3>(0,0,0)),
    // an optional clipping rectangle
    srect16* clip=nullptr)
```
For `font` raster fonts use this overload:
```cpp
template<typename Destination,typename PixelType>
static gfx_result text(
    // the destination to draw to
    Destination& destination,
    // the destination text area
    const srect16& dest_rect,
    // the text
    const char* text,
    // the font
    const font& font,
    // the forecolor
    PixelType color,
    // the backcolor
    PixelType backcolor=
        // black
        convert<rgb_pixel<3>,PixelType>(
            rgb_pixel<3>(0,0,0)),
    // indicates whether or not backcolor
    // is used. If true it is not used.
    // If false, draws are faster.
    bool transparent_background = true,
    // The tab width, in characters
    unsigned int tab_width=4,
    // the optional clipping
    // rectangle
    srect16* clip=nullptr)
```
or
```cpp
template<typename Destination,typename PixelType>
static gfx_result text(
    // the destination to draw to
    Destination& destination,
    // the destination text area
    const srect16& dest_rect,
    const text_info& info,
    // the forecolor
    PixelType color,
    // the backcolor
    PixelType backcolor=
        // black
        convert<rgb_pixel<3>,PixelType>(
            rgb_pixel<3>(0,0,0)),
    // the optional clipping
    // rectangle
    srect16* clip=nullptr)
```
There are also `_async` versions of these methods.

Here's an example of using raster, which assumes you've included a font header:

```cpp
// the font header contains Bm437_ATI_9x16_FON
const font &f = Bm437_ATI_9x16_FON;
const char *text = "(C) 2022\r\nby HTCW";
ssize16 text_size = f.measure_text((ssize16)lcd.dimensions(), text);
srect16 text_rect =
    srect16(spoint16((lcd.dimensions().width - text_size.width) / 2,
                        (lcd.dimensions().height - text_size.height) / 2),
            text_size);
draw::text(lcd, text_rect, text, f, lcd_color::old_lace,
                lcd_color::black, false);
```

Here's an example of using vector/TrueType:

```cpp
// The font header contains Maziro_ttf
const open_font &f = Maziro_ttf;
const char *text = "GFX";
int height = min(lcd.dimensions().width, lcd.dimensions().height) / 2;
const float scale = f.scale(height);
srect16 text_rect =
    srect16(spoint16::zero(),
            f.measure_text(ssize16::max(), spoint16::zero(), text, scale));
draw::text(lcd, 
            text_rect.center((srect16)lcd.bounds()), 
            spoint16::zero(), 
            text, 
            f,
            scale, 
            lcd_color::blue);
```
<a name="5.4"></a>

## 5.4 Images

As mentioned in [section 3](images.md), images are not draw sources because they are loaded incrementally and cannot provide random access access to their pixel data. Due to that, there is a `draw::image<>()` method for images that works in lieu of `draw::bitmap<>()`. Rather than take a draw source like `draw::bitmap<>()`, `draw::image<>()` takes a `stream` as its input pixel source. The stream must be in JPG format.
```cpp
template<typename Destination>
static gfx_result image(
    // the destination to draw to
    Destination& destination, 
    // the rectangle to draw to
    // within destination
    const srect16& dest_rect, 
    // the source stream in 
    // JPG format
    stream* src_stream, 
    // the source rect to draw from
    // (can be bigger than the source)
    const rect16& src_rect={0,0,65535,65535},
    // the resize/crop mode. Only crop
    // is supported currently
    bitmap_resize resize_type=bitmap_resize::crop,
    // the optional clipping
    // rectangle
    srect16* clip=nullptr)
```
There is also an `_async` version of this method. Unlike drawing a bitmap, this async method will be mostly synchronous, due to file stream limitations (no way to pipeline a file to an SPI bus over DMA) and the decompression process itself.

See [section 3](images.md) for example code.

<a name="5.5"></a>

## 5.5 Sprites

Sprites are useful for animating simple non-rectangular figures. In addition to a bitmap representing the actual figure, a sprite also has a monochrome mask indicating which pixels in the bitmap rectangle are actually a visible part of the figure and which parts are not drawn. By creating a mask in the shape of your figure you can animate it as though it was non-rectangular. Besides being able to be drawn, sprites also have a `bool hit_test(point16 location) const` method which can determine if the given location within the sprite is part of the mask or not. Once you create a sprite as in [section 2.5](draw_targets.md#2.5) you can draw it to the screen:
```cpp
static gfx_result sprite(
    // the destination to draw to
    Destination& destination, 
    // the location within the
    // destination to draw to
    spoint16 location,
    // the sprite to draw
    const Sprite& sprite,
    // an optional clipping
    // rectangle
    srect16* clip=nullptr)
```
There is also an `_async` version of this method, thought it is mostly synchronous since the shapes are irregular.

<a name="5.6"></a>

## 5.6 Icons

Icons are images that can be drawn in any color. They are composed of a transparency map that indicates how opaque each pixel is. When you draw it, the color you indicate is used as the master color, drawn at a given transparency for each pixel. Typically these are `const_bitmap<alpha_pixel<8>>` in the case of an 8-bit transparency map.

```cpp
// draw my_icon at position 0,0 in green:
draw::icon(lcd,spoint16(0,0),my_icon,color_t::green);
```
Note that in order to work properly the draw destination must support alpha blending or the icon must be drawn with a non-transparent background color.

See the [Tools](tools.md) page for information on getting icons for your project.

<a name="5.7"></a>

## 5.7 Alpha blending

Alpha blending occurs whenever a pixel with an alpha channel is drawn to a supporting draw target - that is a draw target which can act as both a draw source and a draw destination. To enable alpha blending, simply declare a pixel that carries an alpha channel and then use that to perform your drawing.

```cpp
// draw an alpha blended rectangle
// to bmp:
// declare a pixel w/ alpha channel:
auto col = color<rgba_pixel<32>>::red;
// set the alpha to half - 1.0 would be totally
// opaque while 0.0 would be totally transparent
col.template channelr<channel_name::A>(.5);
// draw the rectangle
draw::filled_rectangle(
    bmp,
    srect16(
        spoint16(0,0),
        ssize16(
            bmp.dimensions().width,
            bmp.dimensions().height/4)
        ),
        col);
```

<a name="5.7.1"></a>

### 5.7.1 Performance considerations

Alpha blending can take significant bus traffic and computation time and so it should be used with care. Drawing rectangles and individual pixels is fastest, assuming you have a little RAM to spare, but most alpha blending operations must draw pixel by pixel instead of pushing pixels in bulk which signficantly slows things down.

<a name="5.7.2"></a>

### 5.7.2 Draw target considerations

A draw target that can alpha blend must be both a draw source and a draw destination or alpha blending will not occur and the alpha channel will be ignored. Some displays such as the SSD1351 over SPI cannot alpha blend at all. In those cases, you must first blend to a bitmap, then draw the blended bitmap to the screen.

<a name="5.8"></a>

## 5.8 Suspend and resume

Double buffering for supported displays is facilitated through `draw::suspend<>()` and `draw::resume<>()`. Once suspend is called, subsequent drawing operations will not show up on the draw destination until resume is called on that destination. Repeated calls to `suspend<>()` must be balanced with the same number of repeated calls to `resume<>()` so that the calls can be nested. GFX automatically suspends during intrinsic drawing operations so that a line for example will be drawn all at once. However, you can use these calls to extend the suspension across several drawing operations:

```cpp
// suspend the destination
template<typename Destination>
static gfx_result suspend(
    Destination& destination)

// resume the destination, optionally
// forcing a resume despite how many
// suspensions were called
template<typename Destination>
static gfx_result resume(
    Destination& destination, 
    bool force=false)
```
This is necessary in particular for using e-paper displays due to their slow refresh rates. The idea is to suspend, draw an entire screen, and then resume:

```cpp
draw::suspend(epaper);
// draw a checkerboard tile pattern
draw::filled_rectangle(epaper, epaper.bounds(), ep_color::black);
for (int y = 0; y < epaper.dimensions().height; y += 16) {
    for (int x = 0; x < epaper.dimensions().width; x += 16) {
        if (0 != ((x + y) % 32)) {
            draw::filled_rectangle(epaper,
                                    srect16(spoint16(x, y), ssize16(16, 16)),
                                    ep_color::white);
        }
    }
}
draw::resume(epaper);
```

<a name="5.9"></a>

## 5.9 Batching

Batching can dramatically speed up complicated draw operations by avoiding sending redundant coordinate data to a device, or - memory permitting, by using a temporary memory buffer to hold the batched draws, and sending it to the target all at once.

It should be noted that you cannot alpha blend with batch operations.

```cpp
// retrieves a batch writer that can be used to write a batch operation to the display
template<typename Destination>
static inline batch_writer<Destination> batch(Destination& destination, 
                                                const srect16& bounds)
```
You use it by specifying calling `batch<>()` to get a writer, and then calling `write<>()` on that writer until you are done. The pixels will be written out top to bottom, left to right until the entire rectangle is filled. The batch is committed either when the writer goes out of scope or `commit()` is called on the writer. If you do any other operations that write to the destination outside of the batch operation itself, the results are undefined:

```cpp
hsv_pixel<24> px(true,.5,1,1);

// use batching here
auto ba = draw::batch(lcd,lcd.bounds());

// draw a gradient
for(int y = 0;y<lcd.dimensions().height;++y) {
  px.template channelr<channel_name::S>(((double)y)/lcd.bounds().y2);
  for(int x = 0;x<lcd.dimensions().width;++x) {
    px.template channelr<channel_name::V>(((double)x)/lcd.bounds().x2);
    ba.write(px);
  }
}
ba.commit();
```
[→ Positioning](positioning.md)

[← Fonts](fonts.md)

