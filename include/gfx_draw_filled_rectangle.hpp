#ifndef HTCW_GFX_DRAW_FILLED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_FILLED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
namespace gfx {
namespace helpers {
class xdraw_filled_rectangle {
        template <typename Destination, bool Indexed>
    struct rect_blend_helper {
        static inline gfx_result do_blend(const Destination& dst, typename Destination::pixel_type px, float ratio, typename Destination::pixel_type bpx, typename Destination::pixel_type* out_px) {
            return px.blend(bpx, ratio, out_px);
        }
    };
    template <typename Destination>
    struct rect_blend_helper<Destination, true> {
        static inline gfx_result do_blend(const Destination& dst, typename Destination::pixel_type px, float ratio, typename Destination::pixel_type bpx, typename Destination::pixel_type* out_px) {
            rgb_pixel<HTCW_MAX_WORD> tmp;
            gfx_result r = convert_palette_to(dst, px, &tmp);
            if (r != gfx_result::success) {
                return r;
            }
            rgb_pixel<HTCW_MAX_WORD> btmp;
            r = convert_palette_to(dst, bpx, &btmp);
            if (r != gfx_result::success) {
                return r;
            }
            r = tmp.blend(btmp, ratio, &tmp);
            if (r != gfx_result::success) {
                return r;
            }
            return convert_palette_from(dst, tmp, out_px);
        }
    };
    template<typename Destination,typename PixelType>
    static gfx_result do_draw_complex_no_copy_to(Destination& destination, const rect16& rect, PixelType color) {
        gfx_result r;
        typename Destination::pixel_type dpx;
        // TODO: recode to use an async read when finally implemented
        using thas_alpha = typename PixelType::template has_channel_names<channel_name::A>;
        using tdhas_alpha = typename Destination::pixel_type::template has_channel_names<channel_name::A>;
        const bool has_alpha = thas_alpha::value;
        const bool dhas_alpha = tdhas_alpha::value;
        if (has_alpha && !dhas_alpha) {
            using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
            const int chiA = tindexA::value;
            using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
            auto alp = color.template channel_unchecked<chiA>();
            const float ar = alp * tchA::scaler;
            typename Destination::pixel_type cpx;
            typename Destination::pixel_type bpx, obpx, bbpx;
            bool first = true;
            if (alp != tchA::max) {
                if (alp == tchA::min) {
                    return gfx_result::success;
                }

                r = convert_palette_from(destination, color, &cpx);

                rect16 rr = rect.normalize();
                for (int y = rr.y1; y <= rr.y2; ++y) {
                    for (int x = rr.x1; x <= rr.x2; ++x) {
                        r = destination.point(point16(x,y), &bpx);
                        if (gfx_result::success != r) {
                            return r;
                        }
                        if (first || bpx.native_value != obpx.native_value) {
                            first = false;
                            r = rect_blend_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::do_blend(destination, cpx, ar, bpx, &bbpx);
                            if (gfx_result::success != r) {
                                return r;
                            }
                        }
                        obpx = bpx;
                        r = destination.point({uint16_t(x), uint16_t(y)}, bbpx);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                }

                return gfx_result::success;
            }
        }
        r = convert_palette_from(destination, color, &dpx);
        if (gfx_result::success != r) {
            return r;
        }
        return destination.fill(rect, dpx);
    }
    template<typename Destination, typename PixelType>
    static gfx_result do_draw_complex_copy_to(Destination& destination, const rect16& rect, PixelType color) {
        gfx_result r;
        typename Destination::pixel_type dpx;
        using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
        const int chiA = tindexA::value;
        using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
        auto alp = color.template channel_unchecked<chiA>();
        const float ar = alp * tchA::scaler;
        bool first = true;
        if (alp != tchA::max) {
            if (alp == tchA::min) {
                return gfx_result::success;
            }
            typename Destination::pixel_type cpx;
            r = convert_palette_from(destination, color, &cpx);
            if (r != gfx_result::success) {
                return r;
            }
            rect16 rr = rect.normalize();

            uint8_t* buf = nullptr;
            size16 sz = rr.dimensions();
            using bmp_type = gfx::bitmap_type_from<Destination>;
            size_t buflen = bmp_type::sizeof_buffer(sz);
            size_t linelen = bmp_type::sizeof_buffer({sz.width, uint16_t(1)});
            size_t lines = sz.height;
            bool odd = lines & 1;
            lines += odd;
            bool entire = true;
            if (sz.width > 2 && !Destination::caps::blt &&!Destination::caps::blt_spans) {
                size_t len = buflen;
                while (lines) {
                    buf = (uint8_t*)malloc(len);
                    if (buf != nullptr) {
                        break;
                    }
                    lines >>= 1;
                    len = bmp_type::sizeof_buffer({sz.width, uint16_t(lines)});
                }
                if (len < buflen) {
                    entire = false;
                    lines = len / linelen;
                }
            }
            if (odd) {
                lines -= 1;
            }
            if (buf == nullptr) {
                typename Destination::pixel_type bpx, obpx, bbpx;
                for (int y = rr.y1; y <= rr.y2; ++y) {
                    for (int x = rr.x1; x <= rr.x2; ++x) {
                        r = destination.point(point16(x,y), &bpx);
                        if (gfx_result::success != r) {
                            return r;
                        }
                        if (first || bpx.native_value != obpx.native_value) {
                            first = false;
                            r = rect_blend_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::do_blend(destination, cpx, ar, bpx, &bbpx);
                            if (gfx_result::success != r) {
                                return r;
                            }
                        }
                        obpx = bpx;
                        r = destination.point({uint16_t(x), uint16_t(y)}, bbpx);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                }
            } else {
                bmp_type bmp = helpers::bitmap_from_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::create_from(destination, sz, buf);
                if (r != gfx_result::success) {
                    free(buf);
                    return r;
                }
                if (entire) {
                    typename Destination::pixel_type bpx, obpx, bbpx;
                    r = destination.copy_to(rr, bmp, {0, 0});
                    if (gfx_result::success != r) {
                        free(buf);
                        return r;
                    }
                    for (int y = rr.y1; y <= rr.y2; ++y) {
                        for (int x = 0; x < bmp.dimensions().width; ++x) {
                            point16 pt = {uint16_t(x), uint16_t(y - rr.y1)};
                            bmp.point(pt, &bpx);
                            if (first || bpx.native_value != obpx.native_value) {
                                first = false;
                                r = rect_blend_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::do_blend(destination, cpx, ar, bpx, &bbpx);
                                if (gfx_result::success != r) {
                                    free(buf);
                                    return r;
                                }
                            }
                            obpx = bpx;
                            r = bmp.point(pt, bbpx);
                            if (gfx_result::success != r) {
                                free(buf);
                                return r;
                            }
                        }
                    }
                    r = copy_to_fast<Destination, bmp_type, true>::do_copy(destination, bmp, bmp.bounds(), rr.top_left());
                    if (gfx_result::success != r) {
                        free(buf);
                        return r;
                    }
                } else {
                    typename bmp_type::pixel_type bpx, obpx, bbpx;
                    for (int y = rr.y1; y <= rr.y2; y += lines) {
                        first = true;
                        int l = lines - 1;
                        if (lines + y > rr.y2) {
                            l = rr.y2 - y;
                        }
                        r = destination.copy_to({rr.x1, uint16_t(y), rr.x2, uint16_t(y + l)}, bmp, {0, 0});
                        if (gfx_result::success != r) {
                            free(buf);
                            return r;
                        }
                        for (int yy = 0; yy <= l; ++yy) {
                            for (int x = 0; x < bmp.dimensions().width; ++x) {
                                bmp.point({uint16_t(x), uint16_t(yy)}, &bpx);
                                if (first || bpx.native_value != obpx.native_value) {
                                    first = false;
                                    r = rect_blend_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::do_blend(destination, cpx, ar, bpx, &bbpx);
                                    if (gfx_result::success != r) {
                                        free(buf);
                                        return r;
                                    }
                                }
                                obpx = bpx;
                                bmp.point({uint16_t(x), uint16_t(yy)}, bbpx);
                            }
                        }
                        r = copy_to_fast<Destination, bmp_type, true>::do_copy(destination, bmp, {0, 0, uint16_t(rr.x2 - rr.x1), uint16_t(l)}, {rr.x1, uint16_t(y)});
                        if (gfx_result::success != r) {
                            free(buf);
                            return r;
                        }
                    }
                }
                free(buf);
            }
            return gfx_result::success;
        }
        r = convert_palette_from(destination, color, &dpx);
        if (gfx_result::success != r) {
            return r;
        }
        return destination.fill(rect, dpx);
    }
    
    template <typename Destination, typename PixelType, bool CopyTo>
    struct draw_filled_rect_helper {
    };
    /*
    template <typename Destination>
    struct draw_filled_rect_helper<Destination,typename Destination::pixel_type,false> {
        static gfx_result do_draw(Destination& destination, const rect16& rect, typename Destination::pixel_type color) {
            using p_t = typename Destination::pixel_type;
            static constexpr const size_t ai = p_t::template channel_index_by_name<channel_name::A>::value;
            if(ai>-1) {
                auto av = color.template channel_unchecked<ai>();
                if(av==0) {
                    return gfx_result::success;
                }
                if(av<p_t::channel_by_index_unchecked<ai>::max) {
                    return do_draw_complex_no_copy_to(destination,rect,color);
                }
            }
            return destination.fill(rect,color);
        }
    };
    template <typename Destination>
    struct draw_filled_rect_helper<Destination,typename Destination::pixel_type,true> {
        static gfx_result do_draw(Destination& destination, const rect16& rect, typename Destination::pixel_type color) {
            using p_t = typename Destination::pixel_type;
            static constexpr const size_t ai = p_t::template channel_index_by_name<channel_name::A>::value;
            if(ai>-1) {
                auto av = color.template channel_unchecked<ai>();
                if(av==0) {
                    return gfx_result::success;
                }
                if(av<p_t::template channel_by_index_unchecked<ai>::max) {
                    return do_draw_complex_copy_to(destination,rect,color);
                }
            }
            return destination.fill(rect,color);
        }
    };
    */
    template <typename Destination, typename PixelType>
    struct draw_filled_rect_helper<Destination, PixelType, false> {
        static gfx_result do_draw(Destination& destination, const rect16& rect, PixelType color) {
            return do_draw_complex_no_copy_to(destination,rect,color);
        }
    };
    template <typename Destination, typename PixelType>
    struct draw_filled_rect_helper<Destination, PixelType, true> {
        static gfx_result do_draw(Destination& destination, const rect16& rect, PixelType color) {
            return do_draw_complex_copy_to(destination,rect,color);
        }
    };
    template <typename Destination, typename PixelType>
    static gfx_result filled_rectangle_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip) {
        srect16 sr = rect;
        if (nullptr != clip)
            sr = sr.crop(*clip);
        rect16 r;
        if (!draw_translate_adjust(sr, &r))
            return gfx_result::success;
        if (!r.intersects(destination.bounds())) {
            return gfx_result::success;
        }
        r = r.crop(destination.bounds());

        return draw_filled_rect_helper<Destination, PixelType, Destination::caps::copy_to>::do_draw(destination, r, color);
    }
    public:
    // draws a filled rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_rectangle(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return filled_rectangle(destination, (srect16)rect, color, clip);
    }
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_rectangle(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return filled_rectangle_impl(destination, rect, color, clip);
    }

};
}
}
#endif