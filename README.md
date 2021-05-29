# gfx

A device independent graphics library for embedded and IoT devices

Draw on anything

Read more here: https://www.codeproject.com/Articles/5302085/GFX-Forever-The-Complete-Guide-to-GFX-for-IoT

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
Introduction
------------

I wanted a graphics library that was faster and better than what I had found for various IoT devices. The most popular is probably Adafruit\_GFX, but it's not optimal. It's very minimalistic, not very optimized, and doesn't have a fully decoupled driver interface. Also, being tied to the Arduino framework, it can't take advantage of platform specific features that increase performance, like being able to switch between interrupt and polling based SPI transactions on the ESP32.

GFX on the other hand, isn't tied to anything. It can draw anywhere, on any platform. It's basically standard C++, and things like line drawing and font drawing algorithms. Without a driver, it can only draw to in memory bitmaps, but once you add a driver to the mix, you can draw directly onto displays the same way you do to bitmaps.

**Update:** Some minor bugfixes, SPI drivers are refactored to use a common base, more drivers are now added, and one click configuration for generic ESP32 boards is now available

**Update 2:** Included support for the LilyGo TTGO board, as well as the green tab 128x128 1.44" ST7735 display (though other green tab models may work too they have not been tested)

**Update 3:** Added `transparent_color` argument to `draw::bitmap<>()` so you can do sprites.

**Update 4:** GFX `draw::rectangle<>()` bugfix and added experimental MAX7219 driver

**Update 5:** Bug fixes with bitmap drawing

**Update 6:** Fixed some of the demos that were failing the build

**Update 7:** Added alpha blending support!

**Update 8:** Some API changes, added `large_bitmap<>` support and edged a little closer to adaptive indexed color support. I also used the large bitmap for the frame buffer in the demos, and changed that code to be easier to understand at the cost of raw pixel throughput.

### Building this Mess

You'll need Visual Studio Code with the Platform IO extension installed. You'll need an ESP32 with a connected ILI9341 LCD display an SSD1306 display, or other display depending on the demo.

I recommend the [Espressif ESP-WROVER-KIT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-wrover-kit.html) development board which has an integrated ILI9341 display and several other pre-wired peripherals, plus an integrated debugger and a superior USB to serial bridge with faster upload speeds. They can be harder to find than a standard ESP32 devboard, but I found them at JAMECO and Mouser for about \$40 USD. They're well worth the investment if you do ESP32 development. The integrated debugger, though very slow compared to a PC, is faster than you can get with an external JTAG probe attached to a standard WROVER devboard.

Most of you however, will be using one or more of the "generic-esp32" configurations. At the bottom of the screen in the blue bar of VS Code, there is a configuration switcher. It should be set at Default to start, but you can change it by clicking on default. A list of many configurations will drop down from the top of the screen. From there, you can choose which generic-esp32 setup you have, based on the display attached to it. 

In order to wire all this up, refer to *wiring\_guide.txt* which has display wirings for SPI and I2C displays. Keep in mind some display vendors name their pins with non-standard names. For example, on some displays `MOSI` might be labled as `DIN` or `A0`. You make have to do some googling to find out the particulars for your device.

Before you can run it, you must Upload Filesystem Image under the Platform IO sidebar - Tasks.

**Note**: The Platform IO IDE is kind of cantankerous sometimes. The first time you open the project, you'll probably need to go to the Platform IO icon on the left side - it looks like an alien. Click it to open up the sidebar and look under *Quick Access|Miscellaneous* for *Platform IO Core CLI*. Click it, and then when you get a prompt type `pio run` to force it to download necessary components and build. You shouldn't need to do this again, unless you start getting errors again while trying to build. Also, for some reason, whenever you switch a configuration you have to go and refresh (the little circle-arrow next to "PROJECT TASKS") before it will take.

Currently, the following configurations should work:

-   esp-wrover-kit

-   esp32-ILI9341

-   esp32-ST7789

-   esp32-ST7735

-   esp32-SSD1306

-   esp32-SSD1351

-   esp32-TTGO

