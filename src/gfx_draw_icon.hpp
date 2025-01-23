#ifndef HTCW_GFX_DRAW_ICON_HPP
#define HTCW_GFX_DRAW_ICON_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_filled_rectangle.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_draw_bitmap.hpp"
namespace gfx {
namespace helpers {
class xdraw_icon {
    template <typename Destination, typename Source, typename PixelType>
    struct draw_icon_helper final {
        static gfx_result do_draw(Destination& destination, spoint16 location, const Source& source, PixelType forecolor, PixelType backcolor, bool transparent_background, bool invert, const srect16* clip) {
            bool opaque = false;
            if (transparent_background == false) {
                opaque=true;
                if (PixelType::template has_channel_names<channel_name::A>::value) {
                    constexpr static const auto i = PixelType::template channel_index_by_name<channel_name::A>::value;
                    if (forecolor.template channelr_unchecked<i>() != 1.0 ||
                        backcolor.template channelr_unchecked<i>() != 1.0) {
                        opaque = false;
                    }
                }
            }
            srect16 bounds(location, (ssize16)source.dimensions());
            if (clip != nullptr) {
                bounds = bounds.crop(*clip);
            }
            if (!bounds.intersects((srect16)destination.bounds())) {
                return gfx_result::success;
            }

            rect16 srcr = source.bounds();
            if (location.x != bounds.x1) {
                srcr.x1 += (bounds.x1 - location.x);
            }
            if (location.y != bounds.y1) {
                srcr.y1 += (bounds.y1 - location.y);
            }
            if (bounds.x1 < 0) {
                srcr.x1 += -bounds.x1;
            }
            if (bounds.y1 < 0) {
                srcr.y1 += -bounds.y1;
            }
            if (srcr.x1 > srcr.x2 || srcr.y1 > srcr.y2) {
                return gfx_result::success;
            }
            bounds = bounds.crop((srect16)destination.bounds());
            rect16 dstr = (rect16)bounds;
            gfx_result r;
            if(opaque) {
                gfx_result r;

                typename Destination::pixel_type dpx, fgpx, bgpx;
                r = convert_palette_from(destination, forecolor, &fgpx);
                if (r != gfx_result::success) {
                    return r;
                }
                r = convert_palette_from(destination, backcolor, &bgpx);
                if (r != gfx_result::success) {
                    return r;
                }
                double a = NAN, oa = NAN;
                int w = srcr.x2 - srcr.x1 + 1, h = srcr.y2 - srcr.y1 + 1;
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        point16 pt(x + srcr.x1, y + srcr.y1);
                        typename Source::pixel_type rpx;
                        r = source.point(pt, &rpx);
                        if (r != gfx_result::success) {
                            return r;
                        }
                        a = rpx.template channelr<channel_name::A>();
                        if (invert) {
                            a = 1.0 - a;
                        }

                        if (a != oa) {
                            dpx = fgpx.blend(bgpx, a);
                        }

                        r = destination.point(point16(dstr.x1 + x, dstr.y1 + y), dpx);
                        if (r != gfx_result::success) {
                            return r;
                        }
                        oa = a;
                    }
                }
                return r;
            }
            if (transparent_background == false) {
                r = xdraw_filled_rectangle::filled_rectangle(destination, dstr, backcolor);
                if (r != gfx_result::success) {
                    return r;
                }
            }
            constexpr static const auto a_idx = PixelType::template channel_index_by_name<channel_name::A>::value;
            auto alpha_factor = 1.0;
            if (a_idx != -1) {
                alpha_factor = forecolor.template channelr_unchecked<a_idx>();
            }
            typename Destination::pixel_type dpx, fgpx, bgpx, obgpx;
            r = convert_palette_from(destination, forecolor, &fgpx);
            if (r != gfx_result::success) {
                return r;
            }
            double a = NAN, oa = NAN;

