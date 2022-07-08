# gfx

A device independent graphics library for embedded and IoT devices

Draw on anything

Documentation: https://honeythecodewitch.com/gfx/wiki/index.md

General Guide: https://www.codeproject.com/Articles/5302085/GFX-Forever-The-Complete-Guide-to-GFX-for-IoT

```
           @@@@@@@@@@                                                                                                           
        @@@@@@@@@@@@@@@@                                                                                                        
      @@@@@@@@@@@@@@@@@@@@                                                                                                      
     @@@@@@@@@@@@@@@@@@@@@@               @@@@@@@@@@@@                                                                          
   @@@@    @@@@@@@@@@    @@@@           @@@@@@@@@@@@@@@                                                                         
  @@@@      @@@@@@@@      @@@@          @@@@@      @@@@@                                                                        
  @@@@      @@@@@@@@      @@@@        @@@@@  @@@@@@  @@@@@          @@@@@@@@                                                    
 @@@@@      @@@@@@@@      @@@@@      @@@  @@@@@@@@@@@@ @@@         @@@@@@@@@@@                                                  
 @@@@@      @@@@@@@@      @@@@@      @@@@@@@@@@@@@@@@@@ @@@      @@   @@@@   @@                                                 
@@@@@@      @@@@@@@@      @@@@@@     @@@@@@@@@@@@@@@@@@ @@@      @@   @@@@   @@@                                                
@@@@@@      @@@@@@@@      @@@@@@    @@@@@@@@@@@@@@@@@@@@@@@     @@@   @@@@   @@@                                                
@@@@@@@    @@@@@@@@@@    @@@@@@@    @@@@@@@@@@@@@@@@@@@@@@@     @@@@  @@@@@  @@@                                                
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@    @@@@@   @@@@@@@@   @@@@     @@@@@@@@@@@@@@@@                                                
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@    @@@@     @@@@@@    @@@@      @@@@@@@@@@@@ @@                                                
@@@@ @@@@@@@@@@@@@@@@@@@@@@ @@@@     @@@     @@@@@@    @@@@      @@ @@@@@@@@ @@                                                 
 @@@@ @@@@@@@@@@@@@@@@@@@@ @@@@      @@@     @@@@@@    @@@@       @@@ @@@@  @@@                                                 
 @@@@ @@@@@@@@@@@@@@@@@@@@ @@@@      @@@     @@@@@@    @@@         @@@@@@@@@@                                                   
  @@@@  @@@@@@@@@@@@@@@@  @@@@        @@@   @@@@@@@@   @@@          @@@@@@@@                                                    
  @@@@@@ @@@@@@@@@@@@@@ @@@@@@          @@@@@@@@@@@@@@@@                                                                        
   @@@@@@   @@@@@@@@   @@@@@@             @@@@@@@@@@@@                                                                          
     @@@@@@@        @@@@@@@                 @@@@@@@@                                                                            
      @@@@@@@@@@@@@@@@@@@@                                                                                                      
        @@@@@@@@@@@@@@@@                                                                                                        
           @@@@@@@@@@                                                                                                           
                                                                                                                                
                                                                                                                                
#   #                                                                       #               #                                   
#   #                                             #                         #               #                                   
#   #                                                                       #               #                                   
#   #  ###  #   #  ###         ###        # ##   ##    ###   ###         ####  ###  #   #   #                                   
#####     # #   # #   #           #       ##  #   #   #   # #   #       #   #     # #   #   #                                   
#   #  #### #   # #####        ####       #   #   #   #     #####       #   #  #### #   #   #                                   
#   # #   #  # #  #           #   #       #   #   #   #     #           #   # #   # #  ##   #                                   
#   # #  ##  # #  #   #       #  ##       #   #   #   #   # #   #       #   # #  ##  ## #                                       
#   #  ## #   #    ###         ## #       #   #  ###   ###   ###         ####  ## #     #   #                                   
                                                                                    #   #                                       
                                                                                     ###                                        
```
Deprecated Documentation
------------------------
This document is being kept here because it's still largely accurate and works as an introduction, but there have been some changes to the way the code works in versions since this was written.

The official documentation is here: https://honeythecodewitch.com/gfx/wiki/index.md

This document is still useful in broad strokes, even if certain details differ.

Introduction
------------

I wanted a graphics library that was faster and better than what I had found for various IoT devices. The most popular is probably Adafruit\_GFX, but it's not optimal. It's very minimalistic, not very optimized, and doesn't have a fully decoupled driver interface. Also, being tied to the Arduino framework, it can't take advantage of platform specific features that increase performance, like being able to switch between interrupt and polling based SPI transactions on the ESP32.

GFX on the other hand, isn't tied to anything. It can draw anywhere, on any platform. It's basically standard C++, and things like line drawing and font drawing algorithms. Without a driver, it can only draw to in memory bitmaps, but once you add a driver to the mix, you can draw directly onto displays the same way you do to bitmaps.

Concepts
--------

Below is a high level summary. For more detailed information, I've been producing a series which [begins at the link](https://www.codeproject.com/Articles/5304372/GFX-In-Depth-Part-1-Drawing-Fundamentals).

### Draw Sources and Destinations

GFX introduces the idea of draw sources and destinations. These are vaguely or loosely defined types that expose a set of members that allow GFX to bind to them to perform drawing operations. The set of members they expose varies based on their capabilities, and GFX adapts to call them in the most efficient manner that the target supports for the given operation. The drawing functions in GFX take destinations, and some operations such as copying bitmapped pixel data or otherwise reading pixel information take sources.

The code size is related to how many different types of sources and destinations you have, and what kind of drawing or copying you're doing between them. The more diverse your sources and destinations, the larger the code.

Draw sources and destinations include in memory bitmaps and display drivers, but you can potentially craft your own. We'll be getting into how later.

#### Bitmaps

Bitmaps are sort of all-purpose draw source/destinations. They basically allow you to hold drawn data around in memory, which you can then draw somewhere else. Bitmaps do not hold their own memory. This is because not all memory is created equal. On some platforms for example, in order to enable a bitmap to be sent to the driver, the bitmap data must be stored in DMA capable RAM. The other reason is so you can recycle buffers. When you're done with a bitmap you can reuse the memory for another bitmap (as long as it's big enough) without deallocating or reallocating. The disadvantage is a small amount of increased code complexity wherein you must determine the size you need for the bitmap, and then allocate the memory for it yourself, and possibly freeing it yourself later.

#### Drivers

Drivers are a typically a more limited draw target, but may have performance features, which GFX will use when available. You can check what features draw sources and destinations have available using the `caps` member so that you can call the right methods for the job and for your device. You don't need to worry about that if you use the `draw` class which does all the work for you. Drivers may have certain limitations in terms of the size of bitmap they can take in one operation, or performance characteristics that vary from device to device. As much as possible, I've tried to make using them consistent across disparate sources/destinations, but some devices, like e-paper displays are so different that care must be taken when using them in order to employ them in the way that is most effective.

#### Custom

You can implement your own draw sources and destinations by simply writing a class with the appropriate members. We'll cover that toward the end.

### Pixel Types

Pixels in GFX may take any form you can imagine, up to your machine's maximum word size. They can have as many as one channel per bit, and will take on any binary footprint you specify. If you want a 3 bit RGB pixel, you can easily make one, and then create bitmaps in that format - where there are 3 pixels every 9 bits. You'll never have to worry whether or not this library can support your display driver or file format's pixel and color model. With GFX, you can define the binary footprint and channels of your pixels, and then extend GFX to support additional color models other than the 4 it supports simply by telling it how to convert to and from RGB\*.

