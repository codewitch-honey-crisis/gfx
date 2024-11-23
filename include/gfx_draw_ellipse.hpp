#ifndef HTCW_GFX_DRAW_ELLIPSE_HPP
#define HTCW_GFX_DRAW_ELLIPSE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_draw_line.hpp"
namespace gfx {
namespace helpers {
class xdraw_ellipse {
    template <typename Destination, typename PixelType>
    static gfx_result ellipse_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip, bool filled) {
        gfx_result r;
        using int_type = typename srect16::value_type;
        int_type x_adj = (1 - (rect.width() & 1));
        int_type y_adj = (1 - (rect.height() & 1));
        int_type rx = rect.width() / 2 - x_adj;
        int_type ry = rect.height() / 2 - y_adj;
        if (0 == rx)
            rx = 1;
        if (0 == ry)
            ry = 1;
        int_type xc = rect.width() / 2 + rect.left() - x_adj;
        int_type yc = rect.height() / 2 + rect.top() - y_adj;
        float dx, dy, d1, d2, x, y;
        x = 0;
        y = ry;
        // Initial decision parameter of region 1
        d1 = (ry * ry) - (rx * rx * ry) + (0.25 * rx * rx);
        dx = 2 * ry * ry * x;
        dy = 2 * rx * rx * y;
        int_type oy = -1, ox = -1;
        // For region 1
        while (dx < dy + y_adj) {
            if (filled) {
                if (oy != y) {
                    r = xdraw_line::line(destination, srect16(-x + xc, y + yc + y_adj, x + xc + x_adj, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    r = xdraw_line::line(destination, srect16(-x + xc, -y + yc, x + xc + x_adj, -y + yc), color, clip);
                    if (r != gfx_result::success)
                        return r;
                }
            } else {
                if (oy != y || ox != x) {
                    // Print points based on 4-way symmetry
                    r = xdraw_point::point(destination, spoint16(x + xc + x_adj, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    r = xdraw_point::point(destination, spoint16(-x + xc, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    r = xdraw_point::point(destination, spoint16(x + xc + x_adj, -y + yc), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    r = xdraw_point::point(destination, spoint16(-x + xc, -y + yc), color, clip);
                    if (r != gfx_result::success)
                        return r;
                }
            }
            ox = x;
            oy = y;
            // Checking and updating value of
            // decision parameter based on algorithm
            if (d1 < 0) {
                ++x;
                dx = dx + (2 * ry * ry);
                d1 = d1 + dx + (ry * ry);
            } else {
                ++x;
                --y;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d1 = d1 + dx - dy + (ry * ry);
            }
        }

        // Decision parameter of region 2
        d2 = ((ry * ry) * ((x + 0.5) * (x + 0.5))) + ((rx * rx) * ((y - 1) * (y - 1))) - (rx * rx * ry * ry);

        // Plotting points of region 2
        while (y >= 0.0f) {
            // printing points based on 4-way symmetry
            if (filled) {
                if (oy != y) {
                    r = xdraw_line::line(destination, srect16(-x + xc, y + yc + y_adj, x + xc + x_adj, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    if (y != 0 || 1 == y_adj) {
                        r = xdraw_line::line(destination, srect16(-x + xc, -y + yc, x + xc + x_adj, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                    }
                }
            } else {
                if (oy != y || ox != x) {
                    r = xdraw_point::point(destination, spoint16(x + xc + x_adj, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    r = xdraw_point::point(destination, spoint16(-x + xc, y + yc + y_adj), color, clip);
                    if (r != gfx_result::success)
                        return r;
                    if (y != 0 || 1 == y_adj) {
                        r = xdraw_point::point(destination, spoint16(x + xc + x_adj, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        r = xdraw_point::point(destination, spoint16(-x + xc, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                    }
                }
            }
            ox = x;
            oy = y;
            // Checking and updating parameter
            // value based on algorithm
            if (d2 > 0) {
                --y;
                dy = dy - (2 * rx * rx);
                d2 = d2 + (rx * rx) - dy;
            } else {
                --y;
                ++x;
                dx = dx + (2 * ry * ry);
                dy = dy - (2 * rx * rx);
                d2 = d2 + dx - dy + (rx * rx);
            }
        }
        return gfx_result::success;
    }
public:
    // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result ellipse(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return ellipse_impl(destination, rect, color, clip, false);
    }
    // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result ellipse(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return ellipse(destination, (srect16)rect, color, clip);
    }
    // draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_ellipse(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return ellipse_impl(destination, rect, color, clip, true);
    }
    // draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_ellipse(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return filled_ellipse(destination, (srect16)rect, color, clip);
    }
};
}
}
#endif