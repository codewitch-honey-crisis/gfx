#### [← Back to index](index.md)

<a name="2"></a>

# Draw Targets

A draw target is an abstract concept that consumes or produces pixel data. Rather than inherit from a base class, they report which operations they support, and GFX will call the appropriate method. Usually you do not interact with instances of these classes. Rather, GFX operates them on your behalf, typically via you using `draw` class. For more on the `draw` class, see [section 5](drawing.md).


<a name="2.1"></a>

## 2.1 Draw sources and destinations

A *draw source* is a type of draw target that produces pixel data for downstream consumers. A *draw destination* consumes pixel information. It might be helpful to think of it as a kind of "canvas" on which drawing operations can take place. Many targets are both draw sources and draw destinations. Only those such targets are capable of alpha blending.

<a name="2.1"></a>

## 2.2 Bitmaps

Bitmaps are sort of all-purpose draw source/destinations. In essence they allow you to hold drawn data around in memory, which you can then draw somewhere else. Like all draw targets, a bitmap's type signature includes the pixel type, and possibly the associated palette type.  There are several types of bitmaps for different situations that we'll expore below.

<a name="2.2.1"></a>

### 2.2.1 Standard bitmaps

A standard bitmap is the most common type and can be read from or written to very efficiently. It keeps its data in memory. Bitmaps do not hold their own memory. This is because not all memory is created equal. On some platforms for example, in order to enable a bitmap to be sent to the driver, the bitmap data must be stored in DMA capable RAM. The other reason is so you can recycle buffers. When you're done with a bitmap you can reuse the memory for another bitmap (as long as it's big enough) without deallocating or reallocating. The disadvantage is a small amount of increased code complexity wherein you must determine the size you need for the bitmap, and then allocate the memory for it yourself, freeing it yourself later. There are however, the convenience methods `create_bitmap<>()` and `create_bitmap_from<>()` for creating them, but you'll still have to call `if(bmp.begin()!=nullptr) free(bmp.begin());` yourself.

In addition to being part of the type signature, the pixel type determines the binary footprint of the memory buffer that holds the pixels. For example `bitmap<rgb_pixel<16>>` would declare a bitmap type that can hold pixels in 16-bit RGB format. In this scenario there is one pixel every 2 bytes, and like all bitmaps, pixels are ordered from top to bottom, left to right within the memory buffer.

Manually declaring a bitmap and an associated buffer looks like this:

```cpp
using bmp_type = bitmap<rgb_pixel<16>>;
constexpr static const size16 bmp_size(16,16);
uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
bmp_type bmp(bmp_size,bmp_buf);
// you can now draw to and from bmp, a 16-bit RGB
// bitmap of size 16x16 pixels.
```
Using the convenience methods it looks like this
```cpp
// create from a draw target "lcd"
// especially useful when indexed/paletted
// targets are in use
auto bmp = create_bitmap_from(lcd,{64,64});
...
if(bmp.begin()!=nullptr) free(bmp.begin());

// or...
// create a bitmap with the specified pixel type
// and optional palette type
auto bmp = create_bitmap<rgb_pixel<16>>({64,64});
...
if(bmp.begin()!=nullptr) free(bmp.begin());
```

<a name="2.2.2"></a>

### 2.2.2 Const bitmaps

A const bitmap operates like a standard bitmap except there are no draw destination methods and all the other members are const. This facilitates using a const buffer, such as an array stored in program flash space on an MCU, as the bitmap data. This is one of the few targets that is a draw source only.

Declaring one is similar to declaring a bitmap as above:

```cpp
using bmp_type = const_bitmap<rgb_pixel<16>>;
// the rest of the code is the same as bitmap<>
```

<a name="2.2.3"></a>

### 2.2.3 Large bitmaps

On little devices there's not a lot of heap, and while it may seem like you should be able to load a 160kB frame buffer on a 512kB system, there's the inconvenient issue of heap fragmentation. Heap fragmentation causes there to not be 160kB of *contiguous* free space anywhere on the heap in many situations because of past deallocations, even if it's the first allocation you make after the RTOS passes you control because the heap is already "dirty" and fragmented by then. What do we do?

In GFX large bitmaps can created using the `large_bitmap<>` template class. It is a composition of a lot of smaller bitmaps such that it presents a facade of one unified draw source/destination. Using this is pretty much the same as the regular bitmap as far as GFX is concerned, even though it doesn't have all the members that standard bitmaps do.

Create it like a normal bitmap except you need to pass in the segment height, in lines as the second parameter. A large bitmap is composed of a bunch of smaller bitmaps (referred to as segments) of the same width stacked vertically. Therefore, each segment is a number of lines high. Every segment but the last one (which may be smaller) has the same number of lines. Unlike normal bitmaps, you do not allocate your own memory for this but you can use custom allocator and deallocator functions if you need special platform specific heap options. Declaring them looks like this:
```cpp
constexpr static const size16 bmp_size(320,240);
using bmp_type = large_bitmap<rgb_pixel<16>>;
bmp_type bmp(bmp_size,1);
```

