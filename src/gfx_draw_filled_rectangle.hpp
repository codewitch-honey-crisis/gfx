#ifndef HTCW_GFX_DRAW_FILLED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_FILLED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
namespace gfx {
namespace helpers {
class xdraw_filled_rectangle {
        template <typename Destination, bool Indexed>
    struct rect_blend_helper {
        static inline gfx_result do_blend(const Destination& dst, typename Destination::pixel_type px, uint8_t ratio, typename Destination::pixel_type bpx, typename Destination::pixel_type* out_px) {
            return px.blend8(bpx, ratio, out_px);
        }
    };
    template <typename Destination>
    struct rect_blend_helper<Destination, true> {
        static inline gfx_result do_blend(const Destination& dst, typename Destination::pixel_type px, uint8_t ratio, typename Destination::pixel_type bpx, typename Destination::pixel_type* out_px) {
            rgb_pixel<24> tmp;
            gfx_result r = convert_palette_to(dst, px, &tmp);
            if (r != gfx_result::success) {
                return r;
            }
            rgb_pixel<24> btmp;
            r = convert_palette_to(dst, bpx, &btmp);
            if (r != gfx_result::success) {
                return r;
            }
            r = tmp.blend8(btmp, ratio, &tmp);
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
        if (PixelType::has_alpha && !Destination::pixel_type::has_alpha) {
            auto alp = color.opacity8();
            typename Destination::pixel_type cpx;
            typename Destination::pixel_type bpx, obpx, bbpx;
            bool first = true;
            if (alp != 255) {
                if (alp == 0) {
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
                            r = rect_blend_helper<Destination, Destination::pixel_type::template has_channel_names<channel_name::index>::value>::do_blend(destination, cpx, alp, bpx, &bbpx);
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
        uint8_t alp = color.opacity8();
        bool first = true;
        if (alp != 255) {
            if (alp == 0) {
                return gfx_result::success;
            }
            typename Destination::pixel_type cpx;
            color.opacity8_inplace(255);
            r = convert_palette_from(destination, color, &cpx);
            if (r != gfx_result::success) {
                return r;
            }
            rect16 rr = rect.normalize();

            size16 sz = rr.dimensions();
            
            typename Destination::pixel_type bpx, obpx, bbpx;
            for (int y = rr.y1; y <= rr.y2; ++y) {
                aa_rasterize_row(destination,spoint16(int16_t(rr.x1),int16_t(y)),nullptr,rr.x2-rr.x1+1,cpx,alp);
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