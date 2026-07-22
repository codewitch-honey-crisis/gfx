#ifndef HTCW_GFX_DRAW_COMMON_HPP
#define HTCW_GFX_DRAW_COMMON_HPP
#include <gfx_core.hpp>
#include <gfx_palette.hpp>
#include <gfx_pixel.hpp>
#include <gfx_positioning.hpp>
namespace gfx {
namespace helpers {
bool draw_translate(spoint16 in, point16* out);
bool draw_translate(size16 in, ssize16* out);
bool draw_translate(const srect16& in, rect16* out);
bool draw_translate_adjust(const srect16& in, rect16* out);
}
}
#include <gfx_draw_point.hpp>
namespace gfx {
namespace helpers {

template <typename Source, typename Destination>
struct blend_helper {
    static constexpr gfx_result do_blend(
        const Source& src, typename Source::pixel_type spx, Destination& dst,
        point16 dstpnt, typename Destination::pixel_type* out_px) {
        gfx_result r = gfx_result::success;
        typename Destination::pixel_type bgpx;
        r = dst.point(point16(dstpnt), &bgpx);
        if (gfx_result::success != r) {
            return r;
        }
        r = convert_palette(dst, src, spx, out_px, &bgpx);
        if (gfx_result::success != r) {
            return r;
        }
        return gfx_result::success;
    }
};
template <typename Destination, typename Source, bool HasAlpha>
struct blender {
    static gfx_result point(Destination& destination, point16 pt,
                            Source& source, point16 spt,
                            typename Source::pixel_type pixel) {
        (void)spt;
        typename Destination::pixel_type px;
        gfx_result r = convert_palette(destination, source, pixel, &px);
        if (gfx_result::success != r) {
            return r;
        }
        return destination.point(pt, px);
    }
};
template <typename Destination, typename Source>
struct blender<Destination, Source, true> {
    static gfx_result point(Destination& destination, point16 pt,
                            Source& source, point16 spt,
                            typename Source::pixel_type pixel) {
        auto alpha = pixel.opacity8();
        if (0 == alpha) return gfx_result::success;
        if (255 == alpha)
            return blender<Destination, Source, false>::point(
                destination, pt, source, spt, pixel);
        typename Source::pixel_type spx;
        gfx_result r = source.point(spt, &spx);
        if (gfx_result::success != r) {
            return r;
        }
        rgb_pixel<HTCW_MAX_WORD> bg;
        r = convert_palette_to(source, spx, &bg);
        if (gfx_result::success != r) {
            return r;
        }
        rgb_pixel<HTCW_MAX_WORD> fg;
        r = convert_palette_to(source, pixel, &fg);
        if (gfx_result::success != r) {
            return r;
        }
        r = fg.blend8(bg, alpha, &fg);
        if (gfx_result::success != r) {
            return r;
        }
        typename Destination::pixel_type px;
        r = convert_palette_from(destination, fg, &px);
        if (gfx_result::success != r) {
            return r;
        }
        return destination.point(pt, px);
    }
};
template <typename Destination, typename Source, bool Async>
struct copy_to_helper {};
template <typename Destination, typename Source>
struct copy_to_helper<Destination, Source, false> {
    static gfx_result do_draw(const Source& src, const rect16& rct,
                              Destination& dst, point16 loc) {
        return src.copy_to(rct, dst, loc);
    }
};
template <typename Destination, typename Source>
struct copy_to_helper<Destination, Source, true> {
    static gfx_result do_draw(const Source& src, const rect16& rct,
                              Destination& dst, point16 loc) {
        return src.copy_to_async(rct, dst, loc);
    }
};

template <typename Destination, typename Source>
struct copy_from_impl_helper {
    static gfx_result do_draw(Destination& destination, const rect16& src_rect,
                              const Source& src, point16 location) {
        gfx_result r;
        rect16 srcr = src_rect.normalize().crop(src.bounds());
        rect16 dstr(location, src_rect.dimensions());
        dstr = dstr.crop(destination.bounds());
        if (srcr.width() > dstr.width()) {
            srcr.x2 = srcr.x1 + dstr.width() - 1;
        }
        if (srcr.height() > dstr.height()) {
            srcr.y2 = srcr.y1 + dstr.height() - 1;
        }
        uint16_t w = dstr.dimensions().width;
        uint16_t h = dstr.dimensions().height;
        for (uint16_t y = 0; y < h; ++y) {
            for (uint16_t x = 0; x < w; ++x) {
                typename Source::pixel_type pp;
                r = src.point(point16(x + srcr.x1, y + srcr.y1), &pp);
                if (r != gfx_result::success) return r;
                typename Destination::pixel_type p;
                r = convert_palette_from(destination, pp, &p);
                if (gfx_result::success != r) {
                    return r;
                }
                r = xdraw_point::point(destination, spoint16(x, y), p, nullptr);
                if (gfx_result::success != r) {
                    return r;
                }
            }
        }
        return gfx_result::success;
    }
};
template <typename Destination, typename Source, bool CopyTo>
struct copy_to_fast {};
template <typename Destination, typename Source>
struct copy_to_fast<Destination, Source, false> {
    static gfx_result do_copy(Destination& dst, const Source& src,
                              const rect16& src_rect, point16 location) {
        return copy_from_impl_helper<Destination, Source>::do_draw(
            dst, src_rect, src, location);
    }
};
template <typename Destination, typename Source>
struct copy_to_fast<Destination, Source, true> {
    static gfx_result do_copy(Destination& dst, const Source& src,
                              const rect16& src_rect, point16 location) {
        return src.copy_to(src_rect, dst, location);
    }
};
float cubic_hermite(float A, float B, float C, float D, float t);

bool clamp_point16(point16& pt, const rect16& bounds);

template <typename Source>
gfx_result sample_nearest(const Source& source, const rect16& src_rect, float u,
                          float v, typename Source::pixel_type* out_pixel) {
    // calculate coordinates

    point16 pt(uint16_t(u * src_rect.width() + src_rect.left()),
               uint16_t(v * src_rect.height() + src_rect.top()));

    clamp_point16(pt, src_rect);
    // return pixel
    return source.point(pt, out_pixel);
}
template <typename Source>
gfx_result sample_bilinear(const Source& source, const rect16& src_rect,
                           float u, float v,
                           typename Source::pixel_type* out_pixel) {
    using pixel_type = typename Source::pixel_type;
    using rgba_type = rgba_pixel<HTCW_MAX_WORD>;
    // calculate coordinates -> also need to offset by half a pixel to keep
    // image from shifting down and left half a pixel
    float x = (u * src_rect.width()) - 0.5f + src_rect.left();
    int xint = int(x);
    float xfract = x - floorf(x);

    float y = (v * src_rect.height()) - 0.5f + src_rect.top();
    int yint = int(y);
    float yfract = y - floorf(y);
    gfx_result r;
    // get pixels

    point16 pt00(uint16_t(xint + 0), uint16_t(yint + 0));
    point16 pt10(uint16_t(xint + 1), uint16_t(yint + 0));
    point16 pt01(uint16_t(xint + 0), uint16_t(yint + 1));
    point16 pt11(uint16_t(xint + 1), uint16_t(yint + 1));
    pixel_type px00, px10, px01, px11;
    rgba_type cpx00, cpx10, cpx01, cpx11;
    clamp_point16(pt00, src_rect);
    clamp_point16(pt10, src_rect);
    clamp_point16(pt01, src_rect);
    clamp_point16(pt11, src_rect);
    r = source.point(pt00, &px00);
    if (gfx_result::success != r) {
        return r;
    }
    r = convert_palette_to(source, px00, &cpx00);
    if (gfx_result::success != r) {
        return r;
    }
    r = source.point(pt10, &px10);
    if (gfx_result::success != r) {
        return r;
    }
    r = convert_palette_to(source, px10, &cpx10);
    if (gfx_result::success != r) {
        return r;
    }
    r = source.point(pt01, &px01);
    if (gfx_result::success != r) {
        return r;
    }
    r = convert_palette_to(source, px01, &cpx01);
    if (gfx_result::success != r) {
        return r;
    }
    r = source.point(pt11, &px11);
    if (gfx_result::success != r) {
        return r;
    }
    r = convert_palette_to(source, px11, &cpx11);
    if (gfx_result::success != r) {
        return r;
    }
    // interpolate bi-linearly!
    rgba_type ccol0;
    r = cpx00.blend(cpx10, xfract, &ccol0);
    if (gfx_result::success != r) {
        return r;
    }
    rgba_type ccol1;
    r = cpx01.blend(cpx11, xfract, &ccol1);
    if (gfx_result::success != r) {
        return r;
    }
    rgba_type ccol2;
    r = ccol0.blend(ccol1, yfract, &ccol2);
    if (gfx_result::success != r) {
        return r;
    }
    r = convert_palette_from(source, ccol2, out_pixel);
    return gfx_result::success;
}
template <typename Source>
gfx_result sample_bicubic(const Source& source, const rect16& src_rect, float u,
                          float v, typename Source::pixel_type* out_pixel) {
    using pixel_type = typename Source::pixel_type;
    using rgba_type = rgba_pixel<HTCW_MAX_WORD>;
    // calculate coordinates -> also need to offset by half a pixel to keep
    // image from shifting down and left half a pixel
    float x = (u * src_rect.width()) - 0.5f + src_rect.left();
    int xint = int(x);
    float xfract = x - floorf(x);

    float y = (v * src_rect.height()) - 0.5f + src_rect.top();
    int yint = int(y);
    float yfract = y - floorf(y);

    gfx_result r;

    point16 pt00(uint16_t(xint - 1), uint16_t(yint - 1)), pt10(uint16_t(xint + 0), uint16_t(yint - 1)),
        pt20(uint16_t(xint + 1), uint16_t(yint - 1)), pt30(uint16_t(xint + 2), uint16_t(yint - 1)),
        pt01(uint16_t(xint - 1), uint16_t(yint + 0)), pt11(uint16_t(xint + 0), uint16_t(yint + 0)),
        pt21(uint16_t(xint + 1), uint16_t(yint + 0)), pt31(uint16_t(xint + 2), uint16_t(yint + 0)),
        pt02(uint16_t(xint - 1), uint16_t(yint + 1)), pt12(uint16_t(xint + 0), uint16_t(yint + 1)),
        pt22(uint16_t(xint + 1), uint16_t(yint + 1)), pt32(uint16_t(xint + 2), uint16_t(yint + 1)),
        pt03(uint16_t(xint - 1), uint16_t(yint + 2)), pt13(uint16_t(xint + 0), uint16_t(yint + 2)),
        pt23(uint16_t(xint + 1), uint16_t(yint + 2)), pt33(uint16_t(xint + 2), uint16_t(yint + 2));

    pixel_type px00, px10, px20, px30, px01, px11, px21, px31, px02, px12, px22,
        px32, px03, px13, px23, px33;
    rgba_type cpx00, cpx10, cpx20, cpx30, cpx01, cpx11, cpx21, cpx31, cpx02,
        cpx12, cpx22, cpx32, cpx03, cpx13, cpx23, cpx33;

    rgba_type rpx;

    float d0, d1, d2, d3;

    // interpolate bi-cubically!

    // 1st row
    clamp_point16(pt00, src_rect);
    clamp_point16(pt10, src_rect);
    clamp_point16(pt20, src_rect);
    clamp_point16(pt30, src_rect);
    r = source.point(pt00, &px00);
    if (gfx_result::success != r) return r;
    r = source.point(pt10, &px10);
    if (gfx_result::success != r) return r;
    r = source.point(pt20, &px20);
    if (gfx_result::success != r) return r;
    r = source.point(pt30, &px30);
    if (gfx_result::success != r) return r;

    r = (convert_palette_to(source, px00, &cpx00));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px10, &cpx10));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px20, &cpx20));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px30, &cpx30));
    if (gfx_result::success != r) return r;

    // 2nd row
    clamp_point16(pt01, src_rect);
    clamp_point16(pt11, src_rect);
    clamp_point16(pt21, src_rect);
    clamp_point16(pt31, src_rect);
    r = source.point(pt01, &px01);
    if (gfx_result::success != r) return r;
    r = source.point(pt11, &px11);
    if (gfx_result::success != r) return r;
    r = source.point(pt21, &px21);
    if (gfx_result::success != r) return r;
    r = source.point(pt31, &px31);
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px01, &cpx01));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px11, &cpx11));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px21, &cpx21));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px31, &cpx31));
    if (gfx_result::success != r) return r;

    // 3rd row
    clamp_point16(pt02, src_rect);
    clamp_point16(pt12, src_rect);
    clamp_point16(pt22, src_rect);
    clamp_point16(pt32, src_rect);
    r = source.point(pt02, &px02);
    if (gfx_result::success != r) return r;
    r = source.point(pt12, &px12);
    if (gfx_result::success != r) return r;
    r = source.point(pt22, &px22);
    if (gfx_result::success != r) return r;
    r = source.point(pt32, &px32);
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px02, &cpx02));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px12, &cpx12));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px22, &cpx22));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px32, &cpx32));
    if (gfx_result::success != r) return r;

    // 4th row
    clamp_point16(pt03, src_rect);
    clamp_point16(pt13, src_rect);
    clamp_point16(pt23, src_rect);
    clamp_point16(pt33, src_rect);
    r = source.point(pt03, &px03);
    if (gfx_result::success != r) return r;
    r = source.point(pt13, &px13);
    if (gfx_result::success != r) return r;
    r = source.point(pt23, &px23);
    if (gfx_result::success != r) return r;
    r = source.point(pt33, &px33);
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px03, &cpx03));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px13, &cpx13));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px23, &cpx23));
    if (gfx_result::success != r) return r;
    r = (convert_palette_to(source, px33, &cpx33));
    if (gfx_result::success != r) return r;

    // Clamp the values since the curve can put the value below 0 or above 1
    const int chiR = rgba_type::channel_index_by_name<channel_name::R>::value;
    d0 = cubic_hermite((float)cpx00.template channelr_unchecked<chiR>(),
                       (float)cpx10.template channelr_unchecked<chiR>(),
                       (float)cpx20.template channelr_unchecked<chiR>(),
                       (float)cpx30.template channelr_unchecked<chiR>(), xfract);
    d1 = cubic_hermite((float)cpx01.template channelr_unchecked<chiR>(),
                       (float)cpx11.template channelr_unchecked<chiR>(),
                       (float)cpx21.template channelr_unchecked<chiR>(),
                       (float)cpx31.template channelr_unchecked<chiR>(), xfract);
    d2 = cubic_hermite((float)cpx02.template channelr_unchecked<chiR>(),
                       (float)cpx12.template channelr_unchecked<chiR>(),
                       (float)cpx22.template channelr_unchecked<chiR>(),
                       (float)cpx32.template channelr_unchecked<chiR>(), xfract);
    d3 = cubic_hermite((float)cpx03.template channelr_unchecked<chiR>(),
                       (float)cpx13.template channelr_unchecked<chiR>(),
                       (float)cpx23.template channelr_unchecked<chiR>(),
                       (float)cpx33.template channelr_unchecked<chiR>(), xfract);
    rpx.channelr<channel_name::R>(helpers::clamp(
        cubic_hermite(d0, d1, d2, d3, yfract), 0.0f, 1.0f));

    const size_t chiG =
        rgba_type::channel_index_by_name<channel_name::G>::value;
    d0 = cubic_hermite((float)cpx00.template channelr_unchecked<chiG>(),
                       (float)cpx10.template channelr_unchecked<chiG>(),
                       (float)cpx20.template channelr_unchecked<chiG>(),
                       (float)cpx30.template channelr_unchecked<chiG>(), xfract);
    d1 = cubic_hermite((float)cpx01.template channelr_unchecked<chiG>(),
                       (float)cpx11.template channelr_unchecked<chiG>(),
                       (float)cpx21.template channelr_unchecked<chiG>(),
                       (float)cpx31.template channelr_unchecked<chiG>(), xfract);
    d2 = cubic_hermite((float)cpx02.template channelr_unchecked<chiG>(),
                       (float)cpx12.template channelr_unchecked<chiG>(),
                       (float)cpx22.template channelr_unchecked<chiG>(),
                       (float)cpx32.template channelr_unchecked<chiG>(), xfract);
    d3 = cubic_hermite((float)cpx03.template channelr_unchecked<chiG>(),
                       (float)cpx13.template channelr_unchecked<chiG>(),
                       (float)cpx23.template channelr_unchecked<chiG>(),
                       (float)cpx33.template channelr_unchecked<chiG>(), xfract);
    rpx.channelr<channel_name::G>(helpers::clamp(
        cubic_hermite(d0, d1, d2, d3, yfract), 0.0f, 1.0f));

    const int chiB = rgba_type::channel_index_by_name<channel_name::B>::value;
    d0 = cubic_hermite((float)cpx00.template channelr_unchecked<chiB>(),
                       (float)cpx10.template channelr_unchecked<chiB>(),
                       (float)cpx20.template channelr_unchecked<chiB>(),
                       (float)cpx30.template channelr_unchecked<chiB>(), xfract);
    d1 = cubic_hermite((float)cpx01.template channelr_unchecked<chiB>(),
                       (float)cpx11.template channelr_unchecked<chiB>(),
                       (float)cpx21.template channelr_unchecked<chiB>(),
                       (float)cpx31.template channelr_unchecked<chiB>(), xfract);
    d2 = cubic_hermite((float)cpx02.template channelr_unchecked<chiB>(),
                       (float)cpx12.template channelr_unchecked<chiB>(),
                       (float)cpx22.template channelr_unchecked<chiB>(),
                       (float)cpx32.template channelr_unchecked<chiB>(), xfract);
    d3 = cubic_hermite((float)cpx03.template channelr_unchecked<chiB>(),
                       (float)cpx13.template channelr_unchecked<chiB>(),
                       (float)cpx23.template channelr_unchecked<chiB>(),
                       (float)cpx33.template channelr_unchecked<chiB>(), xfract);
    rpx.channelr<channel_name::B>(helpers::clamp(
        cubic_hermite(d0, d1, d2, d3, yfract), 0.0f, 1.0f));

    const size_t chiA = rgba_type::channel_index_by_name<channel_name::A>::value;
    if constexpr (-1 < chiA) {
        d0 = cubic_hermite(cpx00.template channelr_unchecked<chiA>(),
                           cpx10.template channelr_unchecked<chiA>(),
                           cpx20.template channelr_unchecked<chiA>(),
                           cpx30.template channelr_unchecked<chiA>(), xfract);
        d1 = cubic_hermite(cpx01.template channelr_unchecked<chiA>(),
                           cpx11.template channelr_unchecked<chiA>(),
                           cpx21.template channelr_unchecked<chiA>(),
                           cpx31.template channelr_unchecked<chiA>(), xfract);
        d2 = cubic_hermite(cpx02.template channelr_unchecked<chiA>(),
                           cpx12.template channelr_unchecked<chiA>(),
                           cpx22.template channelr_unchecked<chiA>(),
                           cpx32.template channelr_unchecked<chiA>(), xfract);
        d3 = cubic_hermite(cpx03.template channelr_unchecked<chiA>(),
                           cpx13.template channelr_unchecked<chiA>(),
                           cpx23.template channelr_unchecked<chiA>(),
                           cpx33.template channelr_unchecked<chiA>(), xfract);
        rpx.channelr<channel_name::A>(helpers::clamp(
            cubic_hermite(d0, d1, d2, d3, yfract), 0.0f, 1.0f));
    }
    return convert_palette_from(source, rpx, out_pixel);
}

}  // namespace helpers
// changes the target's pixel type to an intermediary pixel type and back again.
// Useful for downsampling or converting to grayscale
template <typename TargetType, typename PixelType>
gfx_result resample(TargetType& target) {
    size16 dim = target.dimensions();
    point16 pt;
    gfx_result r;
    for (pt.y = 0; pt.y < dim.width; ++pt.y) {
        for (pt.x = 0; pt.x < dim.height; ++pt.x) {
            typename TargetType::pixel_type px;
            r = target.point(pt, &px);
            if (gfx_result::success != r) {
                return r;
            }
            PixelType dpx;
            r = convert_palette_to(target, px, &dpx);
            if (gfx_result::success != r) {
                return r;
            }
            r = convert_palette_to(target, dpx, &px);
            if (gfx_result::success != r) {
                return r;
            }
            r = target.point(pt, px);
            if (r != gfx_result::success) {
                return r;
            }
        }
    }
    return gfx_result::success;
}
static inline uint32_t pixel_byte_mul32(uint32_t x, uint32_t a) {
    uint32_t t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;
    x = ((x >> 8) & 0xff00ff) * a;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}