-   esp32-MAX7219\* 

The demos are not all the same despite GFX capabilities being pretty much the same for each display. The reason is that there are no asynchronous operations for I2C devices, and because some of the displays simply aren't big enough to show the jpgs or they are monochrome like the SSD1306, so the JPG would look a mess.

\*MAX7219 CS pin is 15, not 5. Driver is experimental and multiple rows of segments does not work, but multiple segments in a single row works.

Concepts
--------

### Draw Sources and Destinations

Include *gfx\_drawing.hpp*

GFX introduces the idea of draw sources and destinations. These are vaguely or loosely defined types that expose a set of members that allow GFX to bind to them to perform drawing operations. The set of members they expose varies based on their capabilities, and GFX adapts to call them in the most efficient manner that the target supports for the given operation. The drawing functions in GFX take destinations, and some operations such as copying bitmapped pixel data or otherwise reading pixel information take sources.

The code size is related to how many different types of sources and destinations you have, and what kind of drawing or copying you're doing between them. The more diverse your sources and destinations, the larger the code.

Draw sources and destinations include in memory bitmaps and display drivers, but you can potentially craft your own. We'll be getting into how later.

#### Bitmaps

Include *gfx\_bitmap.hpp* to use bitmaps

Bitmaps are sort of all-purpose draw source/destinations. They basically allow you to hold drawn data around in memory, which you can then draw somewhere else. Bitmaps do not hold their own memory. This is because not all memory is created equal. On some platforms for example, in order to enable a bitmap to be sent to the driver, the bitmap data must be stored in DMA capable RAM. The other reason is so you can recycle buffers. When you're done with a bitmap you can reuse the memory for another bitmap (as long as it's big enough) without deallocating or reallocating. The disadvantage is a small amount of increased code complexity wherein you must determine the size you need for the bitmap, and then allocate the memory for it yourself, and possibly freeing it yourself later.

#### Drivers

Drivers are a typically a more limited draw destination, but may have performance features, which GFX will use when available. You can check what features draw sources and destinations have available using the `caps` member so that you can call the right methods for the job and for your device. You don't need to worry about that if you use the `draw` class which does all the work for you. Drivers may have certain limitations in terms of the size of bitmap they can take in one operation, or performance characteristics that vary from device to device. As much as possible, I've tried to make using them consistent across disparate sources/destinations.

#### Custom

You can implement your own draw sources and destinations by simply writing a class with the appropriate members.

### Pixel Types

Include *gfx\_pixel.hpp* - usually not necessary as it's included in almost everything else

Include *gfx\_color\_cpp14.hpp* (C++14 spec) or *gfx\_color.hpp* (C++17 spec) for access to predefined colors in any pixel format.

Pixels in GFX may take any form you can imagine, up to your machine's maximum word size. They can have as many as one channel per bit, and will take on any binary footprint you specify. If you want a 3 bit RGB pixel, you can easily make one, and then create bitmaps in that format - where there are 3 pixels every 9 bytes. You'll never have to worry whether or not this library can support your display driver or file format's pixel and color model. With GFX, you can define the binary footprint and channels of your pixels, and then extend GFX to support additional color models other than the 4 it supports simply by telling it how to convert to and from RGB\*.

\*YCbCr is currently bugged. It doesn't quite convert to RGB properly, or from RGB at all. Indexed pixel formats are partially accounted for, but there is no CLUT/palette support yet.

When you declare a pixel format, it becomes part of the type signature for anything that uses it. For example, a bitmap that uses a 24-bit pixel format is a different type than one that uses a 16-bit pixel format.

Due to this, the more pixel formats you have, the greater your code size will be.

### Alpha Blending

If you create pixels with an alpha channel, it will be respected on supported destinations. Not all devices support the necessary features to enable this. It's also a performance killer, with no way to make it faster without hardware support, which isn't currently available for any supported device.

### Drawing Elements

Drawing elements are simply things that can be drawn, like lines and circles.

#### Primitives