<a name="2.3"></a>

## 2.3 Drivers

A driver operates pretty much like a bitmap does in terms of how you use it, although only some drivers are draw sources. However, how you declare and instantiate them is quite different, and varies from driver to driver. Note that some drivers have features specific to the driver that you must access off the driver instance itself instead of using it through GFX. An example might be "sleeping" an e-paper display with a `sleep()` method.

Drivers are not included with GFX. See the documentation and/or demo code for your driver to determine how to declare and instantiate it.

<a name="2.4"></a>

## 2.4 Viewports

A viewport allows you to rotate or offset drawing operations. You can specify an offset, a center point, and a rotation angle in degrees and any draw operation targeting that viewport will be transformed based on how you configured it. They are useful for doing things like drawing text at an angle. You have to be careful how you configure it to get the center correct for your rotation. When you instantiate it, you give it an existing draw target like a bitmap or a driver, and then set the transformation configuration for subsequent draws:
```cpp
viewport<bmp_type> view(bmp);
view.rotation(90);    
view.offset({45,5});
// now draw
srect16 sr = view.translate(textsz.bounds());
sr = sr.clamp_top_left_to_at_least_zero();
// you can now draw the text with something like
// draw::text(view, sr, {0,0}, text, fnt, scale, color_max::white);
// and it will be rotated 90 degrees clockwise and offset.
```

<a name="2.5"></a>

## 2.5 Sprites

A sprite has both a bitmap and an associated monochrome bitmap mask draw target. The sprite acts as a draw source only so it must be initialized with a pre-drawn bitmap and mask. Sprites have a `bool hit_test(point16) const` function which will use the mask to determine if a point intersects with the visible portion of the bitmap - that is a portion of the bitmap whose mask area is set. The mask also determines which part of the bitmap is actually drawn and which is not. The bitmap and mask must be the same size or the results are undefined. Declaring one is something like follows:

```cpp
constexpr static const size16 sprite_size(16,16);
// declare a monochrome bitmap.
using mask_type = bitmap<gsc_pixel<1>>;
// declare a color bitmap
using bmp_type = bitmap<rgb_pixel<16>>;
// instantiate the bitmaps and 
// draw the mask to the mask bitmap,
// and the sprite to the bmp bitmap
... 
// declare the sprite:
using sprite_type = sprite<rgb_pixel<16>>;
sprite_type sprite(sprite_size,bmp.begin(),mask_bmp.begin());
```
You can now use the sprite with `draw::sprite<>()`.

<a name="2.6"></a>

## 2.6 Custom

In addition to the above, you can create your own draw targets by creating a class with some key members. Most of the members are optional to implement, and are made available for performance reasons. The core set of required members is minimal, but with performance to match.

<a name="2.6.1"></a>

### 2.6.1 Common members

All draw targets have a core collection of members needed define them.

The first is the pixel_type alias which is used to express the native pixel type of the draw target. Declare it as follows:
```cpp
// declare a 16-bit RGB native pixel type
using pixel_type = rgb_pixel<16>;
```
If the pixel type is an indexed type, the target must expose the following:
```cpp
// declares the palette to be of type mypalette
using palette_type = mypalette;
// retrieves the instance of the palette
// (normally const)
const palette_type* palette() const;
```

A draw target also exports metrics that determine its dimensions and bounds:
```cpp
// retrieves the dimensions of the draw target
size16 dimensions() const;
// retrieves the bounding rectangle anchored to (0,0)
rect16 bounds() const;
```

If your target supports asynchronous operations you must also provide
```cpp
gfx_result wait_all_async();
```
This waits for all pending asynchronous operations to complete. We'll cover async below.

<a name="2.6.2"></a>

### 2.6.2 Capabilities

Finally, a draw target must expose a `gfx_caps<>` instantiation to tell GFX the target's capabilities.

It looks something like this
```cpp
using caps = gfx::gfx_caps<false,true,true,true,false,true,true>;
```

Those `bool` values will vary depending on what the target supports. In left to right order they are `blt`, `async`, `batch`, `copy_from`, `suspend`, `read` and `copy_to`. "Blt" indicates that the target provides direct access to its in memory pixel buffer. "Async" indicates the availability of asynchronous counterparts to the target's operations. "Batch" indicates the use of batch windowing to increase performance. "Copy from" indicates that this is a draw destination that can do high performance copies from a draw source. "Suspend" indicates that this draw destination supports double buffering by suspending drawing operations. "Read" indicates that this is a draw source. "Copy to" indicates that this draw source supports high performance copies of its pixel data to a draw destination. We'll be covering the specific members associated with these capabilities as we continue.

