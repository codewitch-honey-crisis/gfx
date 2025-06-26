#include <assert.h>
#include <limits.h>

#include <gfx_core.hpp>
#include <gfx_pixel.hpp>
#include <gfx_positioning.hpp>

#include "plutovg-private.h"
#include "plutovg-utils.h"
#define COLOR_TABLE_SIZE 64
typedef struct {
    ::gfx::matrix matrix;
    plutovg_spread_method_t spread;
    uint32_t colortable[COLOR_TABLE_SIZE];
    union {
        struct {
            float x1, y1;
            float x2, y2;
        } linear;
        struct {
            float cx, cy, cr;
            float fx, fy, fr;
        } radial;
    } values;
} gradient_data_t;

typedef struct {
    ::gfx::matrix matrix;
    const ::gfx::const_blt_span* direct;
    ::gfx::on_direct_read_callback_type on_direct_read;
    ::gfx::vector_on_read_callback_type on_read;
    void* on_read_state;
    int width;
    int height;
    int const_alpha;
} texture_data_t;

typedef struct {
    float dx;
    float dy;
    float l;
    float off;
} linear_gradient_values_t;

typedef struct {
    float dx;
    float dy;
    float dr;
    float sqrfr;
    float a;
    float inv2a;
    int extended;
} radial_gradient_values_t;
static inline void color_to_color(plutovg_color_t* color,
                                  ::gfx::vector_pixel* out_color) {
    *out_color = ::gfx::vector_pixel(color->r, color->g, color->b, color->a);
}
static inline uint32_t combine_color_with_opacity(const plutovg_color_t* color,
                                                  float opacity) {
    uint32_t a = lroundf(color->a * opacity * 255);
    uint32_t r = lroundf(color->r * 255);
    uint32_t g = lroundf(color->g * 255);
    uint32_t b = lroundf(color->b * 255);
    return (a << 24) | (r << 16) | (g << 8) | (b);
}

static inline uint32_t premultiply_color_with_opacity(
    const plutovg_color_t* color, float opacity) {
    uint32_t alpha = lroundf(color->a * opacity * 255);
    uint32_t pr = lroundf(color->r * alpha);
    uint32_t pg = lroundf(color->g * alpha);
    uint32_t pb = lroundf(color->b * alpha);
    return (alpha << 24) | (pr << 16) | (pg << 8) | (pb);
}

static inline uint32_t interpolate_pixel(uint32_t x, uint32_t a, uint32_t y,
                                         uint32_t b) {
    uint32_t t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;
    x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

static inline uint32_t BYTE_MUL(uint32_t x, uint32_t a) {
    uint32_t t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;
    x = ((x >> 8) & 0xff00ff) * a;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

#ifdef __SSE2__

#include <emmintrin.h>

static void memfill32(uint32_t* dest, uint32_t value, int length) {
    __m128i vector_data = _mm_set_epi32(value, value, value, value);
    while (length && ((uintptr_t)dest & 0xf)) {
        *dest++ = value;
        length--;
    }

    while (length >= 32) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);
        _mm_store_si128((__m128i*)(dest + 8), vector_data);
        _mm_store_si128((__m128i*)(dest + 12), vector_data);
        _mm_store_si128((__m128i*)(dest + 16), vector_data);
        _mm_store_si128((__m128i*)(dest + 20), vector_data);
        _mm_store_si128((__m128i*)(dest + 24), vector_data);
        _mm_store_si128((__m128i*)(dest + 28), vector_data);

        dest += 32;
        length -= 32;
    }

    if (length >= 16) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);
        _mm_store_si128((__m128i*)(dest + 8), vector_data);
        _mm_store_si128((__m128i*)(dest + 12), vector_data);

        dest += 16;
        length -= 16;
    }

    if (length >= 8) {
        _mm_store_si128((__m128i*)(dest), vector_data);
        _mm_store_si128((__m128i*)(dest + 4), vector_data);

        dest += 8;
        length -= 8;
    }

    if (length >= 4) {
        _mm_store_si128((__m128i*)(dest), vector_data);

        dest += 4;
        length -= 4;
    }

    while (length) {
        *dest++ = value;
        length--;
    }
}

#else

static inline void memfill32(uint32_t* dest, uint32_t value, int length) {
    while (length--) {
        *dest++ = value;
    }
}

#endif  // __SSE2__

static inline int gradient_clamp(const gradient_data_t* gradient, int ipos) {
    if (gradient->spread == PLUTOVG_SPREAD_METHOD_REPEAT) {
        ipos = ipos % COLOR_TABLE_SIZE;
        ipos = ipos < 0 ? COLOR_TABLE_SIZE + ipos : ipos;
    } else if (gradient->spread == PLUTOVG_SPREAD_METHOD_REFLECT) {
        const int limit = COLOR_TABLE_SIZE * 2;
        ipos = ipos % limit;
        ipos = ipos < 0 ? limit + ipos : ipos;
        ipos = ipos >= COLOR_TABLE_SIZE ? limit - 1 - ipos : ipos;
    } else {
        if (ipos < 0) {
            ipos = 0;
        } else if (ipos >= COLOR_TABLE_SIZE) {
            ipos = COLOR_TABLE_SIZE - 1;
        }
    }

    return ipos;
}

#define FIXPT_BITS 8
#define FIXPT_SIZE (1 << FIXPT_BITS)
static inline uint32_t gradient_pixel_fixed(const gradient_data_t* gradient,
                                            int fixed_pos) {
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return gradient->colortable[gradient_clamp(gradient, ipos)];
}

static inline uint32_t gradient_pixel(const gradient_data_t* gradient,
                                      float pos) {
    int ipos = (int)(pos * (COLOR_TABLE_SIZE - 1) + 0.5f);
    return gradient->colortable[gradient_clamp(gradient, ipos)];
}

