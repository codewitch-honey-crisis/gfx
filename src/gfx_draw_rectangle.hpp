#ifndef HTCW_GFX_DRAW_RECTANGLE_HPP
#define HTCW_GFX_DRAW_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
#include "gfx_draw_point.hpp"
namespace gfx {
namespace helpers {
class xdraw_rectangle {
    template <typename Destination, typename PixelType>
    static gfx_result rectangle_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip) {
        if (rect.x1 == rect.x2) {
            if (rect.y1 == rect.y2) {
                return xdraw_point::point(destination, rect.top_left(), color, clip);
            }
            return xdraw_line::line(destination, rect, color, clip);
        } else if (rect.y1 == rect.y2) {
            return xdraw_line::line(destination, rect, color, clip);
        }
        gfx_result r;
        // top or bottom
        r = xdraw_line::line(destination, srect16(rect.x1 + 1, rect.y1, rect.x2, rect.y1), color, clip);
        if (r != gfx_result::success)
            return r;
        // left or right
        r = xdraw_line::line(destination, srect16(rect.x1, rect.y1, rect.x1, rect.y2 - 1), color, clip);
        if (r != gfx_result::success)
            return r;
        // right or left
        r = xdraw_line::line(destination, srect16(rect.x2, rect.y1 + 1, rect.x2, rect.y2), color, clip);
        if (r != gfx_result::success)
            return r;
        // bottom or top
        return xdraw_line::line(destination, srect16(rect.x1, rect.y2, rect.x2 - 1, rect.y2), color, clip);
    }
    public:
    // draws a filled rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result rectangle(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return rectangle(destination, (srect16)rect, color, clip);
    }
    template <typename Destination, typename PixelType>
    inline static gfx_result rectangle(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return rectangle_impl(destination, rect, color, clip);
    }
};
}
}
#endif