Also, we still don't have members for producing and consuming pixel data, but we'll cover those now.

<a name="2.6.3"></a>

### 2.6.3 Draw source members

To be a draw source, that is, a producer of pixels, first your `caps` must have at least `read` and possibly `copy_to` set to `true`.

Once you do that, you must implement at least the following method:
```cpp
gfx_result point(point16 location, pixel_type* out_pixel) const;
```
This method retrieves the pixel at the given location. Or `success` and an empty/default pixel if the `location` was out of bounds.

If you selected `copy_to` in the `caps` you must also implement the following:
```cpp
template<typename Destination>
gfx_result copy_to(const rect16& src_rect,Destination& dst,point16 location) const
```
This copies pixel data from the window indicated by `src_rect` to the draw destination `dst` at the given `location` within the draw destination. This method should - using the available parameters and respective capabilities of the draw targets - choose the fastest available synchronous way to transfer data possible.

If you selected `async` in your `caps` as well as `copy_to` you must also implement `copy_to_async` with the same signature. It should attempt to perform the copy operation asynchronously but can do so synchronously if asynchronous execution isn't possible.

If you selected `blt` in your caps you will also have to expose two methods to retrieve the beginning and just past the end of your target's pixel data memory:
```cpp
uint8_t* begin(); // can be const
uint8_t* end(); // can be const
```

<a name="2.6.4"></a>

### 2.6.4 Draw destination members

For performance reasons, draw destinations can expose quite a few more members than draw sources. At their simplest however, they need just a few members specific to draw destinations:

```cpp
// draws a single pixel of the specified color at the location
// returns `success` without drawing if the pixel is out 
// of bounds
gfx_result point(point16 location, pixel_type color);
// and if async is specified
gfx_result point_async(point16 location, pixel_type color);

// fills the specified rectangular region with the specified
// color. Out of bounds areas are clipped.
gfx_result fill(const rect16& bounds, pixel_type color);
// and if async is specified:
gfx_result fill_async(const rect16& bounds, pixel_type color);

// clears the specified rectangular region
gfx_result clear(const rect16& bounds);
// and if async is specified:
gfx_result clear_async(const rect16& bounds);
```
Beyond those, there may be additional members depending on the caps.

If you selected blt you must provide `begin()` and `end()` accessors to gain access to the beginning and just past the end of your pixel memory buffer:
```cpp
uint8_t* begin();
uint8_t* end();
```

If you selected `batch` you must allow for setting a destination writing window, writing out pixels to that window in order from left to right, top to bottom, and then committing that batch operation when all the pixel data has been written.

You'll be declaring the following methods:
```cpp
// begins a batch operation targeting the specified window.
gfx_result begin_batch(const rect16& rect);
// and if async is specified
gfx_result begin_batch_async(const rect16& rect);

// writes a pixel to the current batch. Pixels are written 
// to the batch window in order from left to right, top
// to bottom. If your pixel buffer gets full, it's 
// acceptable to send as needed. Batch isn't suspend, it's
//  just a way to cut down traffic.
gfx_result write_batch(pixel_type color);
// and if async is specified
gfx_result write_batch_async(pixel_type color);

// writes any remaining batched data out and reverts from
// batching mode.
gfx_result commit_batch();
// and if async is specified
gfx_result commit_batch_async();
```

If you selected `copy_from` in your caps you must provide a performant way to copy pixel data from a draw source to your draw destination. This is basically the corollary to `copy_to` for draw destinations.

```cpp
// copies from a draw source src window as specified by 
// src_rect to the specified location within this draw 
// destination.
template<typename Source>
gfx_result copy_from(const rect16& src_rect,const Source& src,point16 location);
// and if async is specified
template<typename Source>
gfx_result copy_from_async(const rect16& src_rect,const Source& src,point16 location);

```

If you selected `suspend` you must be able to suspend all drawing output until resumed. Suspend and resume calls should balance. Typically on the last resume call you will update the display with the frame buffer data.

You must implement the following:
```cpp
// suspends drawing. Suspends are counted and balanced, 
// so for every suspend call, you must make a corresponding 
// resume() call.
gfx_result suspend()
// and if async is specified
gfx_result suspend_async()

// resumes drawing. Will force a resume despite how
// many times suspend was called if force = true
gfx_result resume(bool force=false)
// and if async is specified
gfx_result resume_async(bool force=false)
```

<a name="2.6.5"></a>

### 2.6.5 Initialization

If a draw destination requires initialization, it should be done the first time any drawing operation takes place. Draw source members may return `gfx_result::invalid_state` if not initialized.


[→ Images](images.md)

[← Pixels](pixels.md)