            if (r != gfx_result::success) {
                return r;
            }
            int w = srcr.x2 - srcr.x1 + 1, h = srcr.y2 - srcr.y1 + 1;
            if (!Destination::caps::blt && !Destination::caps::blt_spans) {
                auto full_bmp = create_bitmap_from(destination, dstr.dimensions());
                if (full_bmp.begin() != nullptr) {
                    r = copy_to_fast<decltype(full_bmp), Destination, Destination::caps::copy_to>::do_copy(full_bmp, destination, dstr, {0, 0});
                    if (r != gfx_result::success) {
                        return r;
                    }
                    for (int y = 0; y < h; ++y) {
                        for (int x = 0; x < w; ++x) {
                            point16 pt(x + srcr.x1, y + srcr.y1);
                            point16 dpt(x, y);
                            typename Source::pixel_type rpx;
                            r = source.point(pt, &rpx);
                            if (r != gfx_result::success) {
                                return r;
                            }
                            a = rpx.template channelr<channel_name::A>();
                            if (invert) {
                                a = 1.0 - a;
                            }
                            float af = alpha_factor * a;
                            if(af==0.0) {
                                dpx=bgpx;
                            } else if(af<1.0f) { 
                                r = full_bmp.point(dpt, &bgpx);
                                if (r != gfx_result::success) {
                                    return r;
                                }
                                if (a != oa || obgpx.native_value != bgpx.native_value) {
                                    dpx = fgpx.blend(bgpx, af);
                                }

                                //r=xdraw_point::point(full_bmp,(spoint16)dpt,dpx);
                                r=full_bmp.point(dpt,dpx);
                                if (r != gfx_result::success) {
                                    return r;
                                }
                            } else {
                                dpx=fgpx;
                                //r=xdraw_point::point(full_bmp,(spoint16)dpt,dpx);
                                r=full_bmp.point(dpt,dpx);
                                if (r != gfx_result::success) {
                                    return r;
                                }
                            }
                            
                            oa = a;
                            obgpx = bgpx;
                        }
                    }
                    r = xdraw_bitmap::bitmap(destination, dstr, full_bmp, full_bmp.bounds());
                    free(full_bmp.begin());
                    return r;
                }
            }

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    point16 pt(x + srcr.x1, y + srcr.y1);
                    point16 dpt(dstr.x1 + x, dstr.y1 + y);
                    typename Source::pixel_type rpx;
                    r = source.point(pt, &rpx);
                    if (r != gfx_result::success) {
                        return r;
                    }
                    r = destination.point(dpt, &bgpx);
                    if (r != gfx_result::success) {
                        return r;
                    }

                    a = rpx.template channelr<channel_name::A>();
                    if (invert) {
                        a = 1.0 - a;
                    }
                    if (a != oa || obgpx.native_value != bgpx.native_value) {
                        dpx = fgpx.blend(bgpx, a * alpha_factor);
                    }
                    //r=xdraw_point::point(destination,(spoint16)dpt,dpx);
                    r=destination.point(dpt,dpx);
                    if (r != gfx_result::success) {
                        return r;
                    }
                    oa = a;
                    obgpx = bgpx;
                }
            }

            return r;
        }
    };

    template <typename Destination, typename Source, typename PixelType>
    static inline gfx_result icon_impl(Destination& destination, spoint16 location, const Source& source, PixelType forecolor, PixelType backcolor, bool transparent_background, bool invert, const srect16* clip) {
        static_assert(Source::pixel_type::template has_channel_names<channel_name::A>::value, "The source must have an alpha channel");
        return draw_icon_helper<Destination, Source, PixelType>::do_draw(destination, location, source, forecolor, backcolor, transparent_background, invert, clip);
    }
public:
    // draws an icon to the destination at the location with an optional clipping rectangle
    template <typename Destination, typename Source, typename PixelType>
    static inline gfx_result icon(Destination& destination, spoint16 location, const Source& source, PixelType forecolor, PixelType backcolor = PixelType(0, true), bool transparent_background = true, bool invert = false, const srect16* clip = nullptr) {
        return icon_impl(destination, location, source, forecolor, backcolor, transparent_background, invert, clip);
    }
    // draws an icon to the destination at the location with an optional clipping rectangle
    template <typename Destination, typename Source, typename PixelType>
    static inline gfx_result icon(Destination& destination, point16 location, const Source& source, PixelType forecolor, PixelType backcolor = PixelType(0, true), bool transparent_background = true, bool invert = false, const srect16* clip = nullptr) {
        return icon(destination,(spoint16)location,source,forecolor,backcolor,transparent_background,invert,clip);
    }
};
}
}
#endif