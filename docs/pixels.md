#### [← Back to index](index.md)

<a name="1"></a>

# 1. Pixels

Pixels in GFX are diverse elements for representing colors, color models, and binary formats. They are such a rich and utilitarian feature of GFX they deserve a thorough treatment. Many of the features of GFX are facilitated through the use of different pixel formats. In addition pixels contain a myriad of information allowing you to query for almost any pixel related information you can imagine. Despite their features, pixels are compiled into high performance instructions wherever possible, and it's possible a lot. In cases where a pixel is constant, it will resolve to a scalar literal value.

A pixel has a bit depth which is the sum of the bit depths of each [channel](#1.2). 

A pixel has a raw value of at most the maximum machine word size (including compound words) that can be represented either as a big endian or a platform specific value. You use `value()` to get the big endian value and the `native_value` field to get the platform ordered value. The type of the value is the smallest unsigned integer type that can hold the a value of the computed bit depth.

```cpp
// get the pixel value in big endian form
auto v = rgb888.value();
// get the pixel in platform endian form
auto v = rgb888.native_value;
// set the value (can also use value(#num))
rgb888.native_value=0xFFFFFF;
```

<a name="1.1"></a>

## 1.1 Declaring common pixels

While there is a way to declare arbitrary and custom pixel formats, there is a shorthand for pixels of known [color models](#1.3). We'll cover declaring them now.

You can declare an RGB pixel using `rgb_pixel<N>` where `N` is the bit depth of the pixel - that is, the number of bits required to hold it. This will declare a pixel with 3 channels named R, G and B, respectively. The bits are divided evenly between each of the channels, with any remainder being allocated to the G (green) channel.

A grayscale pixel can be declared using `gsc_pixel<N>` where `N` again is the bit depth. This will declare a pixel with a single channel named L (luminosity).

``rgba_pixel<N>`` declares an RGB pixel with an [alpha channel](#1.5) that can be used for [alpha blending](drawing.md#5.6).

There are also shorthands for yuv, ycbcr (used by JPG), hsl, hsv, cmyk, and indexed pixels.

<a name="1.2"></a>

## 1.2 Channels

A pixel may be made up of multiple *channels*. A channel is one element of a pixel. The most common pixel format is probably an RGB pixel. RGB pixels contain 3 channels, one for red (R), one for green (G) and one for blue (B). Together these *locate a pixel within a color space*. What that means is they define the color of the pixel in the given color model, which we'll be covering next.

Each channel may be a specified bit depth, and has corresponding integer and real number types that represent the value of that channel. Larger bit-depths use larger integer and real types. The real number accessors scale the channel value to a real/floating point number between 0.0 and 1.0, inclusive. Each channel also potentially has a custom minimum, maximum and default value. Furthermore, each channel has a corresponding index and name, either of which can be used to reference it. Typically, you manipulate color values by manipulating individual channels.

You can access channel values like this:
```cpp
// set channel by index
rgb888.template channel<0>(255); // max
// set channel by name
rgb888.template channel<channel_name::G>(127);
// set channel real value
rgb888.template channelr<2>(1.0); // max

// get red as an int type
uint8_t r = rgb888.template channel<channel_name::R>();
// get green as a real type
float g = rgb888.template channelr<channel_name::G>();
// get blue as an int type
uint8_t b = rgb888.template channel<channel_name::B>();
```
Sometimes the compiler will complain because it can't guarantee that a particular channel exists within a given context. In order to circumvent this you can use ``channel_unchecked<#index>([#value])`` which will return or set a channel, or it will retrieve a special "null channel" with a bit depth of zero when the channel could not be found. A null channel does nothing to the pixel data.

<a name="1.3"></a>

## 1.3 Colors

GFX provides a palette of named X11 colors to choose from. These colors are available in any pixel format except for indexed pixels, converting to the appropriate format (such as grayscale or YUV).

You can access a psuedo enumeration of named colors like this:
```cpp
// get the color enumeration in the
// 16-bit RGB format
using color16 = color<rgb_pixel<16>>;

rgb_pixel<16> px = color16::old_lace;
rgb_pixel<16> px2 = color16::red;
```

You can also get an RGB or RGBA color using one of several shorthands:
```cpp
// get the color in largest RGB pixel the machine can support
auto px = color_max::steel_blue;
// get the color in 16-bit RGB format
auto px2 = color16::old_lace;
// get the color in 24-bit RGB format
auto px3 = color24::brown;
// get the color in 32-bit RGBA format
auto px4 = color32::yellow;
```

Note that when you draw (as in [section 5](drawing.md)) you can pass any type of pixel - bit depth and color model conversions will be automatically applied as necessary. If the draw destination is indexed, color matching will occur.

<a name="1.4"></a>

## 1.4 Color models

A color model is a particular representation of a color. It identifies the "color space" map that the color appears on. Think of a color space as a sort of color wheel in which a color is located. RGB is probably the most common color model, but there is also grayscale, YUV, YCbCr, HSL, HSV, CMYK and potentially more. When necessary GFX identifies the color model of your pixel by the names of the channels it contains, disregarding the order of the channels, and ignoring the alpha channel. You can use the [metadata](#1.9) template "function" `has_channel_names<{channel name ","}>` to determine the color model.

<a name="1.5"></a>

## 1.5 The alpha channel

Declaring a pixel with an alpha channel, as described in [section 1.1](#1.1) enables alpha blending on supporting [draw targets](draw_targets.md). This basically allows colors underneath where you're drawing the "bleed through" so that you can see them underneath, like using a highlighter pen. In order to alpha blend, the draw target must be both a *draw destination* to facilitate the actual drawing operation, and a *draw source* to enable reading the pixel data back in order to blend it. We cover the actual process of drawing with these pixels in [section 5.6](drawing.md#5.6)

<a name="1.6"></a>

## 1.6 Indexed pixels

Historically older PC displays such as EGA used an indexed color scheme in order to use a smaller frame buffer. Indexed colors are basically a palette of colors selected from a much larger palette, such as a selection of palette of 256 colors drawn from a palette of 262,144, as was common with VGA and SVGA. While it's not common to see TFT/LCD/OLED displays with indexed color schemes on IoT these days, with color e-paper displays it's universal. This is because e-paper displays cannot blend different primary colors into new colors, leaving a small amount of fixed colors to choose from - at most 7, including black and white - that I've seen.

Indexed pixels must have an associated [draw target](draw_targets.md) in order to be converted to another pixel type. The reason is that there is simply not enough information to do this conversion without access to the associated palette mapping (palette). This palette is held by the draw target.

When converting from a non-indexed pixel type to an indexed pixel type using a given draw target, automatic color matching will occur, such that if the display's palette has a bright red color, similar colors, such as dark red, will be represented with it. This allows you to do things like load color JPGs onto devices with indexed colors.

<a name="1.7"></a>

## 1.7 Declaring custom pixels

Sometimes the shorthand declarations for common pixels will not be detailed enough to represent your precise pixel. A good example might be a 7-color e-paper display. Such a display may use 4 bits per pixel, but valid values are only zero through six. You can define this using *channel traits*, which declare the properties for a channel. You feed one or more channel traits into a pixel definition in order to declare the channels.

```cpp
// declare a 4-bit 7-color pixel
// parameters: name, index, bit depth, [min], [max], [default], [scale]
using index7 = gfx::pixel<gfx::channel_traits<gfx::channel_name::index,4,0,6>>;


// declare a 16-bit RGB pixel - rgb_pixel<16> is shorthand
// for this:
using rgb565 = pixel<channel_traits<channel_name::R,5>,
                    channel_traits<channel_name::G,6>,
                    channel_traits<channel_name::B,5>>;
```

<a name="1.8"></a>

## 1.8 Converting pixels

Although GFX typically will convert pixels from one format to another during things such as drawing operations, sometimes it might be desirable or even necessary to manually and explicitly convert pixels from one format to another.

For all pixels except indexed pixels you can use the `convert<>()` method to convert pixel types. It takes a source pixel, a pointer to a destination pixel, and a pointer to an optional background color for blending when the source pixel has an alpha channel. Bit depths and color models are converted, irrespective of channel order, so a BGR pixel can become RGB. When converting a pixel with an alpha channel to one without, if no background color was specified the alpha channel is simply discarded, rendering the pixel opaque.

For indexed pixels you have `convert_palette_from<>()` to convert from a non-indexed pixel to an indexed pixel using the indicated draw target's palette, or `convert_palette_to<>()` to convert from an indexed pixel to a non-indexed pixel, again using the draw target's palette.

<a name="1.9"></a>

## 1.9 Metadata

Pixels and channels contain a host of metadata about the element in question. This is accessible of the type for the pixel or the channel.

You shouldn't need this information most of the time, so in the interest of brevity, see the comments in the code in *gfx_pixel.hpp* for the individual functions. Editors like VS Code will report these comments automatically.

[→ Draw targets](draw_targets.md)

[← Index](index.md)

