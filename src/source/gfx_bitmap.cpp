#include "gfx_bitmap.hpp"
namespace gfx {
namespace helpers {
// DIRECT MODE PIXEL FORMAT CONVERSION SPECIALIZATIONS 
constexpr static const uint16_t xrgb16_r_left = 0;
constexpr static const uint16_t xrgb16_g_left = 5;
constexpr static const uint16_t xrgb16_b_left = 11;

uint32_t xrgba32_to_argb32p(uint32_t pixel) {
#ifndef HTCW_GFX_NO_SWAP
    uint32_t a = (pixel >> 24) & 0xFF;
    uint32_t b = (pixel >> 16) & 0xFF;
    uint32_t g = (pixel >> 8) & 0xFF;
    uint32_t r = (pixel >> 0) & 0xFF;
#else
    uint32_t r = (pixel >> 24) & 0xFF;
    uint32_t g = (pixel >> 16) & 0xFF;
    uint32_t b = (pixel >> 8) & 0xFF;
    uint32_t a = (pixel >> 0) & 0xFF;
#endif
    if (a != 255) {
        // Premultiply RGB channels by the alpha value
        r = (r * a) / 255;
        g = (g * a) / 255;
        b = (b * a) / 255;
    }

    // Return the ARGB Premultiplied pixel value
    return (a << 24) | (r << 16) | (g << 8) | b;
}
uint32_t xrgba32_to_argb32(uint32_t pixel) {
#ifndef HTCW_GFX_NO_SWAP
    uint32_t a = (pixel >> 24) & 0xFF;
    uint32_t b = (pixel >> 16) & 0xFF;
    uint32_t g = (pixel >> 8) & 0xFF;
    uint32_t r = (pixel >> 0) & 0xFF;
#else
    uint32_t r = (pixel >> 24) & 0xFF;
    uint32_t g = (pixel >> 16) & 0xFF;
    uint32_t b = (pixel >> 8) & 0xFF;
    uint32_t a = (pixel >> 0) & 0xFF;
#endif
    // if (a != 255) {
    //     // Premultiply RGB channels by the alpha value
    //     r = (r * a) / 255;
    //     g = (g * a) / 255;
    //     b = (b * a) / 255;
    // }

    // Return the ARGB Premultiplied pixel value
    return (a << 24) | (r << 16) | (g << 8) | b;
}
uint32_t xargb32p_to_rgba32(uint32_t pixel) {
    uint32_t a = (pixel >> 24) & 0xFF;
    uint32_t r = (pixel >> 16) & 0xFF;
    uint32_t g = (pixel >> 8) & 0xFF;
    uint32_t b = (pixel >> 0) & 0xFF;
    // if(a==0) return 0;
    // if (a != 255) {
    //     // Reverse the premultiplication to get the original RGB values
    //     r = (r * 255) / a;
    //     g = (g * 255) / a;
    //     b = (b * 255) / a;
    // }
    // Return the RGBA Plain pixel value
#ifndef HTCW_GFX_NO_SWAP
    return (a << 24) | (b << 16) | (g << 8) | r;
#else
    return (r << 24) | (g << 16) | (b << 8) | a;
#endif
}
uint32_t xrgb16_to_argb32p(uint16_t pixel) {
#ifndef HTCW_GFX_NO_SWAP
    pixel = ::bits::swap(pixel);
#endif
    uint32_t b = (pixel >> xrgb16_r_left) & 0x1F;
    uint32_t g = (pixel >> xrgb16_g_left) & 0x3F;
    uint32_t r = (pixel >> xrgb16_b_left) & 0x1F;
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;
    // Return the ARGB Premultiplied pixel value
    return (((uint32_t)255) << 24) | (r << 16) | (g << 8) | b;
}

uint16_t xargb32p_to_rgb16(uint32_t pxl) {
    pixel<
        channel_traits<channel_name::A,8,0,255,255>,
        channel_traits<channel_name::R,8>,
        channel_traits<channel_name::G,8>,
        channel_traits<channel_name::B,8>
    > px(pxl, true);
    rgb_pixel<16> px2;
    convert(px, &px2);
#ifndef HTCW_GFX_NO_SWAP
    return ::bits::swap(px2.native_value);
#else
    return px2.native_value;
#endif
}
uint16_t xargb32p_to_gsc8(uint32_t pixel) {
    //uint32_t a = (pixel >> 24) & 0xFF;
    uint32_t r = (pixel >> 16) & 0xFF;
    uint32_t g = (pixel >> 8) & 0xFF;
    uint32_t b = (pixel >> 0) & 0xFF;
    uint8_t v =255*((r * (1/255.f) * .299) +
                (g * (1/255.f) * .587) +
                (b * (1/255.f) * .114));
    return v;
}
uint32_t gsc_to_argb32p(uint8_t pixel) {
    uint32_t b = pixel;
    uint32_t g = pixel;
    uint32_t r = pixel;
    // Return the ARGB Premultiplied pixel value
    return (((uint32_t)255) << 24) | (r << 16) | (g << 8) | b;
}

// DIRECT MODE CALLBACK SPECIALIZATIONS
void xread_callback_rgba32p(uint32_t* buffer, const uint8_t* data,
                                int length) {
    // Assuming the surface format is RGBA Plain, fetch the pixel data
    const uint32_t* target = (const uint32_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each RGBA Plain pixel to ARGB Premultiplied
        buffer[i] = xrgba32_to_argb32p(target[i]);
    }
}
void xread_callback_rgba32(uint32_t* buffer, const uint8_t* data,
                                int length) {
    // Assuming the surface format is RGBA Plain, fetch the pixel data
    const uint32_t* target = (const uint32_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each RGBA Plain pixel to ARGB 
        buffer[i] = xrgba32_to_argb32(target[i]);
    }
}
void xread_callback_rgb16(uint32_t* buffer, const uint8_t* data,
                                int length) {
    // Assuming the surface format is RGBA Plain, fetch the pixel data
    const uint16_t* target = (const uint16_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each RGBA Plain pixel to ARGB Premultiplied
        buffer[i] = xrgb16_to_argb32p(target[i]);
    }
}
void xread_callback_gsc8(uint32_t* buffer, const uint8_t* data, int length) {
    // Assuming the surface format is GSC Plain, fetch the pixel data
    const uint8_t* target = (const uint8_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each GSC pixel to ARGB Premultiplied
        buffer[i] = gsc_to_argb32p(target[i]);
    }
}
    
void xwrite_callback_rgba32(uint8_t* data, const uint32_t* buffer,
                                int length) {
    // Assuming the surface format is RGBA Plain, write the pixel data
    uint32_t* target = (uint32_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each ARGB Premultiplied pixel to RGBA Plain before writing it
        // back
        target[i] = xargb32p_to_rgba32(buffer[i]);
    }
}

void xwrite_callback_rgb16(uint8_t* data, const uint32_t* buffer,
                                int length) {
    // Assuming the surface format is RGB565 Plain, write the pixel data
    uint16_t* target = (uint16_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each ARGB Premultiplied pixel to RGB565 Plain before writing
        // it back
        target[i] = xargb32p_to_rgb16(buffer[i]);
    }
}
void xwrite_callback_gsc8(uint8_t* data, const uint32_t* buffer,
                                int length) {
    // Assuming the surface format is RGB565 Plain, write the pixel data
    uint8_t* target = (uint8_t*)data;
    for (int i = 0; i < length; ++i) {
        // Convert each ARGB Premultiplied pixel to RGB565 Plain before writing
        // it back
        target[i] = xargb32p_to_gsc8(buffer[i]);
    }
}
}
}