namespace helpers {

// one empty tag type per fast path, plus a generic fallback tag
struct aa_row_565 {};
struct aa_row_rgb24 {};
struct aa_row_rgba32 {};
struct aa_row_indexed {};
struct aa_row_generic {};

// default now categorizes: indexed pixels -> indexed tag, everything else -> generic
template <typename PixelType,
          bool Indexed = PixelType::template has_channel_names<channel_name::index>::value>
struct aa_row_tag { typedef aa_row_generic type; };

template <typename PixelType>
struct aa_row_tag<PixelType, true> { typedef aa_row_indexed type; };

// fast-path overrides — all non-indexed, so the bool defaults to false
template <> struct aa_row_tag<rgb_pixel<16>,  false> { typedef aa_row_565    type; };
template <> struct aa_row_tag<rgb_pixel<24>,  false> { typedef aa_row_rgb24  type; };
template <> struct aa_row_tag<rgba_pixel<32>, false> { typedef aa_row_rgba32 type; };

// --- one overload per tag; each sees only its own code ---

template <typename Destination>
gfx_result aa_row_impl(aa_row_565, Destination& destination, spoint16 location,
                       const uint8_t* cov, size_t width,
                       typename Destination::pixel_type color) {
    const int16_t minx = location.x, py = location.y;
    int16_t row_w = (int16_t)width;
    rgb_pixel<16> rgb = color;
    const uint16_t fg = rgb.native_value;
    const uint32_t fg_s = (uint32_t)(fg & 0xF81F) | ((uint32_t)(fg & 0x07E0) << 16);
    gfx_span span = destination.span(point16(minx, py));
    if(span.length==0) return gfx_result::success;
    int16_t max_width = span.length>>1;
    if(row_w>max_width) {
        row_w = max_width;
    }
    uint8_t* d = span.data;
    for (int i = 0; i < row_w; ++i) {
        const uint8_t a = cov[i];
        if (0 == a) continue;
        const int j = i << 1;
        if (a >= 255) {
#ifndef HTCW_GFX_NO_SWAP
            d[j] = (uint8_t)(fg >> 8); d[j+1] = (uint8_t)fg;
#else
            d[j] = (uint8_t)fg;        d[j+1] = (uint8_t)(fg >> 8);
#endif
            continue;
        }
#ifndef HTCW_GFX_NO_SWAP
        const uint16_t bg = ((uint16_t)d[j] << 8) | (uint16_t)d[j+1];
#else
        const uint16_t bg = (uint16_t)d[j] | ((uint16_t)d[j+1] << 8);
#endif
        const uint32_t bg_s = (uint32_t)(bg & 0xF81F) | ((uint32_t)(bg & 0x07E0) << 16);
        uint32_t a5 = (uint32_t)(a + 4) >> 3; if (a5 > 31) a5 = 31;
        const uint32_t bl = (bg_s + (((fg_s - bg_s) * a5) >> 5)) & 0x07E0F81F;
        const uint16_t out = (uint16_t)((bl & 0xFFFF) | (bl >> 16));
#ifndef HTCW_GFX_NO_SWAP
        d[j] = (uint8_t)(out >> 8); d[j+1] = (uint8_t)out;
#else
        d[j] = (uint8_t)out;        d[j+1] = (uint8_t)(out >> 8);
#endif
    }
    return gfx_result::success;
}

template <typename Destination>
gfx_result aa_row_impl(aa_row_rgb24, Destination& destination, spoint16 location,
                       const uint8_t* cov, size_t width,
                       typename Destination::pixel_type color) {
    const int16_t minx = location.x, py = location.y;
    int16_t row_w = (int16_t)width;
    rgb_pixel<24> rgb = color;
    const uint32_t fg = rgb.native_value;
    gfx_span span = destination.span(point16(minx, py));
    if(span.length==0) return gfx_result::success;
    int16_t max_width = span.length/3;
    if(row_w>max_width) {
        row_w = max_width;
    }
    uint8_t* d = span.data;
    for (int i = 0; i < row_w; ++i) {
        const uint8_t a = cov[i];
        if (0 == a) continue;
        const int j = i * 3;
        uint32_t out;
        if (a >= 255) out = fg;
        else {
#ifndef HTCW_GFX_NO_SWAP
            const uint32_t bg = ((uint32_t)d[j] << 24) | ((uint32_t)d[j+1] << 16) | ((uint32_t)d[j+2] << 8);
#else
            const uint32_t bg = ((uint32_t)d[j] << 8) | ((uint32_t)d[j+1] << 16) | ((uint32_t)d[j+2] << 24);
#endif
            out = pixel_byte_mul32(bg, 255 - a) + pixel_byte_mul32(fg, a);
        }
#ifndef HTCW_GFX_NO_SWAP
        d[j] = (uint8_t)(out >> 24); d[j+1] = (uint8_t)(out >> 16); d[j+2] = (uint8_t)(out >> 8);
#else
        d[j] = (uint8_t)(out >> 8);  d[j+1] = (uint8_t)(out >> 16); d[j+2] = (uint8_t)(out >> 24);
#endif
    }
    return gfx_result::success;
}

template <typename Destination>
gfx_result aa_row_impl(aa_row_rgba32, Destination& destination, spoint16 location,
                       const uint8_t* cov, size_t width,
                       typename Destination::pixel_type color) {
    const int16_t minx = location.x, py = location.y;
    int16_t row_w = (int16_t)width;
    rgba_pixel<32> rgba = color;
    rgba.template channel<channel_name::A>(255);
    const uint32_t fg = rgba.native_value;
    gfx_span span = destination.span(point16(minx, py));
    if(span.length==0) return gfx_result::success;
    int16_t max_width = span.length>>2;
    if(row_w>max_width) {
        row_w = max_width;
    }
    uint8_t* d = span.data;
    for (int i = 0; i < row_w; ++i) {
        const uint8_t a = cov[i];
        if (0 == a) continue;
        const int j = i << 2;
        uint32_t out;
        if (a >= 255) out = fg;
        else {
#ifndef HTCW_GFX_NO_SWAP
            const uint32_t bg = ((uint32_t)d[j] << 24) | ((uint32_t)d[j+1] << 16) | ((uint32_t)d[j+2] << 8) | d[j+3];
#else
            const uint32_t bg = (uint32_t)d[j] | ((uint32_t)d[j+1] << 8) | ((uint32_t)d[j+2] << 16) | ((uint32_t)d[j+3] << 24);
#endif
            out = pixel_byte_mul32(bg, 255 - a) + pixel_byte_mul32(fg, a);
        }
#ifndef HTCW_GFX_NO_SWAP
        d[j] = (uint8_t)(out >> 24); d[j+1] = (uint8_t)(out >> 16); d[j+2] = (uint8_t)(out >> 8); d[j+3] = (uint8_t)out;
#else
        d[j] = (uint8_t)out; d[j+1] = (uint8_t)(out >> 8); d[j+2] = (uint8_t)(out >> 16); d[j+3] = (uint8_t)(out >> 24);
#endif
    }
    return gfx_result::success;
}
template <typename Destination>
gfx_result aa_row_impl(aa_row_generic, Destination& destination, spoint16 location,
                       const uint8_t* cov, size_t width,
                       typename Destination::pixel_type color) {
    const int16_t minx = location.x, py = location.y;
    const int16_t row_w = (int16_t)width;
    typename Destination::pixel_type bgpx, dpx;
    gfx_result r;
    for (int i = 0; i < row_w; ++i) {
        const uint8_t c8 = cov[i];
        if (0 == c8) continue;
        const point16 p((uint16_t)(minx + i), (uint16_t)py);
        if (c8 < 255) {
            r = destination.point(p, &bgpx);
            if (gfx_result::success != r) return r;
            dpx = color.blend8(bgpx, c8);
        } else {
            dpx = color;
        }
        r = destination.point(p, dpx);
        if (gfx_result::success != r) return r;
    }
    return gfx_result::success;
}

template <typename Destination>
gfx_result aa_row_impl(aa_row_indexed, Destination& destination, spoint16 location,
                       const uint8_t* cov, size_t width,
                       typename Destination::pixel_type color) {
    const int16_t minx = location.x, py = location.y;
    const int16_t row_w = (int16_t)width;
    rgba_pixel<32> fgcol, bgcol;
    convert_palette_to(destination, color, &fgcol);   // resolve fg once
    typename Destination::pixel_type bgpx, dpx;
    gfx_result r;
    for (int i = 0; i < row_w; ++i) {
        const uint8_t c8 = cov[i];
        if (0 == c8) continue;
        const point16 p((uint16_t)(minx + i), (uint16_t)py);
        if (c8 < 255) {
            r = destination.point(p, &bgpx);
            if (gfx_result::success != r) return r;
            convert_palette_to(destination, bgpx, &bgcol);   // palette -> rgba
            bgcol = fgcol.blend8(bgcol, c8);                 // blend in rgba space
            convert_palette_from(destination, bgcol, &dpx);  // rgba -> nearest index
        } else {
            dpx = color;
        }
        r = destination.point(p, dpx);
        if (gfx_result::success != r) return r;
    }
    return gfx_result::success;
}

} // namespace helpers

// public entry: picks the tag at compile time, dispatches to the one live overload
template <typename Destination>
gfx_result aa_rasterize_row(Destination& destination, spoint16 location,
                            const uint8_t* cov, size_t width,
                            typename Destination::pixel_type color) {
    typename helpers::aa_row_tag<typename Destination::pixel_type>::type tag;
    return helpers::aa_row_impl(tag, destination, location, cov, width, color);
}

}  // namespace gfx
#endif