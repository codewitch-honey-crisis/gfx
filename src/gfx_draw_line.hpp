#ifndef HTCW_GFX_DRAW_LINE_HPP
#define HTCW_GFX_DRAW_LINE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_filled_rectangle.hpp"
namespace gfx {
namespace helpers {
class xdraw_line {
    template <typename Destination, typename PixelType>
    static gfx_result line_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip) {
        srect16 c = (nullptr != clip) ? *clip : rect;
        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        c = c.crop(srect16(spoint16(0, 0), ss));
        srect16 r = rect;
        if (!c.intersects(r)) {
            return gfx_result::success;
        }
        if (rect.x1 == rect.x2 || rect.y1 == rect.y2) {
            return xdraw_filled_rectangle::filled_rectangle(destination, rect, color, clip);
        }

        float xinc, yinc, x, y, ox, oy;
        float dx, dy, e;
        dx = abs(r.x2 - r.x1);
        dy = abs(r.y2 - r.y1);
        if (r.x1 < r.x2)
            xinc = 1;
        else
            xinc = -1;
        if (r.y1 < r.y2)
            yinc = 1;
        else
            yinc = -1;
        x = ox = r.x1;
        y = oy = r.y1;
        if (dx >= dy) {
            e = (2 * dy) - dx;
            while (x != r.x2) {
                if (e < 0) {
                    e += (2 * dy);
                } else {
                    e += (2 * (dy - dx));
                    oy = y;
                    y += yinc;
                }

                x += xinc;
                if (oy != y || y == r.y1) {
                    xdraw_filled_rectangle::filled_rectangle(destination, srect16(x, oy, x, oy), color, clip);
                    ox = x;
                }
            }
        } else {
            e = (2 * dx) - dy;
            while (y != r.y2) {
                if (e < 0) {
                    e += (2 * dx);
                } else {
                    e += (2 * (dx - dy));
                    ox = x;
                    x += xinc;
                }
                y += yinc;
                if (ox != x || x == r.x1) {
                    xdraw_filled_rectangle::filled_rectangle(destination, srect16(ox, y, ox, y), color, clip);
                    oy = y;
                }
            }
        }
        return gfx_result::success;
    }
public:
    // draws a line with the specified start and end point and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result line(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return line(destination, (srect16)rect, color, clip);
    }
    // draws a line with the specified start and end point and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result line(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return line_impl(destination, rect, color, clip);
    }
};
}
}
#endif