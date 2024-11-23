#ifndef HTCW_GFX_DRAW_ROUNDED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_ROUNDED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
#include "gfx_draw_arc.hpp"
namespace gfx {
namespace helpers {
class xdraw_rounded_rectangle {
    template <typename Destination, typename PixelType>
    static gfx_result rounded_rectangle_impl(Destination& destination, const srect16& rect, float ratio, PixelType color, const srect16* clip) {
        // TODO: This can be sped up by copying the ellipse algorithm and modifying it slightly.
        gfx_result r;
        srect16 sr = rect.normalize();
        float fx = .025 > ratio ? .025 : ratio > .5 ? .5
                                                    : ratio;
        float fy = .025 > ratio ? .025 : ratio > .5 ? .5
                                                    : ratio;
        int rw = (sr.width() * fx + .5);
        int rh = (sr.height() * fy + .5);
        // top
        r = xdraw_line::line(destination, srect16(sr.x1 + rw, sr.y1, sr.x2 - rw, sr.y1), color, clip);
        if (gfx_result::success != r)
            return r;
        // left
        r = xdraw_line::line(destination, srect16(sr.x1, sr.y1 + rh, sr.x1, sr.y2 - rh), color, clip);
        if (gfx_result::success != r)
            return r;
        // right
        r = xdraw_line::line(destination, srect16(sr.x2, sr.y1 + rh, sr.x2, sr.y2 - rh), color, clip);
        if (gfx_result::success != r)
            return r;
        // bottom
        r = xdraw_line::line(destination, srect16(sr.x1 + rw, sr.y2, sr.x2 - rw, sr.y2), color, clip);
        if (gfx_result::success != r)
            return r;

        r = xdraw_arc::arc(destination, srect16(sr.top_left(), ssize16(rw + 1, rh + 1)), color, clip, false);
        if (gfx_result::success != r)
            return r;
        r = xdraw_arc::arc(destination, srect16(spoint16(sr.x2 - rw, sr.y1), ssize16(rw + 1, rh + 1)).flip_horizontal(), color, clip, false);
        if (gfx_result::success != r)
            return r;
        r = xdraw_arc::arc(destination, srect16(spoint16(sr.x1, sr.y2 - rh), ssize16(rw, rh)).flip_vertical(), color, clip, false);
        if (gfx_result::success != r)
            return r;
        return xdraw_arc::arc(destination, srect16(spoint16(sr.x2 - rw, sr.y2 - rh), ssize16(rw + 1, rh)).flip_all(), color, clip, false);
    }

public:
    // draws a rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result rounded_rectangle(Destination& destination, const srect16& rect, float ratio, PixelType color, const srect16* clip = nullptr) {
        return rounded_rectangle_impl(destination, rect, ratio, color, clip);
    }
    // draws a rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result rounded_rectangle(Destination& destination, const rect16& rect, float ratio, PixelType color, const srect16* clip = nullptr) {
        return rounded_rectangle(destination, (srect16)rect, ratio, color, clip);
    }
};
}
}
#endif