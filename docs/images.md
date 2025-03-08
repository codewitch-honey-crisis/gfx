#### [← Back to index](index.md)

<a name="3"></a>

# 3. Images

Images allow you to load potentially large picture/image data iteratively onto a draw target. They are typically streamed from a source, such as an SD card, or flash memory. Currently the only image format supported is JPG.

Images aren't draw sources because they are compressed and because they are never loaded into memory all at once. Due to that, there is no way to randomly access the pixel data, which is a requirement for draw sources.

Instead, you are called back with bitmaps representing small portions of the image at a time. It's at this point you can draw that portion to a destination.

It should be noted that the `draw` class can handle the callbacks for you ([section 5.4](drawing.md#5.4)), so you don't have to use the method that is about to be described to load an image, but it can be useful to use if you intend to do post-processing on the image:

To load an image you start by passing the stream to the image loader function along with a callback (I prefer to use a "flat" lambda for this) that handles the incremental loading. You'll be called back multiple times, again, each time with a portion of the image as a bitmap, along with a location where it belongs within the image, and any state you passed along to the `load()` function. Note that to reduce overhead, a state variable is used to pass state instead of using a functor like `std::function<>`. As I do, you can use a "flat" lambda that decays to a simple function pointer, and then pass your class pointer in as the state argument, to be reconstituted inside your callback. Often times, you won't even need a state argument because everything you're after, such as the display itself, is available globally:
```cpp
// this line may vary for different platforms
// like arduino:
file_stream fs("/spiffs/image.jpg");
// TODO: check caps().read to see if the file is opened/readable
jpeg_image::load(&fs,[](size16 dimensions,
                        typename jpeg_image::region_type& region,
                        point16 location,
                        void* state){
    // use draw:: to render this portion to the display
    return draw::bitmap(lcd, // lcd is available globally
                        srect16((spoint16)location,
                                (ssize16)region.dimensions()),
                                region,region.bounds());
// we don't need state, so just use nullptr
},nullptr);
```
Additionally, under Arduino you can pass a File object:
```cpp
File file = SPIFFS.open("/image.jpg","rb");
gfx::draw::image(lcd,lcd.bounds(),&file);
file.close();
```
If you don't know the dimensions you can pass `size16(-1,-1)`.

You probably won't understand some of the calls in this as they will covered in sections [5](drawing.md) and [6](positioning.md), but it should give you a general idea of what is being covered.

Note that these bitmaps have a pixel type that is reflective of the native pixel format of the image. For JPG that's YCbCr, not RGB. Fortunately `draw` handles that conversion automatically.


[→ Fonts](fonts.md)

[← Draw targets](draw_targets.md)

