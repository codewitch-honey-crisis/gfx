#ifndef HTCW_GFX_DRAW_LINE_HPP
#define HTCW_GFX_DRAW_LINE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_filled_rectangle.hpp"
namespace gfx {
namespace helpers {
class xdraw_line {
    // Emit one horizontal run [xa..xb] at row y, clipped to c ourselves so the
    // filled_rectangle call never has to clip (we already have the clip info).
    template <typename Destination, typename PixelType>
    static void hrun(Destination& destination, int xa, int xb, int y, PixelType color, const srect16& c) {
        if (y < c.y1 || y > c.y2) return;
        int lo = (xa < xb) ? xa : xb;
        int hi = (xa < xb) ? xb : xa;
        if (lo < c.x1) lo = c.x1;
        if (hi > c.x2) hi = c.x2;
        if (lo > hi) return;
        xdraw_filled_rectangle::filled_rectangle(
            destination, srect16((int16_t)lo, (int16_t)y, (int16_t)hi, (int16_t)y), color, nullptr);
    }
    // Emit one vertical run [ya..yb] at column x, clipped to c ourselves.
    template <typename Destination, typename PixelType>
    static void vrun(Destination& destination, int x, int ya, int yb, PixelType color, const srect16& c) {
        if (x < c.x1 || x > c.x2) return;
        int lo = (ya < yb) ? ya : yb;
        int hi = (ya < yb) ? yb : ya;
        if (lo < c.y1) lo = c.y1;
        if (hi > c.y2) hi = c.y2;
        if (lo > hi) return;
        xdraw_filled_rectangle::filled_rectangle(
            destination, srect16((int16_t)x, (int16_t)lo, (int16_t)x, (int16_t)hi), color, nullptr);
    }

    // ceil(a/b) for b>0, integer, handles negative a.
    static long long ceil_div(long long a, long long b) {
        if (a >= 0) return (a + b - 1) / b;
        return -(((-a)) / b);
    }

    template <typename Destination, typename PixelType>
    static gfx_result line_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip) {
        // Effective clip = (caller clip, or the whole target) intersected with the
        // target bounds. All clipping is done against this, by us.
        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        srect16 dr(spoint16(0, 0), ss);
        srect16 c = (nullptr != clip) ? clip->crop(dr) : dr;

        srect16 r = rect;
        if (!c.intersects(r)) {
            return gfx_result::success;
        }
        // Perfectly horizontal / vertical lines are just a rectangle.
        if (rect.x1 == rect.x2 || rect.y1 == rect.y2) {
            return xdraw_filled_rectangle::filled_rectangle(destination, rect, color, &c);
        }

        // Integer Bresenham over the TRUE endpoints. We never move the endpoints,
        // so the pixels drawn are exactly the unclipped line's pixels that fall
        // inside c -- a true clip. Consecutive pixels on the same minor-axis
        // coordinate are batched into a single filled_rectangle run.
        int x0 = rect.x1, y0 = rect.y1, x1 = rect.x2, y1 = rect.y2;
        int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
        int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        if (dx >= dy) {
            // X is the driving axis; runs are horizontal.
            // Jump straight to the portion whose x lies within the clip window,
            // computing the exact error/row at that entry (phase preserved).
            long long e0 = dx / 2;
            int ilo, ihi;
            if (sx > 0) { ilo = c.x1 - x0; ihi = c.x2 - x0; }
            else        { ilo = x0 - c.x2; ihi = x0 - c.x1; }
            if (ilo < 0) ilo = 0;
            if (ihi > dx) ihi = dx;
            if (ilo > ihi) return gfx_result::success;

            long long k = ceil_div((long long)dy * ilo - e0, dx);
            int e = (int)(e0 - (long long)dy * ilo + (long long)dx * k);
            int cur_y = y0 + sy * (int)k;
            int run_x0 = x0 + sx * ilo;

            for (int i = ilo; i <= ihi; ++i) {
                // The minor axis is monotonic, so once it leaves the clip on the
                // far side it never returns: stop.
                if ((sy > 0 && cur_y > c.y2) || (sy < 0 && cur_y < c.y1)) break;
                e -= dy;
                bool ystep = false;
                if (e < 0) { e += dx; ystep = true; }
                if (ystep && i < ihi) {
                    hrun(destination, run_x0, x0 + sx * i, cur_y, color, c);
                    cur_y += sy;
                    run_x0 = x0 + sx * (i + 1);
                } else if (i == ihi) {
                    hrun(destination, run_x0, x0 + sx * i, cur_y, color, c);
                }
            }
        } else {
            // Y is the driving axis; runs are vertical.
            long long e0 = dy / 2;
            int ilo, ihi;
            if (sy > 0) { ilo = c.y1 - y0; ihi = c.y2 - y0; }
            else        { ilo = y0 - c.y2; ihi = y0 - c.y1; }
            if (ilo < 0) ilo = 0;
            if (ihi > dy) ihi = dy;
            if (ilo > ihi) return gfx_result::success;

            long long k = ceil_div((long long)dx * ilo - e0, dy);
            int e = (int)(e0 - (long long)dx * ilo + (long long)dy * k);
            int cur_x = x0 + sx * (int)k;
            int run_y0 = y0 + sy * ilo;

            for (int i = ilo; i <= ihi; ++i) {
                if ((sx > 0 && cur_x > c.x2) || (sx < 0 && cur_x < c.x1)) break;
                e -= dx;
                bool xstep = false;
                if (e < 0) { e += dy; xstep = true; }
                if (xstep && i < ihi) {
                    vrun(destination, cur_x, run_y0, y0 + sy * i, color, c);
                    cur_x += sx;
                    run_y0 = y0 + sy * (i + 1);
                } else if (i == ihi) {
                    vrun(destination, cur_x, run_y0, y0 + sy * i, color, c);
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