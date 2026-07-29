#ifndef HTCW_GFX_DRAW_BITMAP_HPP
#define HTCW_GFX_DRAW_BITMAP_HPP
#include "gfx_bitmap.hpp"
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
namespace gfx {

// enum struct bitmap_resize {
//     crop = 0,
//     resize_fast = 1,
//     resize_bilinear = 2,
//     resize_bicubic = 3
// };
namespace helpers {
class xdraw_bitmap {
    struct rect_info {
        int width;
        int height;
        point16 src_loc;
        point16 dst_loc;
    };
    template <bool HasAlpha, typename Destination, typename Source>
    static gfx_result draw_bitmap_crop_span(
        Destination& destination, const srect16& dest_rect, const srect16& dsr,
        Source& source, const rect16& srcr,
        int w, int yy, int syy, int o,
        const typename Source::pixel_type* transparent_color) {
        gfx_result r;
        for (int x = 0; x < w; ++x) {
            int xx = x + dest_rect.left();
            if (xx < dsr.x1) {
                continue;
            } else if (xx > dsr.x2)
                break;
            spoint16 spt(xx, yy);
            int sxx = x + srcr.left();
            if ((int)rect_orientation::flipped_horizontal == ((int)rect_orientation::flipped_horizontal & o)) {
                sxx = srcr.right() - x;
            }
            typename Source::pixel_type srcpx;
            point16 srcpt(sxx, syy);
            r = source.point(srcpt, &srcpx);
            if (gfx_result::success != r) {
                return r;
            }
            if (!HasAlpha && nullptr == transparent_color) {
                typename Destination::pixel_type px;
                r = convert_palette(destination, source, srcpx, &px);
                if (gfx_result::success != r) {
                    return r;
                }
                r = destination.point((point16)spt, px);
                if (gfx_result::success != r) {
                    return r;
                }
            } else if (nullptr == transparent_color || transparent_color->native_value != srcpx.native_value) {
                r = helpers::blender<Destination, Source, HasAlpha>::point(destination, (point16)spt, source, srcpt, srcpx);
                if (gfx_result::success != r) {
                    return r;
                }
            }
        }
        return gfx_result::success;
    }
    template <typename Destination, typename Source>
    static gfx_result blend_rgba8888_to_rgb565(Destination& destination, spoint16 location,
                                               const Source source, spoint16 source_location, size_t width) {
        gfx_cspan src_span = source.cspan((point16)source_location);
        const uint8_t* src = src_span.cdata;
        if (src == nullptr) return gfx_result::success;
        const int16_t minx = location.x, py = location.y;
        const int16_t row_w = (int16_t)width;

        int i = 0;
        while (i < row_w) {
            gfx_span span = destination.span(point16((int16_t)(minx + i), py));
            if (span.length == 0 || span.data == nullptr) {
                ++i;  // column outside bitmap: skip src[i] too
                continue;
            }
            uint8_t* d = span.data;
            const int run = (int)(span.length >> 1);  // dest pixels available in this span
            int k = 0;                                // pixel index within this span
            for (; k < run && i < row_w; ++k, ++i) {
                const uint8_t* s = src + ((size_t)i << 2);  // 4 bytes per RGBA pixel
#ifndef HTCW_GFX_NO_SWAP
                const uint8_t sr = s[0], sg = s[1], sb = s[2], sa = s[3];
#else
                const uint8_t sa = s[0], sb = s[1], sg = s[2], sr = s[3];
#endif
                if (sa == 0) continue;

                // pack source 8:8:8 -> 5:6:5
                const uint16_t fg = (uint16_t)(((uint16_t)(sr & 0xF8) << 8) |
                                               ((uint16_t)(sg & 0xFC) << 3) |
                                               ((uint16_t)(sb) >> 3));

                const int j = k << 1;  // byte offset within THIS span
                if (sa >= 255) {
#ifndef HTCW_GFX_NO_SWAP
                    d[j] = (uint8_t)(fg >> 8);
                    d[j + 1] = (uint8_t)fg;
#else
                    d[j] = (uint8_t)fg;
                    d[j + 1] = (uint8_t)(fg >> 8);
#endif
                    continue;
                }

#ifndef HTCW_GFX_NO_SWAP
                const uint16_t bg = ((uint16_t)d[j] << 8) | (uint16_t)d[j + 1];
#else
                const uint16_t bg = (uint16_t)d[j] | ((uint16_t)d[j + 1] << 8);
#endif
                uint32_t a5 = (uint32_t)(sa + 4) >> 3;
                if (a5 > 31) a5 = 31;

                const uint32_t fg_s = (uint32_t)(fg & 0xF81F) | ((uint32_t)(fg & 0x07E0) << 16);
                const uint32_t bg_s = (uint32_t)(bg & 0xF81F) | ((uint32_t)(bg & 0x07E0) << 16);
                const uint32_t bl = (bg_s + (((fg_s - bg_s) * a5) >> 5)) & 0x07E0F81F;
                const uint16_t out = (uint16_t)((bl & 0xFFFF) | (bl >> 16));

#ifndef HTCW_GFX_NO_SWAP
                d[j] = (uint8_t)(out >> 8);
                d[j + 1] = (uint8_t)out;
#else
                d[j] = (uint8_t)out;
                d[j + 1] = (uint8_t)(out >> 8);
#endif
            }
            if (k == 0) ++i;  // no forward progress guard
        }
        return gfx_result::success;
    }
    template <typename Destination, typename Source>
    static gfx_result blend_rgba8888_to_rgb888(Destination& destination, spoint16 location,
                                               const Source source, spoint16 source_location, size_t width) {
        gfx_cspan src_span = source.cspan((point16)source_location);
        const uint8_t* src = src_span.cdata;
        if (src == nullptr) return gfx_result::success;
        const int16_t minx = location.x, py = location.y;
        const int16_t row_w = (int16_t)width;

        // out = bg + round((fg-bg)*a / 255), branch-free, one multiply.
        // (fg-bg)*a is signed in [-65025,65025]; add 255*256 to make the
        // rounding divide operate on a non-negative value, then remove the
        // 256 that adds to the quotient.
        auto lerp8 = [](uint8_t fg, uint8_t bg, uint8_t a) -> uint8_t {
            int32_t t = ((int)fg - (int)bg) * (int)a;  // one MULL
            uint32_t u = (uint32_t)(t + 65280) + 128;  // +255*256, +round bias
            u = (u + (u >> 8)) >> 8;                   // == round((t+65280)/255)
            return (uint8_t)((int)bg + (int)u - 256);
        };

        int i = 0;
        while (i < row_w) {
            gfx_span span = destination.span(point16((int16_t)(minx + i), py));
            if (span.length == 0 || span.data == nullptr) {
                ++i;  // column outside bitmap: skip src[i] too
                continue;
            }
            uint8_t* d = span.data;
            const int run = (int)(span.length / 3);  // dest pixels available (3 bytes each)
            int k = 0;                               // pixel index within this span
            for (; k < run && i < row_w; ++k, ++i) {
                const uint8_t* s = src + ((size_t)i << 2);  // 4 bytes per RGBA pixel
#ifndef HTCW_GFX_NO_SWAP
                const uint8_t sr = s[0], sg = s[1], sb = s[2], sa = s[3];
#else
                const uint8_t sa = s[0], sb = s[1], sg = s[2], sr = s[3];
#endif
                if (sa == 0) continue;

                const int j = k * 3;  // byte offset within THIS span

                if (sa >= 255) {  // fully opaque: copy source RGB
#ifndef HTCW_GFX_NO_SWAP
                    d[j] = sr;
                    d[j + 1] = sg;
                    d[j + 2] = sb;
#else
                    d[j] = sb;
                    d[j + 1] = sg;
                    d[j + 2] = sr;
#endif
                    continue;
                }

#ifndef HTCW_GFX_NO_SWAP
                const uint8_t dr = d[j], dg = d[j + 1], db = d[j + 2];
#else
                const uint8_t db = d[j], dg = d[j + 1], dr = d[j + 2];
#endif
                const uint8_t orr = lerp8(sr, dr, sa);
                const uint8_t og = lerp8(sg, dg, sa);
                const uint8_t ob = lerp8(sb, db, sa);

#ifndef HTCW_GFX_NO_SWAP
                d[j] = orr;
                d[j + 1] = og;
                d[j + 2] = ob;
#else
                d[j] = ob;
                d[j + 1] = og;
                d[j + 2] = orr;
#endif
            }
            if (k == 0) ++i;  // no forward progress guard
        }
        return gfx_result::success;
    }
    static bool venn_rects(size16 dst_dim, const srect16& dst_rect, size16 src_dim, const rect16& src_rect, const srect16* clip, rect_info* out_result) {
        srect16 dstr = dst_rect.normalize();
        rect16 srcr = src_rect.normalize().crop(src_dim.bounds());
        if (!srcr.intersects(src_dim.bounds()) || !dstr.intersects((srect16)dst_dim.bounds())) {
            return false;
        }
        int sofx = srcr.x1, sofy = srcr.y1;
        if (dstr.x1 < 0) {
            sofx += -dstr.x1;
            dstr.x1 = 0;
        }
        if (dstr.y1 < 0) {
            sofy += -dstr.y1;
            dstr.y1 = 0;
        }
        if (clip != nullptr) {
            dstr = dstr.crop(*clip);
        }
        if (!dstr.intersects((srect16)dst_dim.bounds())) {
            return false;
        }
        out_result->dst_loc = (point16)dstr.point1();
        out_result->src_loc = point16(sofx, sofy);
        out_result->width = math::min_(dstr.x2 - dstr.x1 + 1, srcr.width() - sofx);
        out_result->height = math::min_(dstr.y2 - dstr.y1 + 1, srcr.height() - sofx);
        return true;
    }
    template <typename Destination, typename Source, bool CopyFrom, bool CopyTo, bool BltDst, bool BltSrc>
    struct draw_bmp_caps_helper {
    };

    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, true, false, true, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return source.copy_to(src_rect, destination, location);
        }
    };
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, true, false, false, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return destination.copy_from(src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, true, false, false, false> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return destination.copy_from(src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, false, false, false, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            // using thas_alpha = typename Source::pixel_type::template has_channel_names<channel_name::A>;
            // using tdhas_alpha = typename Destination::pixel_type::template has_channel_names<channel_name::A>;
            return copy_from_impl_helper<Destination, Source>::do_draw(destination, src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, false, false, true, false> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return copy_from_impl_helper<Destination, Source>::do_draw(destination, src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, true, true, false, false> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            // using thas_alpha = typename Source::pixel_type::template has_channel_names<channel_name::A>;
            // using tdhas_alpha = typename Destination::pixel_type::template has_channel_names<channel_name::A>;
            return copy_from_impl_helper<Destination, Source>::do_draw(destination, src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    // copyfrom, copyto, bltdst, bltsrc, async
    struct draw_bmp_caps_helper<Destination, Source, true, true, true, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return destination.copy_from(src_rect, source, location);
        }
    };
    // copyfrom, copyto, bltdst, bltsrc, async
    template <typename Destination, typename Source>
    struct draw_bmp_caps_helper<Destination, Source, true, true, false, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return destination.copy_from(src_rect, source, location);
        }
    };
    template <typename Destination, typename Source>
    // copyfrom, copyto, bltdst, bltsrc, async
    struct draw_bmp_caps_helper<Destination, Source, false, true, false, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return source.copy_to(src_rect, destination, location);
        }
    };

    template <typename Destination, typename Source>
    // copyfrom, copyto, bltdst, bltsrc, async
    struct draw_bmp_caps_helper<Destination, Source, false, false, false, false> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& srcr, gfx::point16 location) {
            int o = (int)rect_orientation::normalized;
            rect16 dstr(location, srcr.dimensions());
            // do cropping
            bool flipX = (int)rect_orientation::flipped_horizontal == (o & (int)rect_orientation::flipped_horizontal);
            bool flipY = (int)rect_orientation::flipped_vertical == (o & (int)rect_orientation::flipped_vertical);

            uint16_t x2 = srcr.x1;
            uint16_t y2 = srcr.y1;
            for (int y = dstr.y1; y <= dstr.y2; ++y) {
                for (int x = dstr.x1; x <= dstr.x2; ++x) {
                    typename Source::pixel_type px;
                    uint16_t ssx = x2;
                    uint16_t ssy = y2;
                    if (flipX) {
                        ssx = srcr.x2 - x2;
                    }
                    if (flipY) {
                        ssy = srcr.y2 - y2;
                    }
                    gfx_result r = source.point(point16(ssx, ssy), &px);
                    if (gfx_result::success != r)
                        return r;
                    typename Destination::pixel_type px2 = convert<typename Source::pixel_type, typename Destination::pixel_type>(px);
                    r = xdraw_point::point(destination, spoint16(x, y), px2, nullptr);
                    if (gfx_result::success != r)
                        return r;
                    ++x2;
                    if (x2 > srcr.x2)
                        break;
                }
                x2 = srcr.x1;
                ++y2;
                if (y2 > srcr.y2)
                    break;
            }
            return gfx_result::success;
        }
    };
    template <typename Destination, typename Source>
    // copyfrom, copyto, bltdst, bltsrc, async
    struct draw_bmp_caps_helper<Destination, Source, false, true, true, true> {
        inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect, gfx::point16 location) {
            return source.copy_to(src_rect, destination, location);
        }
    };

    template <typename Destination, typename Source>
    static gfx_result draw_bitmap_impl(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type, const typename Source::pixel_type* transparent_color, const srect16* clip) {
        gfx_result r;
        rect16 srcr = source_rect.normalize().crop(source.bounds());
        srect16 dsr = dest_rect.crop((srect16)destination.bounds()).normalize();
        if (nullptr != clip) {
            dsr = dsr.crop(*clip);
        }
        // rect16 ddr = (rect16)dsr;
        int o = (int)dest_rect.orientation();
        const int w = dest_rect.width(), h = dest_rect.height();

        using thas_alpha = typename Source::pixel_type::template has_channel_names<channel_name::A>;
        const bool has_alpha = thas_alpha::value;
        if (bitmap_resize::crop == resize_type) {
            for (int y = 0; y < h; ++y) {
                int yy = y + dest_rect.top();
                if (yy < dsr.y1)
                    continue;
                else if (yy > dsr.y2)
                    break;

                int syy = y + srcr.top();
                if ((int)rect_orientation::flipped_vertical == ((int)rect_orientation::flipped_vertical & o)) {
                    syy = srcr.bottom() - y;
                }
                if constexpr (Destination::caps::blt_spans && helpers::is_same<typename Destination::pixel_type, rgb_pixel<16>>::value && Source::caps::blt_spans && helpers::is_same<typename Source::pixel_type, rgba_pixel<32>>::value) {
                    int sxx = srcr.left();
                    if ((int)rect_orientation::normalized == o && rect_orientation::normalized == dest_rect.orientation()) {
                        blend_rgba8888_to_rgb565(destination, spoint16(dest_rect.left(), yy), source, spoint16(sxx, syy), w);
                    } else {
                        r = draw_bitmap_crop_span<has_alpha>(destination, dest_rect, dsr, source, srcr, w, yy, syy, o, transparent_color);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                } else if constexpr (Destination::caps::blt_spans && helpers::is_same<typename Destination::pixel_type, rgb_pixel<24>>::value && Source::caps::blt_spans && helpers::is_same<typename Source::pixel_type, rgba_pixel<32>>::value) {
                    int sxx = srcr.left();
                    if ((int)rect_orientation::normalized == o && rect_orientation::normalized == dest_rect.orientation()) {
                        blend_rgba8888_to_rgb888(destination, spoint16(dest_rect.left(), yy), source, spoint16(sxx, syy), w);
                    } else {
                        r = draw_bitmap_crop_span<has_alpha>(destination, dest_rect, dsr, source, srcr, w, yy, syy, o, transparent_color);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                } else {
                    r = draw_bitmap_crop_span<has_alpha>(destination, dest_rect, dsr, source, srcr, w, yy, syy, o, transparent_color);
                    if (gfx_result::success != r) {
                        return r;
                    }
               }
            }
        } else {  // resize
            for (int y = 0; y < h; ++y) {
                int yy = y + dest_rect.top();
                if (yy < dsr.y1)
                    continue;
                else if (yy > dsr.y2)
                    break;

                float v = float(y) / float(h - 1);
                if ((int)rect_orientation::flipped_vertical == ((int)rect_orientation::flipped_vertical & o)) {
                    v = 1.0 - v;
                }
                for (int x = 0; x < w; ++x) {
                    int xx = x + dest_rect.left();
                    if (xx < dsr.x1) {
                        continue;
                    } else if (xx > dsr.x2)
                        break;
                    spoint16 spt(xx, yy);
                    float u = float(x) / float(w - 1.0f);
                    if ((int)rect_orientation::flipped_horizontal == ((int)rect_orientation::flipped_horizontal & o)) {
                        u = 1.0f - u;
                    }
                    typename Source::pixel_type sampx;

                    switch (resize_type) {
                        case bitmap_resize::resize_bicubic:
                            r = helpers::sample_bicubic(source, srcr, u, v, &sampx);
                            if (gfx_result::success != r) {
                                return r;
                            }
                            break;
                        case bitmap_resize::resize_bilinear:
                            r = helpers::sample_bilinear(source, srcr, u, v, &sampx);
                            if (gfx_result::success != r) {
                                return r;
                            }
                            break;
                        default:
                            r = helpers::sample_nearest(source, srcr, u, v, &sampx);
                            if (gfx_result::success != r) {
                                return r;
                            }
                            break;
                    }
                    if (!has_alpha && nullptr == transparent_color) {
                        typename Destination::pixel_type px;
                        r = convert_palette(destination, source, sampx, &px);
                        if (gfx_result::success != r) {
                            return r;
                        }
                        r = destination.point((point16)spt, px);
                    } else if (nullptr == transparent_color || transparent_color->native_value != sampx.native_value) {
                        // already clipped, hence nullptr
                        r = helpers::blender<Destination, Source, has_alpha>::point(destination, (point16)spt, source, point16(x + srcr.x1, y + srcr.y1), sampx);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                }
            }
        }
        return gfx_result::success;
    }
    template <typename Destination, typename Source, typename DestinationPixelType, typename SourcePixelType>
    struct bmp_helper {
        inline static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type, const typename Source::pixel_type* transparent_color, const srect16* clip) {
            return draw_bitmap_impl(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
        }
    };

    template <typename Destination, typename Source, typename PixelType>
    struct bmp_helper<Destination, Source, PixelType, PixelType> {
        static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type, const typename Source::pixel_type* transparent_color, const srect16* clip) {
            const bool optimized = (Destination::caps::blt && Source::caps::blt) || (Destination::caps::copy_from || Source::caps::copy_to);

            // disqualify fast blting
            if (!optimized || transparent_color != nullptr || dest_rect.x1 > dest_rect.x2 ||
                dest_rect.y1 > dest_rect.y2 ||
                ((bitmap_resize::crop != resize_type) &&
                 (dest_rect.width() != source_rect.width() ||
                  dest_rect.height() != source_rect.height()))) {
                return draw_bitmap_impl(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
            }

            rect16 dr;
            if (!draw_translate_adjust(dest_rect, &dr))
                return gfx_result::success;  // whole thing is offscreen
            point16 loc(0, 0);
            rect16 ccr;
            if (nullptr != clip) {
                if (!draw_translate_adjust(*clip, &ccr))
                    return gfx_result::success;  // clip rect is off screen
                dr = dr.crop(ccr);
            }

            size16 dim(dr.width(), dr.height());
            if (dim.width > source_rect.width())
                dim.width = source_rect.width();
            if (dim.height > source_rect.height())
                dim.height = source_rect.height();
            if (0 > dest_rect.x1) {
                loc.x = -dest_rect.x1;
                if (nullptr == clip)
                    dim.width += dest_rect.x1;
            }
            if (0 > dest_rect.y1) {
                loc.y = -dest_rect.y1;
                if (nullptr == clip)
                    dim.height += dest_rect.y1;
            }
            loc.x += source_rect.left();
            loc.y += source_rect.top();
            rect16 r = rect16(loc, dim);
            return draw_bmp_caps_helper<Destination, Source, Destination::caps::copy_from, Source::caps::copy_to, Destination::caps::blt, Source::caps::blt>::do_draw(destination, source, r, dr.top_left());
        }
    };
    template <typename Destination, typename Source, bool BltSpanDst, bool BltSpanSrc>
    struct bmp_blt_span_helper {
        static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type, const typename Source::pixel_type* transparent_color, const srect16* clip) {
            return bmp_helper<Destination, Source, typename Destination::pixel_type, typename Source::pixel_type>::draw_bitmap(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
        }
    };

    template <typename Destination, typename Source>
    struct bmp_blt_span_helper<Destination, Source, true, true> {
        static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type, const typename Source::pixel_type* transparent_color, const srect16* clip) {
            // rule out optimization
            if (resize_type != bitmap_resize::crop || transparent_color != nullptr) {
                return bmp_helper<Destination, Source, typename Destination::pixel_type, typename Source::pixel_type>::draw_bitmap(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
            }
            rect_info ri;
            if (!venn_rects(destination.dimensions(), dest_rect, source.dimensions(), source_rect, clip, &ri)) {
                return gfx_result::success;
            }

            blt_span& dst = destination;
            const const_blt_span& src = source;
            int16_t sx = ri.src_loc.x;
            int16_t sy = ri.src_loc.y;
            int16_t d_offsx = ri.dst_loc.x;
            int16_t d_offsy = ri.dst_loc.y;
            uint16_t dxe = ri.width;
            uint16_t dye = ri.height;
            if (helpers::is_same<typename Destination::pixel_type, typename Source::pixel_type>::value && !Source::pixel_type::has_alpha) {
                for (int y = 0; y < dye; ++y) {
                    int x = d_offsx;
                    gfx_span dst_sp = dst.span(point16(x, y + d_offsy));
                    gfx_cspan src_csp = src.cspan(point16(sx, sy));
                    if (dst_sp.data != nullptr && src_csp.cdata != nullptr) {
                        int len = math::min_((int)(dxe * dst.pixel_width()), (int)dst_sp.length, (int)src_csp.length);
                        memcpy(dst_sp.data, src_csp.cdata, len);
                    }
                    ++sy;
                }
            } else {
                return bmp_blt_span_helper<Destination, Source, false, false>::draw_bitmap(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
            }
            return gfx_result::success;
        }
    };

   public:
    // draws a portion of a bitmap or display buffer to the specified rectangle with an optional clipping rentangle
    template <typename Destination, typename Source>
    static inline gfx_result bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type = bitmap_resize::crop, const typename Source::pixel_type* transparent_color = nullptr, const srect16* clip = nullptr) {
        return bmp_blt_span_helper<Destination, Source, Destination::caps::blt_spans, Source::caps::blt_spans>::draw_bitmap(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
        // return bmp_helper<Destination,Source,typename Destination::pixel_type,typename Source::pixel_type>::draw_bitmap(destination, dest_rect, source, source_rect, resize_type, transparent_color, clip);
    }
    // draws a portion of a bitmap or display buffer to the specified rectangle with an optional clipping rentangle
    template <typename Destination, typename Source>
    static inline gfx_result bitmap(Destination& destination, const rect16& dest_rect, Source& source, const rect16& source_rect, bitmap_resize resize_type = bitmap_resize::crop, const typename Source::pixel_type* transparent_color = nullptr, const srect16* clip = nullptr) {
        return bitmap(destination, (srect16)dest_rect, source, source_rect, resize_type, transparent_color, clip);
    }
};
}  // namespace helpers
}  // namespace gfx
#endif