Drawing primitives include lines, points, rectangles, arcs, and ellipses each except the first two in filled and unfilled varieties. Most take bounding rectangles to define their extents, and for some such as arcs and lines, orientation of the rectangle will alter where or how the element is drawn.

#### Draw Sources

Draw sources again, are things like bitmaps, or display drivers that support read operations. These can be drawn to draw destinations, again like other bitmaps or display drivers that support write operations. The more different types of source and destination combinations that are used in draw operations, the larger the code size. When drawing these, the orientation of the destination rectangle indicates whether to flip the drawing along the horizontal or vertical axes. The draws can also be resized, cropped, or pixel formats converted.

#### Fonts

Fonts can be included as legacy *.FON* files like those used in Windows 3.1. The file format is low overhead, and designed for 16 bit systems, so the fonts are very simple and compact, while supporting relatively rich features for an IoT device, like variable width fonts. When loading a font this way, it will be allocated on the heap.

Alternatively, you can use the *fontgen* to create a C++ header file from a font file. This header can then be included in order to embed the font data directly into your binary. This is a static resource rather than loaded into the heap.

When fonts are drawn, very basic terminal control characters like tab, carriage return, and newline are supported. They can be drawn with or without a background, though it's almost always much faster to draw them with one, at least when drawing to a display.

In addition to loading and providing basic information about fonts, the `font` class also allows you to measure how much space a particular region of text will require in that font.

### Images

Include *gfx\_image.hpp* for access to images

Images include things like JPEGs, which is currently the only format this library supports, although PNG will be included very soon.

Images are not drawing elements because it's not practical to either load an image into memory all at once, nor get random access to the pixel data therein, due to compression or things like progressively stored data.

In order to work with images, you handle a callback that reports a portion of an image at a time, along with a location that indicates where the portion is within the image. For example, for JPEGs a small bitmap (usually 8x8 or so) is returned for each 8x8 region from left to right, top to bottom. Each time you receive a portion, you can draw it to the display or some other target, or you can postprocess it or whatever.

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

Using the GFX API
-----------------

Use namespace *gfx* to access GFX.

### Headers

-   **gfx\_core.hpp** - access to basic common types like `gfx_result`. Not usually necessary to explicitly include.
-   **gfx\_pixel.hpp** - access to the `pixel<>` type template. Not usually necessary to explicitly include.
-   **gfx\_positioning.hpp** - access to point, size, and rect types and templates. Not usually necessary to explicitly include.
-   `gfx_bitmap.hpp` - access to the `bitmap<>` type template.
-   `gfx_drawing.hpp` - access to `draw`, the main facilities of GFX. Include this to use GFX.
-   `gfx_color.hpp` - access to the predefined colors through the `color<>` template, for **C++17** or better compilers. Include this if needed.
-   `gfx_color_cpp14.hpp` - access to the predefined colors through the `color<>` template, for **C++14** compilers. Include this if needed. Do not include both.
-   `gfx_image.hpp` - access to `jpeg_image` used for loading JPEG images. Include this if necessary.

 

GFX was designed using *generic programming*, which isn't common for code that targets little MCUs, but here it provides a number of benefits without many downsides, due to the way it's orchestrated.

For starters, nothing is binary coupled. You don't inherit from anything. If you tell GFX you support a method, all you need to do is implement that method. If you do not, a compile error will occur when GFX tries to use it.

The advantage of this is that methods can be inlined, templatized, and otherwise massaged in a way that simply cannot be done with a binary interface. They also don't require indirection in order to call them. The disadvantage is if that method never gets used, the compiler will never check the code in it beyond parsing it, but the only way that happens is if you implement methods that you then do not tell GFX you implemented, like asynchronous operations.

In a typical use of GFX, you will begin by declaring your types. Since everything is a template basically, you need to instantiate concrete types out of them before you can start to use them. The `using` keyword is great for this and I recommend it over `typedef` since it's templatizable and at least in my opinion, it's more readable.

