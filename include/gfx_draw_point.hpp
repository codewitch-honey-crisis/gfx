#ifndef HTCW_GFX_DRAW_POINT_HPP
#define HTCW_GFX_DRAW_POINT_HPP
#include "gfx_draw_common.hpp"
namespace gfx {
namespace helpers {
class xdraw_point {
    template <typename Destination, typename PixelType>
    static gfx_result draw_complex(Destination& destination, point16 location, PixelType color) {
        gfx_result r = gfx_result::success;
        typename Destination::pixel_type dpx;
        using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
        const int chiA = tindexA::value;
        using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
        auto alp = color.template channel_unchecked<chiA>();
        if (alp != tchA::max) {
            // we have to mix at this level because the destination doesn't support alpha
            if (alp == tchA::min) {
                return gfx_result::success;
            }
            typename Destination::pixel_type bgpx;
            r = destination.point(location, &bgpx);
            if (gfx_result::success != r) {
                return r;
            }
            r = convert_palette_from(destination, color, &dpx, &bgpx);
            if (gfx_result::success != r) {
                return r;
            }
            return destination.point(location, dpx);
        }
        r = convert_palette_from(destination, color, &dpx);
        if (gfx_result::success != r) {
            return r;
        }
        return destination.point(location, dpx);
    }
    template <typename Destination, typename PixelType>
    struct draw_point_helper {
        static gfx_result do_draw(Destination& destination, point16 location, PixelType color) {
            return draw_complex(destination,location,color);
        }
    };
    /*
    template <typename Destination>
    struct draw_point_helper<Destination, typename Destination::pixel_type> {
        static gfx_result do_draw(Destination& destination, point16 location, typename Destination::pixel_type color) {
             using p_t = typename Destination::pixel_type;
            static constexpr const size_t ai = p_t::template channel_index_by_name<channel_name::A>::value;
            if(ai>-1) {
                auto av = color.template channel_unchecked<ai>();
                if(av==0) {
                    return gfx_result::success;
                }
                if(av<p_t::template channel_by_index_unchecked<ai>::max) {
                    return draw_complex(destination,location,color);
                }
            }
            return destination.point(location, color);
        }
    };
    */
    template <typename Destination, typename PixelType>
    static gfx_result point_impl(Destination& destination, spoint16 location, PixelType color, const srect16* clip) {
        if (clip != nullptr && !clip->intersects(location))
            return gfx_result::success;
        if (!((srect16)destination.bounds()).intersects(location))
            return gfx_result::success;
        point16 p;
        if (draw_translate(location, &p)) {
            return draw_point_helper<Destination, PixelType>::do_draw(destination, p, color);
        }
        return gfx_result::success;
    }

public:
    template<typename Destination,typename PixelType>
    static gfx_result point(Destination& destination, point16 location, PixelType color, const srect16* clip = nullptr) {
        return point_impl(destination,(point16)location,color,clip);
    }
    template<typename Destination,typename PixelType>
    static gfx_result point(Destination& destination, spoint16 location, PixelType color, const srect16* clip = nullptr) {
        return point_impl(destination,location,color,clip);
    }
};
}
}
#endif