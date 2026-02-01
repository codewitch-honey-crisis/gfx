#ifndef HTCW_GFX_DRAW_FILLED_ROUNDED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_FILLED_ROUNDED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_filled_rectangle.hpp"
#include "gfx_draw_arc.hpp"
namespace gfx {
namespace helpers {
class xdraw_filled_rounded_rectangle {
    template <typename Destination, typename PixelType>
    static gfx_result filled_rounded_rectangle_impl(Destination& destination, const srect16& rect, float ratio, PixelType color, const srect16* clip) {
        gfx_result r;
        srect16 sr = rect.normalize();
        float fx = .025 > ratio ? .025 : ratio > .5 ? .5
                                                    : ratio;
        float fy = .025 > ratio ? .025 : ratio > .5 ? .5
                                                    : ratio;
        int rw = (sr.width() * fx + .5);
        int rh = (sr.height() * fy + .5);
        // top
        r = xdraw_filled_rectangle::filled_rectangle(destination, srect16(sr.x1 + rw + 1, sr.y1, sr.x2 - rw - 1, sr.y1 + rh - 1), color, clip);
        if (gfx_result::success != r)
            return r;
        // middle
        r = xdraw_filled_rectangle::filled_rectangle(destination, srect16(sr.x1, sr.y1 + rh, sr.x2, sr.y2 - rh - 1), color, clip);
        if (gfx_result::success != r)
            return r;
        // bottom
        r = xdraw_filled_rectangle::filled_rectangle(destination, srect16(sr.x1 + rw, sr.y2 - rh, sr.x2 - rw - 1, sr.y2), color, clip);
        if (gfx_result::success != r)
            return r;

        r = xdraw_arc::filled_arc(destination, srect16(sr.point1(), ssize16(rw + 1, rh)), color, clip);
        if (gfx_result::success != r)
            return r;
        r = xdraw_arc::filled_arc(destination, srect16(spoint16(sr.x2 - rw, sr.y1), ssize16(rw + 1, rh)).flip_horizontal(), color, clip);
        if (gfx_result::success != r)
            return r;
        r = xdraw_arc::filled_arc(destination, srect16(spoint16(sr.x1, sr.y2 - rh), ssize16(rw, rh)).flip_vertical(), color, clip);
        if (gfx_result::success != r)
            return r;
        return xdraw_arc::filled_arc(destination, srect16(spoint16(sr.x2 - rw, sr.y2 - rh), ssize16(rw + 1, rh + 1)).flip_all(), color, clip);
    }
public:
    // draws a filled rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_rounded_rectangle(Destination& destination, const srect16& rect, float ratio, PixelType color, const srect16* clip = nullptr) {
        return filled_rounded_rectangle_impl(destination, rect, ratio, color, clip);
    }
    // draws a filled rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_rounded_rectangle(Destination& destination, const rect16& rect, float ratio, PixelType color, const srect16* clip = nullptr) {
        return filled_rounded_rectangle(destination, (srect16)rect, ratio, color, clip);
    }
};
}
}
#endif