\*Indexed pixel formats are accounted for, but there are certain restrictions in place and care must be taken when using them because they need an associated palette in order to resolve to a color.

When you declare a pixel format, it becomes part of the type signature for anything that uses it. For example, a bitmap that uses a 24-bit pixel format is a different type than one that uses a 16-bit pixel format.

Due to this, the more pixel formats you have, the greater your code size will be.

#### Alpha Blending

If you create pixels with an alpha channel, it will be respected on supported destinations. Not all devices support the necessary features to enable this. It's also a performance killer, with no way to make it faster without hardware support, which isn't currently available for any supported device. Typically, it's best to alpha blend on a bitmap, and then draw the bitmap to a driver due to performance issues and the fact that many drivers currently do not act as draw sources, and so cannot alpha blend.

#### Indexed Color/Palette Support

Some draw destinations use an indexed color scheme, wherein they might have 16 active colors for example picked from a palette of 262,144 possible colors. Or they may have a fixed set of 8 active colors and that's all. The active colors are all that can be displayed at any given point. This was common on older systems with limited frame buffer sizes. It may also be the case with some IoT display hardware, especially color e-paper displays. e-paper displays range from 2 color (monochrome) to 7 color that I've seen.

Draw targets that use an indexed pixel for their `pixel_type` - that is, devices with pixels that have a channel with a `channel_name` of `index`, are expected to expose a `palette_type` type alias that indicates the type of the palette they are using, as well as a `palette_type* palette() const` method that returns a pointer to the current palette.

When you draw to a target that has indexed pixels, a best effort attempt is made to match the requested color with one of the active colors in the palette. It will match the closest color it finds. This isn't free. It's pretty CPU intensive so buyer beware, especially when loading JPEGs or something into an indexed target. It will have to run a nearest match on every single pixel, scanning through the palette once for every pixel!

You have to be careful with indexed colors. They can't be used in isolation, because without a palette you don't have enough information to get a color from it. Draw targets can have palettes, so the *combination* of an indexed pixel and a matching draw target yields a valid color. Because of this you can't use the `color<>` template with indexed colors, for example, because without a palette there's no way to know what index value best corresponds to, for example `old_lace`, or even `white`. You'll hopefully get compile errors when you try to use indexed pixels in places where they can't be used, but worse case, you get errors at run time when trying to draw. When trying to draw, this is usually not something you have to worry about much. 

If you must translate indexed pixels to other kinds of pixels yourself can use `convert_palette_from<>()`, `convert_palette_to<>()` and `convert_palette<>()`. These will convert to and from indexed colors optionally alpha blending in the process. They take draw targets in order to get access to the palette information.

### Drawing Elements

Drawing elements are simply things that can be drawn, like lines and circles.

#### Primitives

Drawing primitives include lines, points, rectangles, arcs, ellipses  and polygons each except the first two in filled and unfilled varieties. Most take bounding rectangles to define their extents, and for some such as arcs and lines, orientation of the rectangle will alter where or how the element is drawn.

#### Draw Sources

Draw sources again, are things like bitmaps, or display drivers that support read operations. These can be drawn to draw destinations, again like other bitmaps or display drivers that support write operations. The more different types of source and destination combinations that are used in draw operations, the larger the code size. When drawing these, the orientation of the destination rectangle indicates whether to flip the drawing along the horizontal or vertical axes. The draws can also be resized, cropped, or pixel formats converted.

#### Fonts

GFX supports two types of fonts. It supports a fast raster font and TrueType fonts, depending on your needs. If you need quick and dirty, with the emphasis on quick, use the `font` class. For pretty, scalable and potentially anti-aliased fonts at the expense of performance, use the `open_font` class.

The behavior and design of each are slightly different due to different capabilities and different performance considerations. For example, raster fonts are always allocated in either RAM or PROGMEM space. This is because they are small and so that they will operate at maximum speed. TrueType fonts on the other hand, are larger, much more complicated fonts, and so GFX will stream them directly from a file as needed, trading speed for minimal RAM use. Unlike raster fonts, TrueType fonts are essentially not loaded into memory, and will only cause temporary memory allocations as needed to render text.

Both `font` and `open_font` can be loaded from a readable, seekable stream such as a `file_stream`. This if anything, will make `font` slightly quicker and `open_font` much slower than when you embed them. The raster fonts are old Windows 3.1 FON files while the TrueType font files are platform agnostic TTF files.

Alternatively, you can use the *fontgen* to create a C++ header file from a font file. This header can then be included in order to embed the font data directly into your binary. This is a static resource rather than loaded into the heap. This is the recommended way of loading fonts when you can, especially with `open_font`.

When fonts are drawn, very basic terminal control characters like tab, carriage return, and newline are supported. Raster fonts can be drawn with or without a background, though it's almost always much faster to draw them with one, at least when drawing to a display.

##### TrueType Layout Considerations

Truetype fonts must usually be downscaled from their native size before being displayed. You can use the `scale()` method passing the desired font height in pixels.

Note that sizes and positions with TrueType are somewhat approximate in that they don't always reflect what you think they might. Part of that is nature of digital typesetting and part of that is because non-commercial font files often have bad font metrics in them. It usually takes some trial and error to get them pixel perfect.

Also note that unlike raster fonts, TrueType font glyphs aren't limited to a bounding box. They can overhang part of the letter outside of the specified draw area which can lead to the left and top edges of letters in your destination area being clipped. Fortunately, you can draw text with an `offset` parameter to offset the text within the drawing area to avoid this, and/or to adjust the precise position of the text.

In addition to loading and providing basic information about fonts, the `font` and `open_font` classes also allow you to measure how much space a particular region of text will require in that font.

### Images

Images include things like JPEGs, which is currently the only format this library supports, although PNG will be included soon-ish.

Images are not drawing elements because it's not practical to either load an image into memory all at once, nor get random access to the pixel data therein, due to compression or things like progressively stored data.

In order to work with images, you can use the `draw` class which will draw all or part of the image to a destination, or you can handle a callback that reports a portion of an image at a time, along with a location that indicates where the portion is within the image. For example, for JPEGs a small bitmap (usually 8x8 or so) is returned for each 8x8 region from left to right, top to bottom. Each time you receive a portion, you can draw it to the display or some other target, or you can postprocess it or whatever.

Currently unlike with fonts, there is no tool to create headers that embed images directly into your binary. This will probably be added in a future version.

### Performance

For the most part, GFX makes a best effort attempt to reduce the number of times it has to call the driver (with the exception of batch writes), even if that means working the CPU a little harder. For example, instead of drawing a diagonal line as a series of points, GFX draws a line as series of horizontal or vertical line segments. Instead of rending a font dot by dot, GFX will batch if possible, or otherwise use run lengths to draw fonts as a series of horizontal lines instead of individual points. Trading CPU for reduced bus traffic is a win because usually you have a lot more of the former than the latter to spare, not that you have much of either one. GFX is relatively efficient, eschewing things like virtual function calls, but most of the gain is through being less chatty at the bus level.

That said, there are ways to significantly increase performance by using features of GFX which are designed for exactly that.

#### Batching

One way to increase performance by reducing bus traffic is by batching. Normally, in order to do a drawing operation, a device must typically set the target rectangle for the draw beforehand, including when setting a single pixel. On an ILI9341 display, this resolves to 6 spi transactions in order to write a pixel, due to DC line control being required which splits each command into two transactions.

One way to cut down on that overhead dramatically is to set this address window, and then fill it by writing out pixels left to right, top to bottom without specifying any coordinates. This is known as batching and it can cut bus traffic and increase performance by orders of magnitude. GFX will use it for you when it can, and if it's an available feature on the draw destination.