static void fetch_linear_gradient(uint32_t* buffer,
                                  const linear_gradient_values_t* v,
                                  const gradient_data_t* gradient, int y, int x,
                                  int length) {
    float t, inc;
    float rx = 0, ry = 0;

    if (v->l == 0.f) {
        t = inc = 0;
    } else {
        rx = gradient->matrix.c * (y + 0.5f) + gradient->matrix.a * (x + 0.5f) +
             gradient->matrix.e;
        ry = gradient->matrix.d * (y + 0.5f) + gradient->matrix.b * (x + 0.5f) +
             gradient->matrix.f;
        t = v->dx * rx + v->dy * ry + v->off;
        inc = v->dx * gradient->matrix.a + v->dy * gradient->matrix.b;
        t *= (COLOR_TABLE_SIZE - 1);
        inc *= (COLOR_TABLE_SIZE - 1);
    }

    const uint32_t* end = buffer + length;
    if (inc > -1e-5f && inc < 1e-5f) {
        memfill32(buffer, gradient_pixel_fixed(gradient, (int)(t * FIXPT_SIZE)),
                  length);
    } else {
        if (t + inc * length < (float)(INT_MAX >> (FIXPT_BITS + 1)) &&
            t + inc * length > (float)(INT_MIN >> (FIXPT_BITS + 1))) {
            int t_fixed = (int)(t * FIXPT_SIZE);
            int inc_fixed = (int)(inc * FIXPT_SIZE);
            while (buffer < end) {
                *buffer = gradient_pixel_fixed(gradient, t_fixed);
                t_fixed += inc_fixed;
                ++buffer;
            }
        } else {
            while (buffer < end) {
                *buffer = gradient_pixel(gradient, t / COLOR_TABLE_SIZE);
                t += inc;
                ++buffer;
            }
        }
    }
}

static void fetch_radial_gradient(uint32_t* buffer,
                                  const radial_gradient_values_t* v,
                                  const gradient_data_t* gradient, int y, int x,
                                  int length) {
    if (v->a == 0.f) {
        memfill32(buffer, 0, length);
        return;
    }

    float rx = gradient->matrix.c * (y + 0.5f) + gradient->matrix.e +
               gradient->matrix.a * (x + 0.5f);
    float ry = gradient->matrix.d * (y + 0.5f) + gradient->matrix.f +
               gradient->matrix.b * (x + 0.5f);
    rx -= gradient->values.radial.fx;
    ry -= gradient->values.radial.fy;

    float inv_a = 1.f / (2.f * v->a);
    float delta_rx = gradient->matrix.a;
    float delta_ry = gradient->matrix.b;

    float b =
        2 * (v->dr * gradient->values.radial.fr + rx * v->dx + ry * v->dy);
    float delta_b = 2 * (delta_rx * v->dx + delta_ry * v->dy);
    float b_delta_b = 2 * b * delta_b;
    float delta_b_delta_b = 2 * delta_b * delta_b;

    float bb = b * b;
    float delta_bb = delta_b * delta_b;

    b *= inv_a;
    delta_b *= inv_a;

    float rxrxryry = rx * rx + ry * ry;
    float delta_rxrxryry = delta_rx * delta_rx + delta_ry * delta_ry;
    float rx_plus_ry = 2 * (rx * delta_rx + ry * delta_ry);
    float delta_rx_plus_ry = 2 * delta_rxrxryry;

    inv_a *= inv_a;

    float det = (bb - 4 * v->a * (v->sqrfr - rxrxryry)) * inv_a;
    float delta_det =
        (b_delta_b + delta_bb + 4 * v->a * (rx_plus_ry + delta_rxrxryry)) *
        inv_a;
    float delta_delta_det =
        (delta_b_delta_b + 4 * v->a * delta_rx_plus_ry) * inv_a;

    const uint32_t* end = buffer + length;
    if (v->extended) {
        while (buffer < end) {
            uint32_t result = 0;
            if (det >= 0) {
                float w = sqrtf(det) - b;
                if (gradient->values.radial.fr + v->dr * w >= 0) {
                    result = gradient_pixel(gradient, w);
                }
            }

            *buffer = result;
            det += delta_det;
            delta_det += delta_delta_det;
            b += delta_b;
            ++buffer;
        }
    } else {
        while (buffer < end) {
            *buffer++ = gradient_pixel(gradient, sqrtf(det) - b);
            det += delta_det;
            delta_det += delta_delta_det;
            b += delta_b;
        }
    }
}
struct composition_params {
    plutovg_canvas_t* canvas;
    const uint32_t* src;
    int x;
    int y;
    size_t length;
    uint32_t color;
    uint32_t const_alpha;
    uint8_t* direct;
};

static void plutovg_memfill32(uint32_t* dest, unsigned int value, int length) {
    while (length--) {
        *dest++ = value;
    }
}

static void composition_solid_source(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;
    if (params.const_alpha == 255) {
        plutovg_memfill32(dest, params.color, params.length);
    } else {
        uint32_t ialpha = 255 - params.const_alpha;
        uint32_t color = BYTE_MUL(params.color, params.const_alpha);
        for (size_t i = 0; i < params.length; i++) {
            dest[i] = color + BYTE_MUL(dest[i], ialpha);
        }
    }
}

static void composition_solid_source_over(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;

    uint32_t color = params.color;
    if (params.const_alpha != 255) color = BYTE_MUL(color, params.const_alpha);
    uint32_t ialpha = 255 - plutovg_alpha(color);
    for (size_t i = 0; i < params.length; i++) {
        dest[i] = color + BYTE_MUL(dest[i], ialpha);
    }
}

