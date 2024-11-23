#ifndef HTCW_GFX_xdraw_ARC
#define HTCW_GFX_xdraw_ARC
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_draw_line.hpp"
namespace gfx {
namespace helpers {
class xdraw_arc {
    template <typename Destination, typename PixelType>
    static gfx_result arc_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip, bool filled) {
        using int_type = typename srect16::value_type;
        gfx_result r;
        int_type x_adj = (1 - (rect.width() & 1));
        int_type y_adj = (1 - (rect.height() & 1));
        int orient;
        if (rect.y1 <= rect.y2) {
            if (rect.x1 <= rect.x2) {
                orient = 0;
            } else {
                orient = 2;
            }
        } else {
            if (rect.x1 <= rect.x2) {
                orient = 1;
            } else {
                orient = 3;
            }
        }
        int_type rx = rect.width() - x_adj - 1;
        int_type ry = rect.height() - 1;
        int_type xc = rect.width() + rect.left() - x_adj - 1;
        int_type yc = rect.height() + rect.top() - 1;
        if (0 == rx)
            rx = 1;
        if (0 == ry)
            ry = 1;
        float dx, dy, d1, d2, x, y;
        x = 0;
        y = ry;
        int_type oy = -1, ox = -1;
        // Initial decision parameter of region 1
        d1 = (ry * ry) - (rx * rx * ry) + (0.25 * rx * rx);
        dx = 2 * ry * ry * x;
        dy = 2 * rx * rx * y;
        // For region 1
        while (dx < dy + y_adj) {
            if (filled) {
                if (oy != y) {
                    switch (orient) {
                        case 0:  // x1<=x2,y1<=y2
                            r = xdraw_line::line(destination, srect16(-x + xc, -y + yc, xc + x_adj, -y + yc), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            break;
                        case 1:  // x1<=x2,y1>y2
                            r = xdraw_line::line(destination, srect16(-x + xc, y + yc - ry, xc + x_adj, y + yc - ry), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            break;
                        case 2:  // x1>x2,y1<=y2
                            r = xdraw_line::line(destination, srect16(xc - rx, -y + yc, x + xc + x_adj - rx, -y + yc), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            break;
                        case 3:  // x1>x2,y1>y2
                            r = xdraw_line::line(destination, srect16(xc - rx, y + yc - ry, x + xc - rx, y + yc - ry), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            break;
                    }
                }
            } else {
                if (oy != y || ox != x) {
                    switch (orient) {
                        case 0:  // x1<=x2,y1<=y2
                            r = xdraw_point::point(destination, spoint16(-x + xc, -y + yc), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            if (x_adj && -y + yc == rect.y1) {
                                r = xdraw_point::point(destination, rect.top_right(), color, clip);
                                if (r != gfx_result::success)
                                    return r;
                            }

                            break;
                        case 1:  // x1<=x2,y1>y2
                            r = xdraw_point::point(destination, spoint16(-x + xc, y + yc - ry), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            if (x_adj && y + yc - ry == rect.y1) {
                                r = xdraw_point::point(destination, rect.bottom_right(), color, clip);
                                if (r != gfx_result::success)
                                    return r;
                            }
                            break;
                        case 2:  // x1>x2,y1<=y2
                            r = xdraw_point::point(destination, spoint16(x + xc + x_adj - rx, -y + yc), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            if (x_adj && -y + yc == rect.y1) {
                                r = xdraw_point::point(destination, rect.top_left(), color, clip);
                                if (r != gfx_result::success)
                                    return r;
                            }
                            break;
                        case 3:  // x1>x2,y1>y2
                            r = xdraw_point::point(destination, spoint16(x + xc + x_adj - rx, y + yc - ry), color, clip);
                            if (r != gfx_result::success)
                                return r;
                            break;
                    }
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
        while (y >= 0) {
            // printing points based on 4-way symmetry
            if (filled) {
                switch (orient) {
                    case 0:  // x1<=x2,y1<=y2
                        r = xdraw_line::line(destination, srect16(-x + xc, -y + yc, xc + x_adj, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 1:  // x1<=x2,y1>y2
                        r = xdraw_line::line(destination, srect16(-x + xc, y + yc - ry, xc + x_adj, y + yc - ry), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 2:  // x1>x2,y1<=y2
                        r = xdraw_line::line(destination, srect16(xc - rx, -y + yc, x + xc + x_adj - rx, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 3:  // x1>x2,y1>y2
                        r = xdraw_line::line(destination, srect16(xc - rx, y + yc - ry, x + xc + x_adj - rx, y + yc - ry), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                }

            } else {
                switch (orient) {
                    case 0:  // x1<=x2,y1<=y2
                        r = xdraw_point::point(destination, spoint16(-x + xc, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 1:  // x1<=x2,y1>y2
                        r = xdraw_point::point(destination, spoint16(-x + xc, y + yc - ry), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 2:  // x1>x2,y1<=y2
                        r = xdraw_point::point(destination, spoint16(x + xc + x_adj - rx, -y + yc), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                    case 3:  // x1>x2,y1>y2
                        r = xdraw_point::point(destination, spoint16(x + xc + x_adj - rx, y + yc - ry), color, clip);
                        if (r != gfx_result::success)
                            return r;
                        break;
                }
            }

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
    // draws an arc with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result arc(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return arc_impl(destination, rect, color, clip, false);
    }
    // draws an arc with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result arc(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return arc(destination, (srect16)rect, color, clip);
    }
    // draws a arc with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_arc(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return arc_impl(destination, rect, color, clip, true);
    }
    // draws a arc with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_arc(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return filled_arc(destination, rect, color, clip);
    }

};
}
}
#endif