The primary disadvantage to this is simply the limited opportunities to use it. It's great for things like drawing filled rects, or drawing bitmaps when blting and DMA transfers are unavailable, but you have to be willing to fill an entire rectangle with pixels in order to use it. If you're drawing a font with a transparent background or a 45 degree diagonal line for example, GFX effectively won't be able to batch. Batching can occur whenever there is an entire rectangle of pixels which must be drawn, and a direct transfer of bitmap data isn't possible, either because the source or target doesn't support it, or because some sort of operation like bit depth conversion or resizing needs to happen. Batching makes for a great way to speed up these types of operations. You can't use it directly unless you talk right at the driver, because at the driver level, using it incorrectly can create problems in terms of what is being displayed. GFX will use it wherever possible when you use `draw`.

#### Double Buffering with Suspend/Resume

Some devices will support double buffering. This can reduce bus traffic when the buffer is in local RAM such as it is with the SSD1306 driver, but even if it's stored on the display device using suspend and resume can make your drawing not flicker so much. What happens is once suspended, any drawing operations are stored instead of sent to the device. Once resumed, the drawn contents get updated all at once. In some situations, this can generate more bus traffic than otherwise, because resuming typically has to send a rectangle bounding the entire modified section to the device. Therefore, this is more meant for smooth drawing than raw throughput. GFX will use it automatically when drawing, but you can use it yourself to extend the scope of the buffered draw, since GFX wouldn't know for example, that you intended to draw 10 lines before updating the display. It will however, buffer on a line by line basis even if you don't, again if the target supports it.

#### Asynchronous Operations

Asynchronous operations, when used appropriately, are a powerful way to increase the throughput of your graphics intensive application. The main downside of using it, is typically asynchronous operations incur processing overhead compared to their synchronous counterparts, coupled with the fact that you have to use it very carefully, and when transferring lots of data in order to get a benefit out of it, which means lots of RAM use.

However, when you *need* to transfer a lot of a data at a time, using asynchronous operations can be a huge win, allowing you to fire off a big transfer in the background, and then almost immediately begin drawing your next frame, well before the transfer completes.

Typically in order to facilitate this, you'll create two largeish bitmaps (say, 320x16) and then what you do is you draw to one while sending the other asynchronously, and then once the send is done, you flip them so now you're drawing on the latter one while sending the first one that you just drew.

Drawing bitmaps asynchronously is really the only time you're going to see throughput improvements. The reason there are other asynchronous API calls as well is usually in order to switch from asynchronous to synchronous operations all the pending asynchronous operations in the target's queue have to complete, so basically after you queue your bitmap to draw, you can continue to queue asynchronous line draws and such in order to avoid having to wait for the pending operations to complete. However, when you're using the flipping bitmaps method above to do your asynchronous processing, these other asynchronous methods won't be necessary, since drawing to bitmaps is always synchronous, and has nothing to do with bus traffic or queuing asynchronous bus transactions. Drawing synchronously to bitmaps does not affect the asynchronous queue of the draw target. Each asynchronous queue is specific to the draw source or destination in question.

#### Performance Differences By Framework

##### ESP-IDF

The ESP-IDF is capable of doing asynchronous DMA transfers over SPI, but right now the overall SPI throughput is less than the Arduino framework. I'm investigating why this is. I have related issues that are preventing me from supporting certain devices under the ESP-IDF, like the RA8875.

##### Arduino Framework

The Arduino Framework's SPI interop is tightly timed and fast, but doesn't support asynchronous DMA transfers - at least not explicitly - and doesn't seem to have a facility for returning errors during SPI read and write operations. Therefore I think it's less likely for wiring problems to be reported with the Arduino versions of the drivers, but I'm not exactly sure since I haven't tried to create such a scenario to test with. The Arduino framework is more likely to support a device than the ESP-IDF due to differences in the SPI communication API characteristics and behavior. `XXXX_async` methods will always be performed synchronously with the Arduino framework.

Using the GFX API
-----------------

Include *gfx.hpp* (C++17) or *gfx\_cpp14.hpp* (C++14) to access GFX. Including both will choose whichever is first, so don't include both. Pick which one you need depending on the C++ standard you are targeting.

For the ESP-IDF toolchain under platform IO I've only been able to get it to target up to C++14. The gcc compiler they use under Windows isn't new enough to support the newer standard. The C++17 version is slightly more efficient in terms of how the predefined colors function, and might actually be more efficient due to more `constexpr` resolution. Still, there's not any difference in actually using them.

Use namespace *gfx* to access GFX.

### Headers

It is not necessary to explicitly include any of these, though if you're writing driver code you can include a subset to reduce compile times a little.

-   **gfx\_core.hpp** - access to basic common types like `gfx_result`.
-   **gfx\_pixel.hpp** - access to the `pixel<>` type template.
-   **gfx\_positioning.hpp** - access to point, size, rect and path types and templates.
-   **gfx\_bitmap.hpp** - access to the `bitmap<>` type template.
-   **gfx\_drawing.hpp** - access to `draw`, the main facilities of GFX.
-   **gfx\_color.hpp** - access to the predefined colors through the `color<>` template, for **C++17** or better compilers.
-   **gfx\_color\_cpp14.hpp** - access to the predefined colors through the `color<>` template, for **C++14** compilers.
-   **gfx\_image.hpp** - access to `jpeg_image` used for loading JPEG images.
-   **gfx\_palette.hpp** - access palette support type.

 

GFX was designed using *generic programming*, which isn't common for code that targets little MCUs, but here it provides a number of benefits without many downsides, due to the way it's orchestrated.

For starters, nothing is binary coupled. You don't inherit from anything. If you tell GFX you support a method, all you need to do is implement that method. If you do not, a compile error will occur when GFX tries to use it.

The advantage of this is that methods can be inlined, templatized, and otherwise massaged in a way that simply cannot be done with a binary interface. They also don't require indirection in order to call them. The disadvantage is if that method never gets used, the compiler will never check the code in it beyond parsing it, but the only way that happens is if you implement methods that you then do not tell GFX you implemented, like asynchronous operations.

In a typical use of GFX, you will begin by declaring your types. Since everything is a template basically, you need to instantiate concrete types out of them before you can start to use them. The `using` keyword is great for this and I recommend it over `typedef` since it's templatizable and at least in my opinion, it's more readable.

You'll often need one for the driver, one for any type of bitmap you wish to declare (you'll need different types for bitmaps with different color models or bit depths, like RGB versus monochrome). Once you do that, you'll want one for the color template, for each pixel type. At the very least, you'll want one that matches the pixel\_type of your display device, such as using `using lcd_color = gfx::color<typename lcd_type::pixel_type>;` which will let you refer to things like `lcd_color::antique_white`.

Once you've done that, almost everything else is handled using the `gfx:draw` class. Despite each function on the class declaring one or more template parameters, the corresponding arguments are inferred from the arguments passed to the function itself, so you should never need to use `<>` explicitly with `draw`. Using draw, you can draw text, bitmaps, lines and simple shapes.

Beyond that, you can also declare fonts, and bitmaps. These use resources while held around, and can be passed as arguments to `draw::text<>()` and `draw::bitmap<>()` respectively.

Images do not use resources directly except for some bookkeeping during loading. They are not loaded into memory and held around, but rather the caller is called back with small bitmaps that contain portions of the image which can then be drawn to any draw destination, like a display or another bitmap. This progressive loading is necessary since realistically, most machines GFX is designed for do not have the RAM to load a real world image all at once.

### Some Basics

Let's dive into some code. The following draws a classic effect around the four edges of the screen in four different colors, with "ESP32 GFX Demo" in the center of the screen:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
draw::filled_rectangle(lcd,(srect16)lcd.bounds(),lcd_color::white);
const font& f = Bm437_ATI_9x16_FON;
const char* text = "ESP32 GFX Demo";
srect16 text_rect = f.measure_text((ssize16)lcd.dimensions(),
                        text).bounds();