You'll often need one for the driver, one for any type of bitmap you wish to declare (you'll need different types for bitmaps with different color models or bit depths, like RGB versus monochrome), and you'll probably want to include one of the GFX color headers, either *gfx\_color.hpp* for *C++17* or better, or *gfx\_color\_cpp14.hpp* for *C++14*. Once you do that, you'll want one for the color template, for each pixel type. At the very least, you'll want one that matches the pixel\_type of your display device, such as using `using lcd_color = gfx::color<typename lcd_type::pixel_type>;` which will let you refer to things like `lcd_color::antique_white`.

Once you've done that, almost everything else is handled using the `gfx:draw` class. Despite each function on the class declaring one or more template parameters, the corresponding arguments are inferred from the arguments passed to the function itself, so you should never need to use `<>` explicitly with `draw`. Using draw, you can draw text, bitmaps, lines and simple shapes.

Beyond that, you can also declare fonts, and bitmaps. These use resources while held around, and can be passed as arguments to `draw::text<>()` and `draw::bitmap<>()` respectively.

Images do not use resources directly except for some bookkeeping during loading. They are not loaded into memory and held around, but rather the caller is called back with small bitmaps that contain portions of the image which can then be drawn to any draw destination, like a display or another bitmap. This progressive loading is necessary since realistically, most machines GFX is designed for do not have the RAM to load a real world image all at once.

Let's dive into some code. The following draws a classic effect around the four edges of the screen in four different colors, with "ESP32 GFX Demo" in the center of the screen:

C++