static void composition_solid_destination_in(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;

    uint32_t color = params.color;
    uint32_t a = plutovg_alpha(color);
    if (params.const_alpha != 255)
        a = BYTE_MUL(a, params.const_alpha) + 255 - params.const_alpha;
    for (size_t i = 0; i < params.length; i++) {
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

static void composition_solid_destination_out(
    const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;

    uint32_t color = params.color;
    uint32_t a = plutovg_alpha(~color);
    if (params.const_alpha != 255)
        a = BYTE_MUL(a, params.const_alpha) + 255 - params.const_alpha;
    for (size_t i = 0; i < params.length; i++) {
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

static void composition_source(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;

    if (params.const_alpha == 255) {
        for (size_t i = 0; i < params.length; ++i) {
            dest[i] = params.src[i];
        }
    } else {
        uint32_t ialpha = 255 - params.const_alpha;
        for (size_t i = 0; i < params.length; i++) {
            dest[i] = interpolate_pixel(params.src[i], params.const_alpha,
                                        dest[i], ialpha);
        }
    }
}

static void composition_source_over(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;

    uint32_t s, sia;
    if (params.const_alpha == 255) {
        for (size_t i = 0; i < params.length; i++) {
            s = params.src[i];
            if (s >= 0xff000000) {
                dest[i] = s;
            } else if (s != 0) {
                sia = plutovg_alpha(~s);
                dest[i] = s + BYTE_MUL(dest[i], sia);
            }
        }
    } else {
        for (size_t i = 0; i < params.length; i++) {
            s = BYTE_MUL(params.src[i], params.const_alpha);
            sia = plutovg_alpha(~s);
            dest[i] = s + BYTE_MUL(dest[i], sia);
        }
    }
}

static void composition_destination_in(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;
    if (params.const_alpha == 255) {
        for (size_t i = 0; i < params.length; i++) {
            dest[i] = BYTE_MUL(dest[i], plutovg_alpha(params.src[i]));
        }
    } else {
        uint32_t cia = 255 - params.const_alpha;
        uint32_t a;
        for (size_t i = 0; i < params.length; i++) {
            a = BYTE_MUL(plutovg_alpha(params.src[i]), params.const_alpha) +
                cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

static void composition_destination_out(const composition_params& params) {
    uint32_t* dest = (uint32_t*)params.direct;
    if (params.const_alpha == 255) {
        for (size_t i = 0; i < params.length; i++) {
            dest[i] = BYTE_MUL(dest[i], plutovg_alpha(~params.src[i]));
        }
    } else {
        uint32_t cia = 255 - params.const_alpha;
        uint32_t sia;
        for (size_t i = 0; i < params.length; i++) {
            sia = BYTE_MUL(plutovg_alpha(~params.src[i]), params.const_alpha) +
                  cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

typedef void (*composition_function_t)(const composition_params& params);

static const composition_function_t composition_solid_table[] = {
    composition_solid_source, composition_solid_source_over,
    composition_solid_destination_in, composition_solid_destination_out};

static const composition_function_t composition_table[] = {
    composition_source, composition_source_over, composition_destination_in,
    composition_destination_out};

#define BUFFER_SIZE 128
enum {
    MODE_CALLBACK = 0,
    MODE_DIRECT = 1,
};

static void blend_solid(plutovg_canvas_t* canvas, plutovg_operator_t op,
                        uint32_t solid,
                        const plutovg_span_buffer_t* span_buffer) {
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;
    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_solid_table[((int)op)];
    uint32_t buffer[BUFFER_SIZE];

    // Process each span from the span buffer
    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    switch (mode) {
        case MODE_CALLBACK:
            while (count--) {
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);
                    params.length = l;
                    params.color = solid;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.src = nullptr;

                    // Read the current surface pixels as ARGB Premultiplied
                    // into the buffer using read_callback

                    for (int i = 0; i < l; ++i) {
                        ::gfx::vector_pixel c;
                        if (params.canvas->read_callback != nullptr) {
                            params.canvas->read_callback(
                                ::gfx::point16(params.x + i, params.y), &c,
                                params.canvas->callback_state);
                        }

                        uint32_t r = c.template channel<gfx::channel_name::R>();
                        uint32_t g = c.template channel<gfx::channel_name::G>();
                        uint32_t b = c.template channel<gfx::channel_name::B>();
                        uint32_t a = c.template channel<gfx::channel_name::A>();
                        if (a != 255) {
                            r = (r * a) / 255;
                            g = (g * a) / 255;
                            b = (b * a) / 255;
                        }

                        buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                    (b);  // covert c to ARBGP and store it in
                                          // the buffer;
                    }

                    params.direct = (uint8_t*)buffer;
                    // Blend the solid color into the buffer
                    func(params);

                    // Write the modified buffer back to the surface using write
                    // callback
                    for (int i = 0; i < l; ++i) {
                        uint32_t pixel = buffer[i];
                        if (params.canvas->write_callback != nullptr) {
                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }

                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
        case MODE_DIRECT:
            while (count--) {
                
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);
                    params.length = l;
                    params.color = solid;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.src = nullptr;
                    // Read the current surface pixels as ARGB Premultiplied
                    // into buffer
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    if (cs.cdata != nullptr) {

                        on_read(buffer, cs.cdata, ll);
                        // Blend the data from target into the buffer
                        params.direct = (uint8_t*)
                            buffer;  // direct->span(::gfx::point16(params.x,params.y)).data;
                        func(params);
                        // Write the modified buffer back to the target
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        ll=ll>s.length?s.length:ll;
                        if (s.data != nullptr) {
                            on_write(s.data, buffer, ll);
                        }
                    }

                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
    }
}

static void blend_linear_gradient(plutovg_canvas_t* canvas,
                                  plutovg_operator_t op,
                                  const gradient_data_t* gradient,
                                  const plutovg_span_buffer_t* span_buffer) {
    // Select the appropriate blending function for the given operator
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;

    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];
    uint32_t buffer[BUFFER_SIZE];
    uint32_t src_buffer[BUFFER_SIZE];

    // Calculate the gradient's direction and length
    linear_gradient_values_t v;
    v.dx = gradient->values.linear.x2 - gradient->values.linear.x1;
    v.dy = gradient->values.linear.y2 - gradient->values.linear.y1;
    v.l = v.dx * v.dx + v.dy * v.dy;
    v.off = 0.f;
    if (v.l != 0.f) {
        v.dx /= v.l;
        v.dy /= v.l;
        v.off = -v.dx * gradient->values.linear.x1 -
                v.dy * gradient->values.linear.y1;
    }

    // Process each span from the span buffer
    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    switch (mode) {
        case MODE_CALLBACK:
            while (count--) {
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);
                    fetch_linear_gradient(src_buffer, &v, gradient, spans->y, x,
                                          l);
                    params.src = src_buffer;
                    params.length = l;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.direct = (uint8_t*)buffer;

                    // Read the current surface pixels as ARGB Premultiplied
                    // into the buffer using read_callback

                    for (int i = 0; i < l; ++i) {
                        ::gfx::vector_pixel c;
                        if (params.canvas->read_callback != nullptr) {
                            params.canvas->read_callback(
                                ::gfx::point16(params.x + i, params.y), &c,
                                params.canvas->callback_state);
                        }

                        uint32_t r = c.template channel<gfx::channel_name::R>();
                        uint32_t g = c.template channel<gfx::channel_name::G>();
                        uint32_t b = c.template channel<gfx::channel_name::B>();
                        uint32_t a = c.template channel<gfx::channel_name::A>();
                        if (a != 255) {
                            r = (r * a) / 255;
                            g = (g * a) / 255;
                            b = (b * a) / 255;
                        }

                        buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                    (b);  // covert c to ARBGP and store it in
                                          // the buffer;
                    }

                    // Blend the solid color into the buffer
                    func(params);

                    // Write the modified buffer back to the surface using write
                    // callback
                    for (int i = 0; i < l; ++i) {
                        uint32_t pixel = buffer[i];
                        if (params.canvas->write_callback != nullptr) {
                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }

                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
        case MODE_DIRECT:
            while (count--) {
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);

                    // Fetch the gradient image into the buffer
                    fetch_linear_gradient(src_buffer, &v, gradient, spans->y, x,
                                          l);
                    params.src = src_buffer;
                    params.length = l;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.direct = (uint8_t*)buffer;
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    if (cs.cdata != nullptr) {
                        // Read the current surface pixels as ARGB Premultiplied
                        // into buffer
                        on_read(buffer, cs.cdata, ll);
                        // Blend the data from target into the buffer

                        func(params);
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        if (s.data != nullptr) {
                            ll=ll>s.length?s.length:ll;
                            // Write the modified buffer back to the target
                            on_write(s.data, buffer, ll);
                        }
                    }

                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
    }
}
static void blend_radial_gradient(plutovg_canvas_t* canvas,
                                  plutovg_operator_t op,
                                  const gradient_data_t* gradient,
                                  const plutovg_span_buffer_t* span_buffer) {
    // Select the appropriate blending function for the given operator
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;

    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];

    uint32_t buffer[BUFFER_SIZE];
    uint32_t src_buffer[BUFFER_SIZE];
    params.canvas = canvas;

    radial_gradient_values_t v;
    v.dx = gradient->values.radial.cx - gradient->values.radial.fx;
    v.dy = gradient->values.radial.cy - gradient->values.radial.fy;
    v.dr = gradient->values.radial.cr - gradient->values.radial.fr;
    v.sqrfr = gradient->values.radial.fr * gradient->values.radial.fr;
    v.a = v.dr * v.dr - v.dx * v.dx - v.dy * v.dy;
    v.inv2a = 1.f / (2.f * v.a);
    v.extended = gradient->values.radial.fr != 0.f || v.a <= 0.f;

    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    switch (mode) {
        case MODE_CALLBACK:
            while (count--) {
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);
                    fetch_radial_gradient(src_buffer, &v, gradient, spans->y, x,
                                          l);
                    params.src = src_buffer;
                    params.length = l;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.direct = (uint8_t*)buffer;

                    // Read the current surface pixels as ARGB Premultiplied
                    // into the buffer using read_callback

                    for (int i = 0; i < l; ++i) {
                        ::gfx::vector_pixel c;
                        if (params.canvas->read_callback != nullptr) {
                            params.canvas->read_callback(
                                ::gfx::point16(params.x + i, params.y), &c,
                                params.canvas->callback_state);
                        }

                        uint32_t r = c.template channel<gfx::channel_name::R>();
                        uint32_t g = c.template channel<gfx::channel_name::G>();
                        uint32_t b = c.template channel<gfx::channel_name::B>();
                        uint32_t a = c.template channel<gfx::channel_name::A>();
                        if (a != 255) {
                            r = (r * a) / 255;
                            g = (g * a) / 255;
                            b = (b * a) / 255;
                        }

                        buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                    (b);  // covert c to ARBGP and store it in
                                          // the buffer;
                    }

                    // Blend the solid color into the buffer
                    func(params);

                    // Write the modified buffer back to the surface using write
                    // callback
                    for (int i = 0; i < l; ++i) {
                        uint32_t pixel = buffer[i];
                        if (params.canvas->write_callback != nullptr) {
                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }

                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
        case MODE_DIRECT:
            while (count--) {
                int length = spans->len;
                int x = spans->x;
                while (length) {
                    int l = plutovg_min(length, BUFFER_SIZE);
                    fetch_radial_gradient(src_buffer, &v, gradient, spans->y, x,
                                          l);

                    params.src = src_buffer;
                    params.length = l;
                    params.const_alpha = spans->coverage;
                    params.x = x + offset_x;
                    params.y = spans->y + offset_y;
                    params.direct = (uint8_t*)buffer;
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    if (cs.cdata != nullptr) {
                        // Read the current surface pixels as ARGB Premultiplied
                        // into buffer
                        on_read(buffer, cs.cdata, ll);
                        func(params);
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        if (s.data != nullptr) {
                            ll=ll>s.length?s.length:ll;
                            // Write the modified buffer back to the target
                            on_write(s.data, buffer, ll);
                        }
                    }
                    x += l;
                    length -= l;
                }

                ++spans;
            }
            break;
    }
}

#define FIXED_SCALE (1 << 16)
static void blend_transformed_argb(plutovg_canvas_t* canvas,
                                   plutovg_operator_t op,
                                   const texture_data_t* texture,
                                   const plutovg_span_buffer_t* span_buffer) {
    // Select the appropriate blending function for the given operator
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;
    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];
    uint32_t buffer[BUFFER_SIZE];
    uint32_t src_buffer[BUFFER_SIZE];
    params.canvas = canvas;

    int image_width = texture->width;
    int image_height = texture->height;

    int fdx = (int)(texture->matrix.a * FIXED_SCALE);
    int fdy = (int)(texture->matrix.b * FIXED_SCALE);

    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;

    while (count--) {
        params.color = 0;
        params.const_alpha = spans->coverage;
        params.x = spans->x + offset_x;
        params.y = spans->y + offset_y;
        const float cx = spans->x + 0.5f;
        const float cy = spans->y + 0.5f;

        int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx +
                       texture->matrix.e) *
                      FIXED_SCALE);
        int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx +
                       texture->matrix.f) *
                      FIXED_SCALE);

        int length = spans->len;
        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        while (length) {
            int l = plutovg_min(length, BUFFER_SIZE);
            const uint32_t* end = src_buffer + l;
            uint32_t* b = src_buffer;
            while (b < end) {
                int px = x >> 16;
                int py = y >> 16;
                if ((px < 0) || (px >= image_width) || (py < 0) ||
                    (py >= image_height)) {
                    *b = 0x00000000;
                } else {
                    if (texture->direct != nullptr) {
                        texture->on_direct_read(
                            b,
                            texture->direct->cspan(::gfx::point16(px, py))
                                .cdata,
                            1);
                    } else if (texture->on_read != nullptr) {
                        uint32_t pixel = 0;
                        ::gfx::vector_pixel c;
                        texture->on_read(::gfx::point16(px, py), &c,
                                         texture->on_read_state);
                        pixel = c.native_value;
                        *b = pixel;
                    }
                }
                x += fdx;
                y += fdy;
                ++b;
            }

            params.length = l;
            params.src = src_buffer;
            params.const_alpha = coverage;
            params.direct = (uint8_t*)buffer;
            switch (mode) {
                case MODE_CALLBACK:
                    // Read the current surface pixels as ARGB Premultiplied
                    // into the buffer using read_callback
                    for (int i = 0; i < l; ++i) {
                        ::gfx::vector_pixel c;
                        if (params.canvas->read_callback != nullptr) {
                            params.canvas->read_callback(
                                ::gfx::point16(params.x + i, params.y), &c,
                                params.canvas->callback_state);
                        }

                        uint32_t r = c.template channel<gfx::channel_name::R>();
                        uint32_t g = c.template channel<gfx::channel_name::G>();
                        uint32_t b = c.template channel<gfx::channel_name::B>();
                        uint32_t a = c.template channel<gfx::channel_name::A>();
                        if (a != 255) {
                            r = (r * a) / 255;
                            g = (g * a) / 255;
                            b = (b * a) / 255;
                        }

                        buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                    (b);  // covert c to ARBGP and store it in
                                          // the buffer;
                    }

                    func(params);
                    // Write the modified buffer back to the surface using write
                    // callback
                    for (int i = 0; i < l; ++i) {
                        uint32_t pixel = buffer[i];
                        if (params.canvas->write_callback != nullptr) {
                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }
                    break;
                case MODE_DIRECT: {
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    // Read the current surface pixels as ARGB Premultiplied
                    // into buffer
                    if (cs.cdata != nullptr) {
                        on_read(buffer, cs.cdata, ll);
                        params.direct = (uint8_t*)buffer;
                        func(params);
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        if (s.data != nullptr) {
                            ll=ll>s.length?s.length:ll;
                            // Write the modified buffer back to the target
                            on_write(s.data, buffer, params.length);
                        }
                    }
                } break;
            }
            params.x += l;
            // target += l;
            length -= l;
        }

        ++spans;
    }
}

static void blend_untransformed_argb(plutovg_canvas_t* canvas,
                                     plutovg_operator_t op,
                                     const texture_data_t* texture,
                                     const plutovg_span_buffer_t* span_buffer) {
    // Select the appropriate blending function for the given operator
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;
    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];
    params.canvas = canvas;

    const int image_width = texture->width;
    const int image_height = texture->height;

    int xoff = (int)(texture->matrix.e);
    int yoff = (int)(texture->matrix.f);
    uint32_t buffer[BUFFER_SIZE];
    uint32_t src_buffer[BUFFER_SIZE];
    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }

            if (sx + length > image_width) length = image_width - sx;
            if (length > 0) {
                const int coverage =
                    (spans->coverage * texture->const_alpha) >> 8;
                if (texture->direct != nullptr) {
                    texture->on_direct_read(
                        src_buffer,
                        texture->direct->cspan(::gfx::point16(sx, sy)).cdata,
                        length);
                } else {
                    if (texture->on_read != nullptr) {
                        ::gfx::point16 pt;
                        pt.y = sy;
                        for (int i = 0; i < length; ++i) {
                            pt.x = i + sx;
                            ::gfx::vector_pixel px;
                            texture->on_read(pt, &px, texture->on_read_state);
                            src_buffer[i] = px.native_value;
                        }
                    } else {
                        memset(src_buffer, 0, length * sizeof(uint32_t));
                    }
                }
                // uint32_t* dest = (uint32_t*)(surface->data + spans->y *
                // surface->stride) + x; params.dest = dest;
                // for(int i = 0; i < length; ++i) {
                //     uint32_t pixel = src[i];
                //     uint32_t b = (pixel >> 24) & 0xFF;
                //     uint32_t g = (pixel >> 16) & 0xFF;
                //     uint32_t r = (pixel >> 8) & 0xFF;
                //     uint32_t a = (pixel >> 0) & 0xFF;
                //     if(a != 255) {
                //         r = (r * a) / 255;
                //         g = (g * a) / 255;
                //         b = (b * a) / 255;
                //     }

                //     src_buffer[i] = (a << 24) | (r << 16) | (g << 8) | (b);
                // }
                params.src = src_buffer;
                params.length = length;
                params.const_alpha = coverage;
                params.x = x + offset_x;
                params.y = spans->y + offset_y;
                params.direct = (uint8_t*)buffer;
                switch (mode) {
                    case MODE_CALLBACK:
                        // Read the current surface pixels as ARGB Premultiplied
                        // into the buffer using read_callback
                        for (size_t i = 0; i < params.length; ++i) {
                            ::gfx::vector_pixel c;
                            if (params.canvas->read_callback != nullptr) {
                                params.canvas->read_callback(
                                    ::gfx::point16(params.x + i, params.y), &c,
                                    params.canvas->callback_state);
                            }

                            uint32_t r =
                                c.template channel<gfx::channel_name::R>();
                            uint32_t g =
                                c.template channel<gfx::channel_name::G>();
                            uint32_t b =
                                c.template channel<gfx::channel_name::B>();
                            uint32_t a =
                                c.template channel<gfx::channel_name::A>();
                            if (a != 255) {
                                r = (r * a) / 255;
                                g = (g * a) / 255;
                                b = (b * a) / 255;
                            }

                            buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                        (b);  // covert c to ARBGP and store it
                                              // in the buffer;
                        }

                        func(params);
                        // Write the modified buffer back to the surface using
                        // write callback
                        for (size_t i = 0; i < params.length; ++i) {
                            uint32_t pixel = buffer[i];
                            if (params.canvas->write_callback != nullptr) {
                                ::gfx::vector_pixel px(pixel, true);

                                params.canvas->write_callback(
                                    ::gfx::rect16(params.x + i, params.y,
                                                  params.x + i, params.y),
                                    px, params.canvas->callback_state);
                            }
                        }
                        break;
                    case MODE_DIRECT: {
                        const ::gfx::gfx_cspan cs =
                            direct->cspan(::gfx::point16(params.x, params.y));
                        size_t ll = params.length>cs.length?cs.length:params.length;
                        if (cs.cdata != nullptr) {
                            // Read the current surface pixels as ARGB
                            // Premultiplied into buffer
                            on_read(buffer, cs.cdata, ll);
                            params.direct = (uint8_t*)buffer;
                            func(params);
                            ::gfx::gfx_span s = direct->span(
                                ::gfx::point16(params.x, params.y));
                            if (s.data != nullptr) {
                                ll=ll>s.length?s.length:ll;
                                // Write the modified buffer back to the target
                                on_write(s.data, buffer, ll);
                            }
                        }
                    } break;
                }
            }
        }

        ++spans;
    }
}

static void blend_untransformed_tiled_argb(
    plutovg_canvas_t* canvas, plutovg_operator_t op,
    const texture_data_t* texture, const plutovg_span_buffer_t* span_buffer) {
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;
    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];
    params.canvas = canvas;
    uint32_t buffer[BUFFER_SIZE];
    uint32_t src_buffer[BUFFER_SIZE];
    int image_width = texture->width;
    int image_height = texture->height;

    int xoff = (int)(texture->matrix.e) % image_width;
    int yoff = (int)(texture->matrix.f) % image_height;

    if (xoff < 0) xoff += image_width;
    if (yoff < 0) {
        yoff += image_height;
    }

    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0) sx += image_width;
        if (sy < 0) {
            sy += image_height;
        }

        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        while (length) {
            int l = plutovg_min(image_width - sx, length);
            if (BUFFER_SIZE < l) l = BUFFER_SIZE;
            if (texture->direct != nullptr) {
                ::gfx::point16 pt(sx, sy);
                texture->on_direct_read(src_buffer,
                                        texture->direct->cspan(pt).cdata, l);
                //  for(int i = 0; i < l; ++i) {
                //      uint32_t pixel = src_buffer[i];
                //      uint32_t a = (pixel >> 24) & 0xFF;
                //      uint32_t r = (pixel >> 16) & 0xFF;
                //      uint32_t g = (pixel >> 8) & 0xFF;
                //      uint32_t b = (pixel >> 0) & 0xFF;
                //     // if(a != 255) {
                //     //     r = (r * a) / 255;
                //     //     g = (g * a) / 255;
                //     //     b = (b * a) / 255;
                //     // }
                //     //src_buffer[i] = (b << 24) | (g << 16) | (r << 8) | (a);
                //     src_buffer[i] = (a << 24) | (r << 16) | (g << 8) | (b);
                // }
            } else {
                if (texture->on_read != nullptr) {
                    ::gfx::point16 pt;
                    pt.y = sy;
                    for (int i = 0; i < l; ++i) {
                        pt.x = i + sx;
                        ::gfx::vector_pixel px;
                        texture->on_read(pt, &px, texture->on_read_state);
                        src_buffer[i] = px.native_value;
                    }
                } else {
                    memset(src_buffer, 0, l * sizeof(uint32_t));
                }
            }

            params.src = src_buffer;
            params.x = x + offset_x;
            params.y = spans->y + offset_y;
            params.length = l;
            params.const_alpha = coverage;
            params.direct = (uint8_t*)buffer;
            switch (mode) {
                case MODE_CALLBACK:
                    if (params.canvas->write_callback != nullptr) {
                        // Read the current surface pixels as ARGB Premultiplied
                        // into the buffer using read_callback
                        ::gfx::vector_pixel c;
                        if (params.canvas->read_callback != nullptr) {
                            for (int i = 0; i < l; ++i) {
                                params.canvas->read_callback(
                                    ::gfx::point16(params.x + i, params.y), &c,
                                    params.canvas->callback_state);

                                uint32_t r =
                                    c.template channel<gfx::channel_name::R>();
                                uint32_t g =
                                    c.template channel<gfx::channel_name::G>();
                                uint32_t b =
                                    c.template channel<gfx::channel_name::B>();
                                uint32_t a =
                                    c.template channel<gfx::channel_name::A>();
                                if (a != 255) {
                                    r = (r * a) / 255;
                                    g = (g * a) / 255;
                                    b = (b * a) / 255;
                                }

                                buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                            (b);  // covert c to ARBGP and store
                                                  // it in the buffer;
                            }
                        } else {
                            memset(buffer, 0, l * sizeof(uint32_t));
                        }
                        func(params);

                        // Write the modified buffer back to the surface using
                        // write callback
                        for (int i = 0; i < l; ++i) {
                            uint32_t pixel = buffer[i];

                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }
                    break;
                case MODE_DIRECT: {
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    if (cs.cdata != nullptr) {
                        // Read the current surface pixels as ARGB Premultiplied
                        // into buffer
                        on_read(buffer, cs.cdata, ll);
                        // Blend the data from target into the buffer
                        params.direct = (uint8_t*)buffer;
                        func(params);
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        if (s.data != nullptr) {
                            params.direct = s.data;
                            ll=ll>s.length?s.length:ll;
                            // Write the modified buffer back to the target
                            on_write(params.direct, buffer, ll);
                        }
                    }
                } break;
            }

            x += l;
            length -= l;
            sx = 0;
        }

        ++spans;
    }
}

static void blend_transformed_tiled_argb(
    plutovg_canvas_t* canvas, plutovg_operator_t op,
    const texture_data_t* texture, const plutovg_span_buffer_t* span_buffer) {
    composition_params params;
    params.canvas = canvas;
    int offset_x = 0;
    int offset_y = 0;
    ::gfx::blt_span* direct = plutovg_canvas_get_direct(canvas);
    int mode = direct != nullptr;
    ::gfx::on_direct_read_callback_type on_read = nullptr;
    ::gfx::on_direct_write_callback_type on_write = nullptr;
    if (mode > 0) {
        offset_x = plutovg_canvas_get_direct_offset_x(canvas);
        offset_y = plutovg_canvas_get_direct_offset_y(canvas);
        on_read = plutovg_canvas_get_direct_on_read(canvas);
        on_write = plutovg_canvas_get_direct_on_write(canvas);
    }
    composition_function_t func = composition_table[((int)op)];
    params.canvas = canvas;

    uint32_t src_buffer[BUFFER_SIZE];
    uint32_t buffer[BUFFER_SIZE];
    int image_width = texture->width;
    int image_height = texture->height;
    // const int scanline_offset = texture->width;

    int fdx = (int)(texture->matrix.a * FIXED_SCALE);
    int fdy = (int)(texture->matrix.b * FIXED_SCALE);

    int count = span_buffer->spans.size;
    const plutovg_span_t* spans = span_buffer->spans.data;
    while (count--) {
        // uint32_t* target = (uint32_t*)(surface->data + spans->y *
        // surface->stride) + spans->x;
        params.x = spans->x + offset_x;
        params.y = spans->y + offset_y;
        // params.dest = target;
        // const uint32_t* image_bits = (const uint32_t*)texture->data;

        const float cx = spans->x + 0.5;
        const float cy = spans->y + 0.5;

        int x = (int)((texture->matrix.c * cy + texture->matrix.a * cx +
                       texture->matrix.e) *
                      FIXED_SCALE);
        int y = (int)((texture->matrix.d * cy + texture->matrix.b * cx +
                       texture->matrix.f) *
                      FIXED_SCALE);

        const int coverage = (spans->coverage * texture->const_alpha) >> 8;
        int length = spans->len;
        while (length) {
            int l = plutovg_min(length, BUFFER_SIZE);
            const uint32_t* end = src_buffer + l;
            uint32_t* b = src_buffer;
            while (b < end) {
                int px = x >> 16;
                int py = y >> 16;
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
                // int y_offset = py * scanline_offset;

                assert(px >= 0 && px < image_width);
                assert(py >= 0 && py < image_height);

                // uint32_t pixel;// = image_bits[y_offset+px];
                if (texture->direct != nullptr) {
                    texture->on_direct_read(
                        b, texture->direct->cspan(::gfx::point16(px, py)).cdata,
                        1);
                } else if (texture->on_read != nullptr) {
                    ::gfx::vector_pixel c;
                    texture->on_read(::gfx::point16(px, py), &c,
                                     texture->on_read_state);
                    *b = c.native_value;
                } else {
                    *b = 0;
                }
                // uint32_t blue = (pixel >> 24) & 0xFF;
                // uint32_t green = (pixel >> 16) & 0xFF;
                // uint32_t red = (pixel >> 8) & 0xFF;
                // uint32_t alpha = (pixel >> 0) & 0xFF;
                // if(alpha != 255) {
                //     red = (red * alpha) / 255;
                //     green = (green * alpha) / 255;
                //     blue = (blue * alpha) / 255;
                // }
                // *b = (alpha << 24) | (red << 16) | (green << 8) | (blue);

                x += fdx;
                y += fdy;
                ++b;
            }

            params.src = src_buffer;
            params.length = l;
            params.const_alpha = coverage;
            params.direct = (uint8_t*)buffer;
            switch (mode) {
                case MODE_CALLBACK:
                    // Read the current surface pixels as ARGB Premultiplied
                    // into the buffer using read_callback
                    if (params.canvas->write_callback != nullptr) {
                        if (params.canvas->read_callback != nullptr) {
                            for (int i = 0; i < l; ++i) {
                                ::gfx::vector_pixel c;

                                params.canvas->read_callback(
                                    ::gfx::point16(params.x + i, params.y), &c,
                                    params.canvas->callback_state);

                                uint32_t r =
                                    c.template channel<gfx::channel_name::R>();
                                uint32_t g =
                                    c.template channel<gfx::channel_name::G>();
                                uint32_t b =
                                    c.template channel<gfx::channel_name::B>();
                                uint32_t a =
                                    c.template channel<gfx::channel_name::A>();
                                if (a != 255) {
                                    r = (r * a) / 255;
                                    g = (g * a) / 255;
                                    b = (b * a) / 255;
                                }

                                buffer[i] = (a << 24) | (r << 16) | (g << 8) |
                                            (b);  // covert c to ARBGP and store
                                                  // it in the buffer;
                            }
                        } else {
                            memset(buffer, 0, sizeof(uint32_t) * l);
                        }

                        func(params);
                        // Write the modified buffer back to the surface using
                        // write callback
                        for (int i = 0; i < l; ++i) {
                            uint32_t pixel = buffer[i];

                            ::gfx::vector_pixel px(pixel, true);

                            params.canvas->write_callback(
                                ::gfx::rect16(params.x + i, params.y,
                                              params.x + i, params.y),
                                px, params.canvas->callback_state);
                        }
                    }
                    break;
                case MODE_DIRECT: {
                    const ::gfx::gfx_cspan cs =
                        direct->cspan(::gfx::point16(params.x, params.y));
                    size_t ll = params.length>cs.length?cs.length:params.length;
                    if (cs.cdata != nullptr) {
                        // Read the current surface pixels as ARGB Premultiplied
                        // into buffer
                        on_read(buffer, cs.cdata, ll);
                        params.direct = (uint8_t*)buffer;
                        func(params);
                        ::gfx::gfx_span s =
                            direct->span(::gfx::point16(params.x, params.y));
                        if (s.data != nullptr) {
                            ll=ll>s.length?s.length:ll;
                            // Write the modified buffer back to the target
                            on_write(s.data, buffer, ll);
                        }
                    }
                } break;
            }
            length -= l;
        }

        ++spans;
    }
}

static void plutovg_blend_color(plutovg_canvas_t* canvas,
                                const plutovg_color_t* color,
                                const plutovg_span_buffer_t* span_buffer) {
    plutovg_state_t* state = canvas->state;
    uint32_t solid = premultiply_color_with_opacity(color, state->opacity);
    ;
    uint32_t alpha = plutovg_alpha(solid);
    if (alpha == 255 && state->op == PLUTOVG_OPERATOR_SRC_OVER) {
        blend_solid(canvas, PLUTOVG_OPERATOR_SRC, solid, span_buffer);
    } else {
        blend_solid(canvas, state->op, solid, span_buffer);
    }
}

static void plutovg_blend_gradient(plutovg_canvas_t* canvas,
                                   const plutovg_gradient_paint_t* gradient,
                                   const plutovg_span_buffer_t* span_buffer) {
    if (gradient->nstops == 0) return;
    plutovg_state_t* state = canvas->state;
    gradient_data_t data;
    data.spread = gradient->spread;
    data.matrix = gradient->matrix;
    data.matrix = data.matrix * state->matrix;

    if (!::gfx::matrix::invert(data.matrix, &data.matrix)) return;
    int i, pos = 0, nstops = gradient->nstops;
    const plutovg_gradient_stop_t *curr, *next, *start, *last;
    uint32_t curr_color, next_color, last_color;
    uint32_t dist, idist;
    float delta, t, incr, fpos;
    float opacity = state->opacity;

    start = gradient->stops;
    curr = start;
    plutovg_color_t cc;
    plutovg_color_init_argb32(&cc, curr->color.native_value);
    curr_color = combine_color_with_opacity(&cc, opacity);
    data.colortable[pos++] = plutovg_premultiply_argb(curr_color);
    incr = 1.f / COLOR_TABLE_SIZE;
    fpos = 1.5f * incr;

    while (fpos <= curr->offset) {
        data.colortable[pos] = data.colortable[pos - 1];
        ++pos;
        fpos += incr;
    }

    for (i = 0; i < nstops - 1; i++) {
        curr = (start + i);
        next = (start + i + 1);
        delta = 1.f / (next->offset - curr->offset);
        plutovg_color_init_argb32(&cc, next->color.native_value);
        next_color = combine_color_with_opacity(&cc, opacity);
        while (fpos < next->offset && pos < COLOR_TABLE_SIZE) {
            t = (fpos - curr->offset) * delta;
            dist = (uint32_t)(255 * t);
            idist = 255 - dist;
            data.colortable[pos] = plutovg_premultiply_argb(
                interpolate_pixel(curr_color, idist, next_color, dist));
            ++pos;
            fpos += incr;
        }

        curr_color = next_color;
    }
    last = start + nstops - 1;
    plutovg_color_init_argb32(&cc, last->color.native_value);
    last_color = premultiply_color_with_opacity(&cc, opacity);
    for (; pos < COLOR_TABLE_SIZE; ++pos) {
        data.colortable[pos] = last_color;
    }
    
    if (gradient->type == PLUTOVG_GRADIENT_TYPE_LINEAR) {
        data.values.linear.x1 = gradient->values[0];
        data.values.linear.y1 = gradient->values[1];
        data.values.linear.x2 = gradient->values[2];
        data.values.linear.y2 = gradient->values[3];
        blend_linear_gradient(canvas, state->op, &data, span_buffer);
    } else {
        data.values.radial.cx = gradient->values[0];
        data.values.radial.cy = gradient->values[1];
        data.values.radial.cr = gradient->values[2];
        data.values.radial.fx = gradient->values[3];
        data.values.radial.fy = gradient->values[4];
        data.values.radial.fr = gradient->values[5];
        blend_radial_gradient(canvas, state->op, &data, span_buffer);
    }
}

static void plutovg_blend_texture(plutovg_canvas_t* canvas,
                                  const plutovg_texture_paint_t* texture,
                                  const plutovg_span_buffer_t* span_buffer) {
    plutovg_state_t* state = canvas->state;
    texture_data_t data;
    data.matrix = texture->matrix;
    data.direct = texture->direct;
    data.on_direct_read = texture->on_direct_read_callback;
    data.on_read = texture->on_read_callback;
    data.on_read_state = texture->on_read_callback_state;
    data.width = texture->width;
    data.height = texture->height;
    data.const_alpha = lroundf(state->opacity * texture->opacity * 256);
    data.matrix = data.matrix * state->matrix;
    if (!::gfx::matrix::invert(data.matrix, &data.matrix)) return;
    const ::gfx::matrix* matrix = &data.matrix;
    bool translating =
        (matrix->a == 1 && matrix->b == 0 && matrix->c == 0 && matrix->d == 1);
    if (translating) {
        if (texture->type == PLUTOVG_TEXTURE_TYPE_PLAIN) {
            blend_untransformed_argb(canvas, state->op, &data, span_buffer);
        } else {
            blend_untransformed_tiled_argb(canvas, state->op, &data,
                                           span_buffer);
        }
    } else {
        if (texture->type == PLUTOVG_TEXTURE_TYPE_PLAIN) {
            blend_transformed_argb(canvas, state->op, &data, span_buffer);
        } else {
            blend_transformed_tiled_argb(canvas, state->op, &data, span_buffer);
        }
    }
}

bool plutovg_blend(plutovg_canvas_t* canvas,
                   const plutovg_span_buffer_t* span_buffer) {
    if(span_buffer->spans.data==nullptr) return false;
    if (span_buffer->spans.size == 0) return true;
    if (canvas->state->paint == NULL) {
        plutovg_blend_color(canvas, &canvas->state->color, span_buffer);
        return true;
    }

    plutovg_paint_t* paint = canvas->state->paint;
    if (paint->type == PLUTOVG_PAINT_TYPE_COLOR) {
        plutovg_solid_paint_t* solid = (plutovg_solid_paint_t*)(paint);
        plutovg_blend_color(canvas, &solid->color, span_buffer);
    } else if (paint->type == PLUTOVG_PAINT_TYPE_GRADIENT) {
        plutovg_gradient_paint_t* gradient = (plutovg_gradient_paint_t*)(paint);
        plutovg_blend_gradient(canvas, gradient, span_buffer);
    } else {
        plutovg_texture_paint_t* texture = (plutovg_texture_paint_t*)(paint);
        plutovg_blend_texture(canvas, texture, span_buffer);
    }
    return true;
}