draw::text(lcd,text_rect.center((srect16)lcd.bounds()),text,f,lcd_color::dark_blue);

for(int i = 1;i<100;++i) {
    // calculate our extents
    srect16 r(i*(lcd_type::width/100.0),
            i*(lcd_type::height/100.0),
            lcd_type::width-i*(lcd_type::width/100.0)-1,
            lcd_type::height-i*(lcd_type::height/100.0)-1);
    // draw the four lines
    draw::line(lcd,srect16(0,r.y1,r.x1,lcd_type::height-1),lcd_color::light_blue);
    draw::line(lcd,srect16(r.x2,0,lcd_type::width-1,r.y2),lcd_color::hot_pink);
    draw::line(lcd,srect16(0,r.y2,r.x1,0),lcd_color::pale_green);
    draw::line(lcd,srect16(lcd_type::width-1,r.y1,r.x2,lcd_type::height-1),lcd_color::yellow);
    // the ESP32 wdt will get tickled
    // unless we do this:
    vTaskDelay(1);
}
```

The first thing is the screen gets filled with white by drawing a white rectangle over the entire screen. Note that draw sources and targets report their bounds as unsigned rectangles, but `draw` typically takes signed rectangles. That's nothing an explicit cast can't solve, and we do that as we need above.

After that, we declare a reference to a `font` we included as a header file. The header file was generated from a old Windows 3.1 *.FON* file using the *fontgen* tool that ships with GFX. GFX can also load them into memory from a stream like a file rather than embedding them in the binary as a header. Each has advantages and disadvantages. The header is less flexible, but allows you to store the font as program memory rather than keeping it on the heap. Again, the former method will not yet work with the Arduino framework.

Now we declare a string literal to display, which isn't exciting, followed by something a little more interesting. We're measuring the text we're about to display so that we can center it. Keep in mind that measuring text requires an initial `ssize16` that indicates the total area the font has to work with, which allows for things like wrapping text that gets too long. Essentially measure text takes this size and returns a size that is shrunk down to the minimum required to hold the text at the given font. We then get the `bounds()` of the returned size to give us a bounding rectangle. Note that we call `center()` on this rectangle when we go to `draw::text<>()`.

After that, we draw 396 lines in total, around the edges of the display, such as to create a moire effect around the edges of the screen. Each set of lines is anchored to its own corner and drawn in its own color.

Compare the performance of line drawing with GFX to other libraries. You'll be pleasantly surprised. The further from 45 degrees (or otherwise perfectly diagonal) a line is, the faster it draws - at least on most devices - with horizontal and vertical lines being the fastest.

#### Double Buffering, Suspend and Resume

Let's try it again - or at least something similar - this time using double buffering on a supporting target, like an SSD1306 display. Note that `suspend<>()` and `resume<>()` can be called regardless of the draw destination, but they will report `gfx::gfx_result::not_supported` on targets that are not double buffered. You don't have to care that much about that, because the draws will still work, unbuffered. Anyway, here's the code:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
draw::filled_rectangle(lcd,(srect16)lcd.bounds(),lcd_color::black);
const font& f = Bm437_Acer_VGA_8x8_FON;
const char* text = "ESP32 GFX";
srect16 text_rect = srect16(spoint16(0,0),
                        f.measure_text((ssize16)lcd.dimensions(),
                        text));
                        
draw::text(lcd,text_rect.center((srect16)lcd.bounds()),text,f,lcd_color::white);

for(int i = 1;i<100;i+=10) {
    draw::suspend(lcd);

    // calculate our extents
    srect16 r(i*(lcd_type::width/100.0),
            i*(lcd_type::height/100.0),
            lcd_type::width-i*(lcd_type::width/100.0)-1,
            lcd_type::height-i*(lcd_type::height/100.0)-1);

    draw::line(lcd,srect16(0,r.y1,r.x1,lcd_type::height-1),lcd_color::white);
    draw::line(lcd,srect16(r.x2,0,lcd_type::width-1,r.y2),lcd_color::white);
    draw::line(lcd,srect16(0,r.y2,r.x1,0),lcd_color::white);
    draw::line(lcd,srect16(lcd_type::width-1,r.y1,r.x2,lcd_type::height-1),lcd_color::white);

    draw::resume(lcd);
    vTaskDelay(1);
}
```

Other than some minor differences, mostly because we're working with a much smaller display that is monochrome, it's the same code as before with one major difference - the presence of `suspend<>()` and `resume<>()` calls. Once suspend is called, further draws aren't displayed until resume is called. The calls should balance, such that to resume a display you must call resume the same number of times that you call suspend. This allows you to have subroutines which suspend and resume their own draws without messing up your code. GFX in fact, uses suspend and resume on supporting devices as it draws individual elements. The main reason you have it is so you can extend the scope across several drawing operations.

##### A Note About Suspend/Resume and E-Ink/E-Paper Displays

The refresh rate of this class of displays is extremely slow. However, GFX does not distinguish between e-paper displays and traditional TFT/LCD/OLED displays in terms of how it uses them. Therefore, in order to achieve reasonable performance, it's important to suspend and resume entire frames at a time. Animation is out of the question for these displays. Some of these displays support partial updating which in theory will improve their refresh rates. However, the displays are not well documented and I haven't been successful in getting that to work yet.

### Let's Do Polygons

Since adding polygon support, I suppose an example of that will be helpful. Here it is in practice:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
// draw a polygon (a triangle in this case)
// find the origin:
const spoint16 porg = srect16(0,0,31,31)
                        .center_horizontal((srect16)lcd.bounds())
                            .offset(0,
                                lcd.dimensions().height-32)
                                    .top_left();
// draw a 32x32 triangle by creating a path
spoint16 path_points[] = {spoint16(0,31),spoint16(15,0),spoint16(31,31)};
spath16 path(3,path_points);
// offset it so it starts at the origin
path.offset_inplace(porg.x,porg.y);
// draw it
draw::filled_polygon(lcd,path,lcd_color::coral);
```

This will draw a small triangle horizontally centered at the bottom of the screen. The most difficult bit was finding the origin, but even that's not hard, if you break down the creation of `porg` call by call.

#### A Pixel For Any Situation

You can define pixels by using the `pixel<>` template, which takes one or more `channel_traits<>` as arguments, themselves taking a name, a bit depth, and optional minimum, maximum, default value, and scale. The channel names are predefined, and combinations of channel names make up *known color models*. Known color models are models that can be converted to and from an RGB color model, essentially. Currently they include RGB, Y'UV, YbCbCr, and grayscale. Declare pixels in order to create bitmaps in different formats or to declare color pixels for use with your particular display driver's native format. In the rare case you need to define one manually, you can do something like this:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
// declare a 16-bit RGB pixel
using rgb565 = pixel<channel_traits<channel_name::R,5>,
                    channel_traits<channel_name::G,6>,
                    channel_traits<channel_name::B,5>>;
```