``` {#pre588163 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

After that, we declare a reference to a `font` we included as a header file. The header file was generated from a old Windows 3.1 *.FON* file using the *fontgen* tool that ships with GFX. GFX can also load them into memory from a stream like a file rather than embedding them in the binary as a header. Each has advantages and disadvantages. The header is less flexible, but allows you to store the font as program memory rather than keeping it on the heap.

Now we declare a string literal to display, which isn't exciting, followed by something a little more interesting. We're measuring the text we're about to display so that we can center it. Keep in mind that measuring text requires an initial `ssize16` that indicates the total area the font has to work with, which allows for things like wrapping text that gets too long. Essentially measure text takes this size and returns a size that is shrunk down to the minimum required to hold the text at the given font. We then get the `bounds()` of the returned size to give us a bounding rectangle. Note that we call `center()` on this rectangle when we go to `draw::text<>()`.

After that, we draw 396 lines in total, around the edges of the display, such as to create a moire effect around the edges of the screen. Each set of lines is anchored to its own corner and drawn in its own color.

Compare the performance of line drawing with GFX to other libraries. You'll be pleasantly surprised. The further from 45 degrees (or otherwise perfectly diagonal) a line is, the faster it draws - at least on most devices - with horizontal and vertical lines being the fastest.

Let's try it again - or at least something similar - this time using double buffering on a supporting target, like an SSD1306 display. Note that `suspend<>()` and `resume<>()` can be called regardless of the draw destination, but they will report `gfx::gfx_result::not_supported` on targets that are not double buffered:

C++


``` {#pre53281 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

#### A Pixel For Any Situation

You can define pixels by using the `pixel<>` template, which takes one or more `channel_traits<>` as arguments, themselves taking a name, a bit depth, and optional minimum, maximum, and scale. The channel names are predefined, and combinations of channel names make up *known color models*. Known color models are models that can be converted to and from an RGB color model, essentially. Currently they include RGB, Y'UV, YbCbCr, and grayscale. Declare pixels in order to create bitmaps in different formats or to declare color pixels for use with your particular display driver's native format. In the rare case you need to define one manually, you can do something like this:

C++


``` {#pre651792 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
// declare a 16-bit RGB pixel
using rgb565 = pixel<channel_traits<channel_name::R,5>,
                    channel_traits<channel_name::G,6>,
                    channel_traits<channel_name::B,5>>;
```

That declares a pixel with 3 channels, each of `uint8_t`: `R:5`, `G:6`, and `B:5`. Note that after the colon is how many effective bits it has. The `uint8_t` type is only for representing each pixel channel in value space in your code. In binary space, like laid out in an in memory bitmap, the pixel takes 16 bits, not 24. This defines a standard 16-bit pixel you typically find on color display adapters for IoT platforms. RGB is one of the known color models so there is a shorthand for declaring an RGB pixel type of any bit depth:

C++

``` {#pre445033 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
using rgb565 = rgb_pixel<16>; // declare a 16-bit RGB pixel
```

This will divide the bits evenly among the channels, with remaining bits going on the green channel. It is shorthand for the longhand declaration given initially and it resolves to that.

Pixels are mainly used to represent colors, and to define the binary layout of a bitmap or framebuffer. A bitmap is a template that is typed by its pixel type. Ergo, bitmaps with different pixel types are different types themselves.

The pixels have a rich API which allow you to read and write individual channels by name or index, and get a dizzying array of metadata about the pixels which you should hopefully never need.

In addition to this, there is a battery of standard color definitions provided in a couple of headers - one for C++14 and a better alternative for C++17, named *gfx\_color\_cpp14.hpp* and *gfx\_color.hpp*, respectively.

These define the `color<>` template which provides a psuedo-enum of dozens of colors in any pixel format you specify - as the template argument. Even if you retrieve `hot_pink` as a monochrome or grayscale pixel, it will do the conversion for you, or I should say the compiler will (at least with C++17, I haven't checked the asm output with 14).

##### Using the Alpha Channel

Pixels that have `channel_name::A` are said to have an alpha channel. In this case, the color can be semi-transparent. Internally the color will be blended with the background color when it is drawn, as long as the destination can support reading, or otherwise supports alpha blending natively (as in its `pixel_format` has an alpha channel). Draw destinations that do not support it always report black for the background color to blend with. Any `draw` method can take pixel colors with an alpha channel present. This is a powerful way to do color blending, but the tradeoff is a significant decrease in performance in most cases due to having to draw pixel by pixel to apply blending. It's also necessary to convert other color models to RGB to do the blending operation, so if conversion is lossy the colors will be fuzzed somewhat. The `rgba_pixel<>` template will create an RGB pixel with an alpha channel.

Here's an example of using it in the wild:

C++

``` {#pre347446 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

We've been using rectangles above a lot. As I've said, you have signed and unsigned rectangles, but you can convert between them with a cast. They also have an arsenal of manipulation methods on them. While rectangles themselves are mutable, these functions do not modify the rectangle, but rather they return a new rectangle, so for example, if you `offset()` a rectangle, a new rectangle is returned from the function.

Some functions that take a destination rectangle will use it as a hint about the orientation of what it is going to draw. `draw::arc<>()` is one such method. `draw::bitmap<>()` is another. You can use the `flip_XXXX()` methods to change the orientation of a rectangle, and the `orientation()` accessor to retrieve the orientation as flags. Most drawing operations - even lines and ellipses - use rectangles as their input parameters due to their flexibility. Get used to using them, as there's a lot of functionality packed into that two coordinate structure.

#### Bitmaps, Bitmaps and More Bitmaps

It can help to think of a bitmap as a variable that holds pixel data.

It's basically an in memory draw source and draw destination.

In isolation, it's not very useful except to hold pixels around in a format suitable for use in a frame buffer, but because its data is efficiently transferable to display devices and other bitmaps, it becomes an extremely utilitarian feature of GFX.

At its simplest, it's basically an array of pixels, with a width and height associated with it, but that's not strictly true. Not all pixels are multiples of 8-bits wide, much less an even machine word size. Frame buffers may not be aligned on byte boundaries. For example, if you have an 18-bit pixel, that means there's a new pixel every 18-bits in the bitmap memory. Because of that, there's no necessarily easy way to access a bitmap as raw memory depending on the pixel format, but bitmaps provide ways to get and set the data therein.

Bitmaps do not hold their own buffers. They're basically a wrapper around a buffer you declare that turns it into a draw source/draw destination. The reason they don't hold their own buffers is because not all memory is created equal. You have stack, and then you have possibly multiple types of heap, including heap that cannot be used to do DMA transfers, like the external 4MB of PSRAM on an ESP32 WROVER.

I like to declare my fixed size bitmap buffers in the global scope (under a namespace if I don't want pollution) because that way they don't get put on the stack, and I don't have to manage heap. Plus allocating early means less potential for fragmentation later. I know people frown on globals, and I understand why, but on these little devices, they're useful in certain situations. I feel this is one of them, but your mileage may vary.

Anyway, first we have to declare our buffer. I was very careful to make my objects `constexpr` enabled so you could do things like the following:

C++

``` {#pre348427 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
using bmp_type = bitmap<rgb_pixel<16>>;
// the following is for convenience:
using bmp_color = color<typename bmp_type::pixel_type>; // needs GFX color header
```

followed by:

C++

``` {#pre254885 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
constexpr static const size16 bmp_size(16,16);
uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
```

To be honest, the first time I wrote code like that, I was surprised it compiled. The modern C++ compiler is a truly wonderful thing. Back in the bad old days, I used to get so frustrated that C and C++ refused to allow you to put array size declarations behind a function call, regardless of how trivial the function was. I knew why, but it didn't make me less frustrated knowing that. The expression `sizeof_buffer()` computes is `(width*height*bit_depth+7)/8`. That returns the minimum number of whole bytes required to store your bitmap data at that size and color resolution.

Now that we have all that, wrapping it with a bitmap is trivial:

C++

``` {#pre684719 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
bmp_type bmp(bmp_size,bmp_buf);
// you'll probably want to do this, but not necessary if 
// you're redrawing the entire bmp anyway:
bmp.clear(bmp.bounds()); // zero the bmp memory
```

Now you can call `draw` methods passing `bmp` as the destination:

C++

``` {#pre516987 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

C++

``` {#pre829136 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

Loading images is a matter of creating a stream over the input, like a file, and then passing that to an image loader function along with a callback (I prefer to use an anonymous method/lambda for this) that handles the progressive loading. You'll be called back multiple times, each time with a portion of the image as a bitmap, along with a location where it belongs within the image, and any state you passed along to the load function. Note that to reduce overhead, a state variable is used to pass state instead of using a functor like `std::function`. You can use a "flat" lambda that decays to a simple function pointer, and then pass your class pointer in as the `state` argument, to be reconstituted inside your callback. Often times, you won't even need a state argument because everything you're after, such as the display itself, is available globally:

C++

``` {#pre554653 .lang-cplusplus style="margin-top:0;" data-language="C++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
file_stream fs("/spiffs/image.jpg");
jpeg_image::load(&fs,[](const typename jpeg_image::region_type& region,
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

Anyway, you'll usually want to go with the embedded fonts, unless you intend for the fonts to be able to be loaded and unloaded at runtime for some reason, or program space is at more of a premium than say, SPIFFs and RAM, or if you want to be able to load *.FON* files from an SD card for example.

Speaking of *.FON* files, they are an old (primarily) raster font format from the Windows 3.1 days. Given those were 16-bit systems, the *.FON* files were to the point, with little extra overhead and were designed to be read quickly. Furthermore while being old, at least they aren't a completely proprietary format. It is possible to hunt them down online, or even make your own. For these devices, *.FON* files are a nearly ideal format, which is why they were chosen here. With IoT, everything old is new again.

You can use the *fontgen* tool to create header files from font files. Simply include these and reference the font in your code. The font is a global variable with the same name as the file, including the extension, with illegal identifier characters turned into underscores.

Let's talk about the first method - embedding:

First, generate a header file from a font file using fontgen under the *tools* folder of the GFX library:

C++

``` {#pre986639 .lang-cplusplus style="margin-top:0;" data-language="C++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
~$ fontgen myfont.fon > myfont.hpp
```

Now you can include that in your code:

C++

``` {#pre284808 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
#include "myfont.hpp"
```

This allows you to reference the font like this:

C++

``` {#pre342699 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
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

The second way to access a font is by loading a *.FON* file from a stream, which stores the font around on the heap rather than embedded as a `static` `const` array in your code, just replace the first line of code above with this:

C++

``` {#pre48749 .lang-cplusplus style="margin-top:0;" data-language="c++" data-collapse="False" data-linecount="False" data-allow-shrink="True"}
io::file_stream fs("/spiffs/myfon.fon");
if(!fs.caps().read) {
    printf("Font file not found.\r\n");
    vTaskDelay(portMAX_DELAY);
}
font f(&fs);
```

That will create a font on the heap from the given file. You can then go on to draw it like normal. When it goes out of scope, the heap it used is reclaimed.

It is usually more efficient to draw fonts with a solid background than ones with a transparent background, so if raw performance is your ultimate goal, stick with non-transparent font draws.

### GFX Draw Bindings

Drivers and other things may be draw destinations and may also be draw sources. In order to work as those things, the custom draw target must expose some members so that GFX can bind to them.

#### Common Members to All Draw Targets

The first member is a public `using caps = gfx::gfx_caps<...>;` alias that will be used to determine what kinds of features your driver supports. If, for example, you indicate batching support, GFX will attempt to call methods like `write_batch()` on your driver. If you indicate support for a feature without implementing the corresponding methods, that's a compile error:

-   caps - Indicates the capabilities of the target, which consist of these members:
    -   **blt** - the target supports accessing its memory as raw data using `begin()` and its memory must be laid out contiguously from left to right, top to bottom in the corresponding `pixel_format`.
    -   **async** - the target supports asynchronous versions of its methods. GFX will call the methods suffixed with `_async()` when asynchronous operations are requested by the caller. If it doesn't support all of them, but rather only some of them, then the implementer should delegate from the unsupported `_async()` operations to the synchronous ones for the methods where there is no asynchronous counterpart.
    -   **batch** - the target supports batching write operations and is expected to expose `begin_batch()`, `write_batch()`, and `commit_batch()`.
    -   **copy\_from** - the target supports optimized copying from a draw source, and GFX should use the exposed `copy_from<>()` template method when possible.
    -   **suspend** - the target supports granular double buffering, wherein drawing operations can be written offscreen when `suspend()` is called and then a portion of the screen that was updated sent to the display upon `resume()`. The implementor should keep a count of suspends to balance the calls of suspend with those to resume.
    -   **read** - the target supports reading pixel data, which facilitates its use as a draw source and also enables alpha blending.
    -   **copy\_to** - the target supports optimized copying to a draw destination and GFX should use the exposed `copy_to<>()` method when possible. If given the choice between using a draw source's `copy_to<>()` method and a draw destinations's `copy_from<>()` method, GFX will choose the `copy_from<>()` method, since there are more optimization opportunities with that.

Next, you have to declare the `using pixel_type` alias on your draw target. This is probably most often an alias for `gfx::rgb_pixel<16>` for color displays and `gfx::gsc_pixel<1>` for monochrome (1 bit grayscale) displays. It tells GFX what the native format of your draw object is.

-   `pixel_type` - indicates the native pixel format for this target

Now you can start implementing methods you'll need. Most of the methods return the `enum` `gfx::gfx_result` indicating the status of the operation.

First, aside from the `caps` and `pixel_type` aliases, there are methods you must implement regardless:

-   `dimensions()` - returns a `size16` that indicates the dimensions of the draw target.
-   `bounds()` - returns a `rect16` with a top left corner of (0,0) and a width and height equal to that of the draw target. This is an alias for `dimensions().bounds()`.

#### Draw Source Members

To implement a target as a draw source, you must additionally implement one, or possibly two methods:

-   `gfx_result point(point16 location, pixel_type* out_color)` - retrieves a pixel at the specified location
-   `gfx_result copy_to<typename Destination>(const rect16& src_rect,Destination& dst,point16 location)` - copies a portion of the target to the specified destination at the specified location

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

Where to Go From Here
---------------------

The demo projects that ship with this should provide you ample code to learn GFX or even build your own drivers. Currently, I'm focused on supporting the ESP32 via the ESP-IDF, but GFX itself is not limited by platform, and drivers can be written for anything - even DirectX on a Windows PC.

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