That declares a pixel with 3 channels, each of `uint8_t`: `R:5`, `G:6`, and `B:5`. Note that after the colon is how many effective bits it has. The `uint8_t` type is only for representing each pixel channel in value space in your code. In binary space, like laid out in an in memory bitmap, the pixel takes 16 bits, not 24. This defines a standard 16-bit pixel you typically find on color display adapters for IoT platforms. RGB is one of the known color models so there is a shorthand for declaring an RGB pixel type of any bit depth:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
using rgb565 = rgb_pixel<16>; // declare a 16-bit RGB pixel
```

This will divide the bits evenly among the channels, with remaining bits going on the green channel. It is shorthand for the longhand declaration given initially and it resolves to that.

Pixels are mainly used to represent colors, and to define the binary layout of a bitmap or framebuffer. A bitmap is a template that is typed by its pixel type. Ergo, bitmaps with different pixel types are different types themselves.

The pixels have a rich API which allow you to read and write individual channels by name or index, and get a dizzying array of metadata about the pixels which you should hopefully never need.

Most of the time you'll just need to read pixel values from a draw source, or get them from a standard color value. However, sometimes you may need to set the pixel colors yourself. 

Each pixel is composed of the channels you declared, and the channels may be accessed by "name" (`channel_name` enumeration) or by index. The values can be retrieved or set using `channel<>()` accessors for the native integer value and `channelr<>()` for real/floating point values scaled to between zero and one. Often times you need to set or get the channel programmatically based on some other compile time constant and the compiler will complain because it can't verify that the channel actually exists. In order to avoid this you can use `channel_unchecked<>()` which accesses the channel without compile time verification. If the channel does not exist, setting and getting does nothing. If you need to translate between real and integer values for a channel you can use the channel's `::scale` and `::scaler` values.

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
// declare a 24-bit rgb pixel
rgb_pixel<24> rgb888;

// set channel by index
rgb888.channel<0>(255); // max
// set channel by name
rgb888.channel<channel_name::G>(127);
// set channel real value
rgb888.channelr<2>(1.0); // max

// get red as an int type
uint8_t r = rgb888.channel<channel_name::R>();
// get green as a real type
float g = rgb888.channelr<channel_name::G>();
// get blue as an int type
uint8_t b = rgb888.channel<channel_name::B>();

// get the pixel value in big endian form
uint32_t v = rgb888.value();
```

In addition to this, there is a battery of standard color definitions provided when you include the main gfx header.

These are accessed through the `color<>` template which provides a psuedo-enum of dozens of colors in any pixel format you specify - as the template argument. Even if you retrieve `hot_pink` as a monochrome or grayscale pixel, it will do the conversion for you, or I should say the compiler will (at least with C++17, I haven't checked the asm output with 14).

##### Using the Alpha Channel

Pixels that have `channel_name::A` are said to have an alpha channel. In this case, the color can be semi-transparent. Internally the color will be blended with the background color when it is drawn, as long as the destination can support reading, or otherwise supports alpha blending natively (as in its `pixel_format` has an alpha channel). Draw destinations that do not support it will not respect the alpha channel and will not blend. Any `draw` method can take pixel colors with an alpha channel present. This is a powerful way to do color blending, but the tradeoff is a significant decrease in performance in most cases due to having to draw pixel by pixel to apply blending. The `rgba_pixel<>` template will create an RGB pixel with an alpha channel.

Here's an example of using it in the wild:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
using bmpa_type = rgba_pixel<32>;
using bmpa_color = color<bmpa_type>;

// do some alpha blended rectangles
bmpa_type col = bmpa_color::yellow;
col.channelr<channel_name::A>(.5);
col = bmpa_color::red;
col.channelr<channel_name::A>(.5);
draw::filled_rectangle(bmp,
                    srect16(
                        spoint16(0,0),
                        ssize16(
                            bmp.dimensions().width,
                            bmp.dimensions().height/4)),
                    col);
col = bmpa_color::blue;
col.channelr<channel_name::A>(.5);
draw::filled_rectangle(bmp,
                    srect16(
                        spoint16(0,0),
                        ssize16(
                            bmp.dimensions().width/4,
                            bmp.dimensions().height)),
                    col);
col = bmpa_color::green;
col.channelr<channel_name::A>(.5);
draw::filled_rectangle(bmp,
                    srect16(
                        spoint16(0,
                            bmp.dimensions().height-
                                bmp.dimensions().height/4),
                        ssize16(bmp.dimensions().width,
                            bmp.dimensions().height/4)),
                    col);
col = bmpa_color::purple;
col.channelr<channel_name::A>(.5);
draw::filled_rectangle(bmp,
                    srect16(
                        spoint16(bmp.dimensions().width
                                -bmp.dimensions().width/4,
                                0),
                        ssize16(bmp.dimensions().width/4,
                            bmp.dimensions().height)),
                    col);
```

Forgive the formatting. It's the best I could do to avoid even worse line breaks. Basically what we're doing here is creating colors from pixels with an alpha channel, and then setting the alpha channel to half, before drawing a filled rectangle over whatever was already there on the bitmap, blending the colors. The ILI9341 demo has an example of this. The others do not due to screen size constraints.

#### Ruminating on Rectangles

We've been using rectangles above a lot. As I've said, you have signed and unsigned rectangles, but you can convert between them with a cast. They also have an arsenal of manipulation methods on them. While rectangles themselves are mutable, these functions do not modify the rectangle, but rather they return a new rectangle, so for example, if you `offset()` a rectangle, a new rectangle is returned from the function. That said, there are `XXXX_inplace()` counterparts for some of these that modify the existing rectangle.

Some functions that take a destination rectangle will use it as a hint about the orientation of what it is going to draw. `draw::arc<>()` is one such method. `draw::bitmap<>()` is another. You can use the `flip_XXXX()` methods to change the orientation of a rectangle, and the `orientation()` accessor to retrieve the orientation as flags. Most drawing operations - even lines and ellipses - use rectangles as their input parameters due to their flexibility. Get used to using them, as there's a lot of functionality packed into that two coordinate structure.

#### Plotting a Course with Paths

Paths are simply a series of points, but they get interesting in terms of what we can do with them. We can find the bounding rectangle of a series of points, and determine if something intersects it, whether it represents a simple line segment series, or a polygon. You supply the buffer, for efficiency's sake, but then types like spath16 wrap it with an API that allows for offsetting\*, intersection determination, and bounding.

\* offsets are only supported in-place due to overhead of allocating a copy of a path, which I want to avoid doing casually.

#### Bitmaps, Bitmaps and More Bitmaps

It can help to think of a bitmap as a variable that holds pixel data.

It's basically an in memory draw source and draw destination.

In isolation, it's not very useful except to hold pixels around in a format suitable for use in a frame buffer, but because its data is efficiently transferable to display devices and other bitmaps, it becomes an extremely utilitarian feature of GFX.

At its simplest, it's basically an array of pixels, with a width and height associated with it, but that's not strictly true. Not all pixels are multiples of 8-bits wide, much less an even machine word size. Frame buffers may not be aligned on byte boundaries. For example, if you have an 18-bit pixel, that means there's a new pixel every 18-bits in the bitmap memory. Because of that, there's no necessarily easy way to access a bitmap as raw memory depending on the pixel format, but bitmaps provide ways to get and set the data therein.

Bitmaps do not hold their own buffers. They're basically a wrapper around a buffer you declare that turns it into a draw source/draw destination. The reason they don't hold their own buffers is because not all memory is created equal. You have stack, and then you have possibly multiple types of heap, including heap that cannot be used to do DMA transfers, like the external 4MB of PSRAM on an ESP32 WROVER.

I like to declare my fixed size bitmap buffers in the global scope (under a namespace if I don't want pollution) because that way they don't get put on the stack, and I don't have to manage heap. Plus allocating early means less potential for fragmentation later. I know people frown on globals, and I understand why, but on these little devices, they're useful in certain situations. I feel this is one of them, but your mileage may vary.

Anyway, first we have to declare our buffer. I was very careful to make my objects `constexpr` enabled so you could do things like the following:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
using bmp_type = bitmap<rgb_pixel<16>>;
// the following is for convenience:
using bmp_color = color<typename bmp_type::pixel_type>; // needs GFX color header
```

followed by:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
constexpr static const size16 bmp_size(16,16);
uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
```

To be honest, the first time I wrote code like that, I was surprised it compiled. The modern C++ compiler is a truly wonderful thing. Back in the bad old days, I used to get so frustrated that C and C++ refused to allow you to put array size declarations behind a function call, regardless of how trivial the function was. I knew why, but it didn't make me less frustrated knowing that. The expression `sizeof_buffer()` computes is `(width*height*bit_depth+7)/8`. That returns the minimum number of whole bytes required to store your bitmap data at that size and color resolution.

Now that we have all that, wrapping it with a bitmap is trivial:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
bmp_type bmp(bmp_size,bmp_buf);
// you'll probably want to do this, but not necessary if 
// you're redrawing the entire bmp anyway:
bmp.clear(bmp.bounds()); // zero the bmp memory
```

Now you can call `draw` methods passing `bmp` as the destination:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
 // draw a happy face

// bounding info for the face
// change the line below if you want a subregion
srect16 bounds=(srect16)bmp.bounds();
rect16 ubounds=(rect16)bounds;

// draw the face
draw::filled_ellipse(bmp,bounds,bmp_color::yellow);

// draw the left eye
srect16 eye_bounds_left(spoint16(bounds.width()/5,
                                bounds.height()/5),
                                ssize16(bounds.width()/5,
                                        bounds.height()/3));
draw::filled_ellipse(bmp,eye_bounds_left,bmp_color::black);

// draw the right eye
srect16 eye_bounds_right(
    spoint16(
        bmp_size.width-eye_bounds_left.x1-eye_bounds_left.width(),
        eye_bounds_left.y1
    ),eye_bounds_left.dimensions());
draw::filled_ellipse(bmp,eye_bounds_right,bmp_color::black);

// draw the mouth
srect16 mouth_bounds=bounds.inflate(-bounds.width()/7,
                                    -bounds.height()/8).normalize();
// we need to clip part of the circle we'll be drawing
srect16 mouth_clip(mouth_bounds.x1,
                mouth_bounds.y1+mouth_bounds.height()/(float)1.6,
                mouth_bounds.x2,
                mouth_bounds.y2);

draw::ellipse(bmp,mouth_bounds,bmp_color::black,&mouth_clip);
```

Now you can take that bitmap and `draw::bitmap<>()` to your display or to another bitmap, or really any draw destination. You can even draw from a bitmap (or other draw source) to itself as long as the effective source and destination rectangles do not overlap. If they do, the data will probably get corrupted.

So now this `bmp` is essentially a variable that refers to a draw target which currently holds a happy face. Neat.

The individual members of the `bitmap<>` template class are not that important. The important thing is that a bitmap is both a draw source, and a draw destination, so it can be used with `draw` functions.

##### Large Bitmaps

On little devices there's not a lot of heap, and while it may *seem* like you should be able to load a 160kB frame buffer on a 512kB system, there's the inconvenient little issue of heap fragmentation. Heap fragmentation causes there to not be 160kB of *contiguous* free space anywhere on the heap in many situations because of past deallocations, even if it's the first allocation you make after the RTOS passes you control because the heap is already "dirty" and fragmented by then. What do we do?

In GFX large bitmaps can created using the `large_bitmap<>` template class. It is a composition of a lot of smaller bitmaps such that it presents a facade of one unified draw source/destination. Using this is pretty much the same as the regular bitmap as far as GFX is concerned, even though it doesn't have all the fancy members that bitmaps do. I'll be adding to and optimizing the large bitmap API as I go, but I wanted to get it out there.

Create it like a normal bitmap except you need to pass in the segment height, in lines as the second parameter. A large bitmap is composed of a bunch of smaller bitmaps (referred to as segments) of the same width stacked vertically. Therefore, each segment is a number of lines high. Every segment but the last one (which may be smaller) has the same number of lines. Unlike normal bitmaps, you do not allocate your own memory for this but you can use custom allocator and deallocator functions if you need special platform specific heap options. It's not worth getting into code, because using it is no different than using a regular bitmap, sans some features which GFX will work around the lack of.

#### Properly Using Asynchronous Draws

Every drawing method has an asynchronous counterpart. While they do, it's usually not a good idea to use them. There are however, cases where using them can significantly increase your frame rate. The situation in which this is possible is somewhat narrow, but replicable. The key to performance is `draw::bitmap_async<>()`. This method is DMA-aware for drivers that support it, and will initiate a background DMA transfer by way of a frame write on the driver where available. This is facilitated by a driver's implementation of `copy_from_async<>()`, but in order to work at maximum efficiency there can be no cropping, resizing, flipping or color conversion of the bitmap in question - otherwise a (mostly) synchronous operation will take place. Additionally, you probably cannot use PSRAM for these transfers - you are limited to RAM that is available to use for DMA. Finally the bitmap actually has to be large enough to make it worthwhile.

What size is worthwhile? It depends. The idea is you want to be sending part of your frame to be drawn in the background *while* you're rendering the next part of the frame. In order for that to work, you'll have to kind of tune the size of the bitmap you'll be sending, but I find 10kB (320x16x16bpp) at a time or so @ 26MHz is a win. As a rule, more data at a time is better, but takes more RAM.

The code looks approximately like this under the ESP-IDF at least:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
uint16_t *lines[2];
//Allocate memory for the pixel buffers
for (int i=0; i<2; i++) {
    lines[i]=(uint16_t*)heap_caps_malloc(lcd.dimensions().width
        *PARALLEL_LINES*sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(lines[i]!=NULL);
}
using lines_bmp_type = bitmap<typename lcd_type::pixel_type>;
lines_bmp_type line_bmps[2] {
    lines_bmp_type(size16(lcd.dimensions().width,PARALLEL_LINES),lines[0]),
    lines_bmp_type(size16(lcd.dimensions().width,PARALLEL_LINES),lines[1])
};

int frame=0;
//Indexes of the line currently being sent to the LCD and the line we're calculating.
int sending_line=-1;
int calc_line=0;

// set up lines[] with data here...

++frame;
for (int y=0; y<lcd.dimensions().height; y+=PARALLEL_LINES) {
    //Calculate some lines
    do_line_effects(line_bmps[calc_line], y, frame, PARALLEL_LINES);
    // wait for the last frame to finish. Don't need this unless transactions are > 7
    if(-1!=sending_line)
        draw::wait_all_async(lcd);
    //Swap sending_line and calc_line
    sending_line=calc_line;
    calc_line=(calc_line==1)?0:1;
    //Send the lines we currently calculated.
    // draw::bitmap_async works better the larger the transfer size. Here ours is pretty big
    const lines_bmp_type& sending_bmp = line_bmps[sending_line];
    rect16 src_bounds = sending_bmp.bounds();

    draw::bitmap_async(lcd,(srect16)src_bounds.offset(0,y),sending_bmp,src_bounds);
    //The lines set is queued up for sending now; the actual sending happens in the
    //background. We can go on to calculate the next lines set as long as we do not
    //touch lines[sending_line] or the bitmap for it; 
    // the SPI sending process is still reading from that.
}
```

The basic idea here is as I said, we have two bitmaps, and we draw to one, here with `do_line_effects()` - the idea being that it fills the current frame (indicated by `calc_frame`) with some pretty colors. Then we make sure to wait for any pending operations to complete. The reason being is that we're about to start writing to the *other* `line_bmps[]` bitmap now and we need to make sure it's not still being read from in the background. The first time through as indicated by `sending_line==-1`, we skip this wait step because we weren't sending anything yet.

Next we swap out the index of our bitmaps, so like I said, we'll be drawing to the other one now. Then we simply get its bounds and pass it as well as the first bitmap to `draw::bitmap_async<>()`, and it goes full metal DMA on your hardware (assuming your hardware is capable), doing it in the background and freeing the loop here to continue almost immediately after the call so we can start drawing again, rather than waiting for the transfer to complete.

It's a little bit complicated, and I'm stewing on some ideas to make it easier to implement this pattern. I'll keep you posted. It should be noted that this technique is not exclusive to GFX, nor did I come up with it. It is in fact, a common rendering technique when you need real time double buffered animation without the memory to hold an entire frame, and also a way to stream in the background in order to animate more efficiently.

##### What About the Other XXXX\_async Methods?

These methods aren't as effective. The lack of blocking doesn't make up for the overhead for such small transactions. The main reason to use them is in the rare case where you want to continue to queue drawing operations after `draw::bitmap_async<>()`. Typically, when you switch from asynchronous to synchronous methods, the driver must wait for all pending asynchronous operations to complete. By continuing the async chain with `line_async<>()` instead of `line<>()` for example, you can prevent it from forcing a wait for the bitmap data to complete, at the cost of some extra CPU overhead.

#### Loading Images

There are two ways to get an image from a JPG stream (other formats are coming). Both require creating a stream over the input, like a file, and then using it with one of two methods:

The first, and easiest method is to use `draw::image<>()` which allows you to position the image on the destination and crop a portion of the image. There is an option for resizing but it's not currently supported. It's actually really difficult to do progressively so I'm not sure when it will be. Currently if you try to pass something other than `bitmap_resize::crop` it will return `gfx_result::not_supported`. Also currently the destination rect's orientation is ignored, so flipping isn't possible. This will be updated when I can manage it - I've got a lot to juggle.

Below `lcd` represents our target on which to draw the image:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
file_stream fs("/spiffs/image.jpg");
// TODO: check caps().read to see if the file is opened/readable
draw::image(lcd,(srect16)lcd.bounds(),&fs,rect16(0,0,-1,-1));
```

Note above since we don't know the size of the bitmap we can pass 0xFFFF or -1 for the extents and the source rectangle extents will end up based on the image size since the source rectangle is cropped to fit the image.

The second way of loading an image is passing the stream to an image loader function along with a callback (I prefer to use an anonymous method/lambda for this) that handles the progressive loading. You'll be called back multiple times, each time with a portion of the image as a bitmap, along with a location where it belongs within the image, and any state you passed along to the load function. Note that to reduce overhead, a state variable is used to pass state instead of using a functor like `std::function`. You can use a "flat" lambda that decays to a simple function pointer, and then pass your class pointer in as the `state` argument, to be reconstituted inside your callback. Often times, you won't even need a state argument because everything you're after, such as the display itself, is available globally:

``` {.lang-cplusplus data-language="C++" data-allowshrink="True" datacollapse="False"}
file_stream fs("/spiffs/image.jpg");
// TODO: check caps().read to see if the file is opened/readable
jpeg_image::load(&fs,[](size16 dimensions,
                        typename jpeg_image::region_type& region,
                        point16 location,
                        void* state){
    return draw::bitmap(lcd, // lcd is available globally
                        srect16((spoint16)location,
                                (ssize16)region.dimensions()),
                                region,region.bounds());
},nullptr);
```

#### Loading (or Embedding) Fonts

Fonts can be used with `draw::text<>()` and can either be loaded from a stream, similar to images, or they can be embedded into the binary by generating a C++ header file for them. Which way you choose depends on what you need and what you're willing to give up. With embedded, everything is a matter of robbing Peter to pay Paul.

Anyway, you'll usually want to go with the embedded fonts, unless you intend for the fonts to be able to be loaded and unloaded at runtime for some reason, or program space is at more of a premium than say, SPIFFs and RAM, or if you want to be able to load *.FON* or *.TTF* files from an SD card for example.

Speaking of *.FON* files, they are an old (primarily) raster font format from the Windows 3.1 days. Given those were 16-bit systems, the *.FON* files were to the point, with little extra overhead and were designed to be read quickly. Furthermore while being old, at least they aren't a completely proprietary format. It is possible to hunt them down online, or even make your own. For these devices, *.FON* files are a nearly ideal format, which is why they were chosen here. With IoT, everything old is new again.

You can also use .TTF files, which are more flexible, nicer, modern fonts, but you pay a significant penalty in terms of performance and complexity. 

You can use the *fontgen* tool to create header files from font files. Simply include these to embed them and then reference the font in your code. The font is a global variable with the same name as the file, including the extension, with illegal identifier characters turned into underscores.

Let's talk about the first method - embedding:

First, generate a header file from a font file using fontgen under the *tools* folder of the GFX library:

``` {.lang-shell data-language="shell" data-allowshrink="True" datacollapse="False"}
~$ fontgen myfont.fon > myfont.hpp
```

or

``` {data-allowshrink="True" datacollapse="False"}
C:\> fontgen myfont.ttf > myfont.hpp
```

Note with Windows, it might try to spit it out in UTF-16 which will mangle your header file to death. If that happens, open the header in notepad, and resave it as ASCII or UTF-8. Also note in the fontgen source there is a `#define WINDOWS` which should be set on the Windows platform.

Now you can include that in your code:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
#include "myfont.hpp"
```

This allows you to reference the font like this:

##### Raster Fonts

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
const font& f = myfont_fon;
const char* text = "Hello world!";
srect16 text_rect = f.measure_text((ssize16)lcd.dimensions(),
                        text).bounds();
draw::text(lcd,
        text_rect.center((srect16)lcd.bounds()),
        text,
        f,
        lcd_color::white);
```

The second way to access a font is by loading a *.FON* file from a stream, which stores the font around on the heap rather than embedded as a `static` `const` array in your code is to just replace the first line of code above with this:

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
file_stream fs("/spiffs/myfon.fon");
if(!fs.caps().read) {
    printf("Font file not found.\r\n");
    vTaskDelay(portMAX_DELAY);
}
font f(&fs);
```

That will create a font on the heap from the given file. You can then go on to draw it like normal. When it goes out of scope, the heap it used is reclaimed. Note that this method of loading fonts does not currently work under the Arduino framework. They must be embedded.

It is usually more efficient to draw fonts with a solid background than ones with a transparent background, so if raw performance is your ultimate goal, stick with non-transparent font draws.

##### TrueType Fonts

``` {.lang-cplusplus data-language="c++" data-allowshrink="True" datacollapse="False"}
const open_font& f=Maziro_ttf;
draw::filled_rectangle(lcd,(srect16)lcd.bounds(),lcd_color::white);
const char* text = "ESP32 GFX Demo";
float scale = f.scale(40);
srect16 text_rect = f.measure_text((ssize16)lcd.dimensions(),{5,-7},
                        text,scale).bounds();
draw::text(lcd,
        text_rect.center((srect16)lcd.bounds()),
        {5,-7},
        text,
        f,
        scale,
        lcd_color::dark_blue);
```

Note the addition of the offset and scale parameters compared to raster fonts.

Files are the same as loading raster fonts.

### GFX Draw Bindings

Drivers and other things may be draw destinations and may also be draw sources. In order to work as those things, the custom draw target must expose some members so that GFX can bind to them.

#### Common Members to All Draw Targets

The first member is a public `using caps = gfx::gfx_caps<...>;` alias that will be used to determine what kinds of features your driver supports. If, for example, you indicate batching support, GFX will attempt to call methods like `write_batch()` on your driver. If you indicate support for a feature without implementing the corresponding methods, that's a compile error:

-   **`caps`** - Indicates the capabilities of the target, which consist of these members:
    -   **blt** - the target supports accessing its memory as raw data using `begin()` and its memory must be laid out contiguously from left to right, top to bottom in the corresponding `pixel_format`.
    -   **async** - the target supports asynchronous versions of its methods. GFX will call the methods suffixed with `_async()` when asynchronous operations are requested by the caller. If it doesn't support all of them, but rather only some of them, then the implementer should delegate from the unsupported `_async()` operations to the synchronous ones for the methods where there is no asynchronous counterpart.
    -   **batch** - the target supports batching write operations and is expected to expose `begin_batch()`, `write_batch()`, and `commit_batch()`.
    -   **copy\_from** - the target supports optimized copying from a draw source, and GFX should use the exposed `copy_from<>()` template method when possible.
    -   **suspend** - the target supports granular double buffering, wherein drawing operations can be written offscreen when `suspend()` is called and then a portion of the screen that was updated sent to the display upon `resume()`. The implementor should keep a count of suspends to balance the calls of suspend with those to resume.
    -   **read** - the target supports reading pixel data, which facilitates its use as a draw source and also enables alpha blending.
    -   **copy\_to** - the target supports optimized copying to a draw destination and GFX should use the exposed `copy_to<>()` method when possible. If given the choice between using a draw source's `copy_to<>()` method and a draw destinations's `copy_from<>()` method, GFX will choose the `copy_from<>()` method, since there are more optimization opportunities with that.

Next, you have to declare the `using pixel_type` alias on your draw target. This is probably most often an alias for `gfx::rgb_pixel<16>` for color displays and `gfx::gsc_pixel<1>` for monochrome (1 bit grayscale) displays. It tells GFX what the native format of your draw object is.

-   `pixel_type` - indicates the native pixel format for this target

If your pixel type is indexed, meaning it contains `channel_name::index`, you must include using `palette_type` alias for your palette type. For drivers like e-paper displays, they will usually have an associated palette class that this aliases.

-   `palette_type` - indicates the associated palette type if `pixel_type` refers to an indexed pixel.

Now you can start implementing methods you'll need. Most of the methods return the `enum` `gfx::gfx_result` indicating the status of the operation.

First, aside from the `caps` and `pixel_type` aliases, there are methods you must implement regardless:

-   `size16 dimensions() const` - returns a `size16` that indicates the dimensions of the draw target.
-   `rect16 bounds() const` - returns a `rect16` with a top left corner of (0,0) and a width and height equal to that of the draw target. This is an alias for `dimensions().bounds()`.

Next, if your `pixel_type` refers to an indexed pixel you must implement a `palette()` method which returns a pointer to a palette associated with your draw target.

-   `const palette_type* palette() const` - returns a pointer to the palette associated with this draw target

#### Draw Source Members

To implement a target as a draw source, you must additionally implement one, or possibly two methods:

-   `gfx_result point(point16 location, pixel_type* out_color) const` - retrieves a pixel at the specified location
-   `gfx_result copy_to<typename Destination>(const rect16& src_rect,Destination& dst,point16 location) const` - copies a portion of the target to the specified destination at the specified location

If you implement `copy_to<>()` be sure to set the corresponding entry in your `caps` so that GFX will call it.

Either way, now you can use it as a source to calls like `draw::bitmap<>()`.

#### Draw Destination Members

Because of the variety of optimizations necessary to achieve good performance on device drivers, it can be significantly more involved to implement a draw destination than a draw source.

The least you must implement other than the common methods are the first two methods, but the methods that follow those are optional and allow for better performance. I will not be listing the `_async()` methods to save space, since their names and signatures are derived from the synchronous methods.

-   `gfx_result point(point16 location, pixel_type color)` - sets a pixel at the specified location
-   `gfx_result fill(const rect16& rect, pixel_type color)` - fills a rectangle at the specified location

Above is the minimum. What the rest is depends on the `caps` settings.

An important one for doing high performance copies of bitmap data is `copy_from<>()`:

-   `template<typename Source> gfx_result copy_from(const rect16& src_rect,const Source& src,point16 location)` - copys from a draw source to the draw target.

This method should determine what the best action to be taken for sending the `src`'s data to this target as quickly as possible. It should not use the source's `copy_to<>()` method but aside from that it can use anything. Often what one will do is internally call another template that specializes for when `Source::pixel_type` is the same as `pixel_type`, and that the source can be `blt`ed. then potentially do a raw memory read. If so, you may be able to initiate a DMA transfer, for example. Then you have to have fallback scenarios if that isn't supported.

Next we'll cover batching. If you support batching operations, you'll need to implement the following three methods:

-   `gfx_result begin_batch(const rect16& rect)` - begins a batch operation at the specified rectangle.
-   `gfx_result write_batch(pixel_type color)` - writes a pixel to the current batch. Pixels are written to the batch rectangle in order from left to right, top to bottom. If your pixel buffer gets full, it's acceptable to send as needed. Batch isn't suspend, it's just a way to cut down traffic.
-   `gfx_result commit_batch()` - writes any remaining batched data out and reverts from batching mode.

When you implement batching, make sure that you automatically commit the current batch if the caller begins a non-batch operation while in the middle of a batch, or if they begin a new batch operation.

If you support suspend/resume (double buffering) you'll need to implement the following two methods:

-   `gfx_result suspend()` - suspends drawing. Suspends are counted and balanced, so for every suspend call, you must make a corresponding `resume()` call.

-   `gfx_result resume(bool force=false)` - resumes drawing, optionally discarding all previous suspends() and forcibly resuming the draw. The screen is updated when the final resume is called.

Finally, every one of the writing methods potentially has an `_async()` counterpart that take the same parameters but queue the operation and return as soon as possible. There is currently no provision for doing async reads but that will change in the future.

History
-------

-   10<sup>th</sup> May, 2021 - Initial submission
-   16<sup>th</sup> May, 2021 - Added drivers, configurations, wiring guide
-   17<sup>th</sup> May, 2021 - Added more drivers
-   18<sup>th</sup> May, 2021 - Added sprite/transparent color support to `draw::bitmap<>()`
-   21<sup>st</sup> May, 2021 - Bugfix and added another driver
-   21<sup>st</sup> May, 2021 - `draw::bitmap<>()` bugfix
-   24<sup>th</sup> May, 2021 - Fixed build errors on some demos
-   27<sup>th</sup> May, 2021 - Added alpha blending support
-   29<sup>th</sup> May, 2021 - Added `large_bitmap<>` support, API changes, demo changes
-   31<sup>st</sup> May, 2021 - API cleanup and added paths and polygon support
-   31<sup>st</sup> May, 2021 - Fixed several build errors
-   1<sup>st</sup> June, 2021 - Added/fixed bitmap resize options and added dimensions to image callback
-   5<sup>th</sup> June, 2021 - Added single header file, and easier to use image loading. cleaned up positioning api a bit. bugfix in declarions of clipping rect parameters on `draw::`
-   7<sup>th</sup> June, 2021 - Service release. Certain draw operations between certain draw targets would fail to compile
-   8<sup>th</sup> June, 2021 - Added palette/CLUT support (initial/experimental)
-   8<sup>th</sup> June, 2021 - Service release. Fixed `large_bitmap<>` out of bounds crashing issue
-   13<sup>th</sup> June, 2021 - Added Arduino framework support and several Arduino based drivers
-   15<sup>th</sup> June, 2021 - Added support for two e-ink/e-paper displays: the DEP0290B (and the associated LilyGo T5 2.2 board) as well as the GDEH0154Z90 (WaveShare 1.54 inch 3-color black/white/red display).
-   17<sup>th</sup> June, 2021 - Added dithering support for e-ink/e-paper displays
-   13<sup>th</sup> July, 2021 - Added TrueType font support

