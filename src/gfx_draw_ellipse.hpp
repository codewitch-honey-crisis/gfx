#ifndef HTCW_GFX_DRAW_ELLIPSE_HPP
#define HTCW_GFX_DRAW_ELLIPSE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_draw_line.hpp"
namespace gfx {
namespace helpers {
class xdraw_ellipse {
    template <typename Destination, typename PixelType>
    static gfx_result xellipse_impl(Destination& destination, const srect16& rect, PixelType color, const srect16* clip, bool filled) {
        gfx_result rr;

        // Draw a single outline pixel.
        auto PT = [&](int px, int py) -> gfx_result {
            return xdraw_point::point(destination, spoint16((int16_t)px, (int16_t)py), color, clip);
        };
        // Draw one horizontal span (a filled row) from xa..xb at row py.
        auto SPAN = [&](int xa, int xb, int py) -> gfx_result {
            return xdraw_line::line(destination, srect16((int16_t)xa, (int16_t)py, (int16_t)xb, (int16_t)py), color, clip);
        };

        // Normalized bounding box. The ellipse is fit exactly inside it: the
        // extreme pixels land on all four edges for both odd and even sizes.
        int L = rect.x1 < rect.x2 ? rect.x1 : rect.x2;
        int R = rect.x1 < rect.x2 ? rect.x2 : rect.x1;
        int T = rect.y1 < rect.y2 ? rect.y1 : rect.y2;
        int B = rect.y1 < rect.y2 ? rect.y2 : rect.y1;

        // Integer bounding-rectangle midpoint ellipse (after A. Zingl), reworked
        // so that (1) every pixel is emitted exactly once -- required for alpha
        // blending, since an overlapping pixel would be blended twice -- and
        // (2) the poles are completed explicitly instead of via the original
        // tip loop, which both double-drew and fell short on eccentric ellipses.
        int x0 = L, y0 = T, x1 = R, y1 = B;
        long long a = (x1 > x0) ? (x1 - x0) : (x0 - x1);
        long long b = (y1 > y0) ? (y1 - y0) : (y0 - y1);
        long long b1 = b & 1;
        long long dx = 4 * (1 - a) * b * b;
        long long dy = 4 * (b1 + 1) * a * a;
        long long err = dx + dy + b1 * a * a, e2;
        if (x0 > x1) { x0 = x1; x1 += (int)a; }
        if (y0 > y1) y0 = y1;
        y0 += (int)((b + 1) / 2);
        y1 = y0 - (int)b1;
        a *= 8 * a;
        b1 = 8 * b * b;

        int ly0 = T, ly1 = B, lex0 = x0, lex1 = x1;   // last drawn rows + their x-extent
        int py0 = T - 2, py1 = B + 2;                 // last row emitted per half (fill de-dup)

        do {
            if (filled) {
                // One span per row, taken at row entry (widest extent). y0 (lower
                // half) and y1 (upper half) are each monotonic, so no row repeats;
                // the y1 != y0 guard keeps the shared center row single.
                if (y0 != py0) { rr = SPAN(x0, x1, y0); if (rr != gfx_result::success) return rr; py0 = y0; }
                if (y1 != py1 && y1 != y0) { rr = SPAN(x0, x1, y1); if (rr != gfx_result::success) return rr; py1 = y1; }
            } else {
                // Four-way symmetric points, de-duplicated where they coincide
                // (left==right column, or the two center rows on an even box).
                rr = PT(x1, y0); if (rr != gfx_result::success) return rr;
                if (x0 != x1) { rr = PT(x0, y0); if (rr != gfx_result::success) return rr; }
                if (y0 != y1) { rr = PT(x1, y1); if (rr != gfx_result::success) return rr; }
                if (x0 != x1 && y0 != y1) { rr = PT(x0, y1); if (rr != gfx_result::success) return rr; }
            }
            ly0 = y0; ly1 = y1; lex0 = x0; lex1 = x1;
            e2 = 2 * err;
            if (e2 <= dy) { y0++; y1--; err += dy += a; }
            if (e2 >= dx || 2 * err > dy) { x0++; x1--; err += dx += b1; }
        } while (x0 <= x1);

        // Pole cap: the main loop converges x before y on eccentric (tall)
        // ellipses, leaving the top/bottom tips undrawn. Extend the last row's
        // center column(s) straight out to the box edges. On round/wide ellipses
        // the main loop already reaches the edge, so these loops don't run.
        int cl = lex0 < lex1 ? lex0 : lex1;
        int cr = lex0 < lex1 ? lex1 : lex0;
        for (int yy = ly0 + 1; yy <= B; ++yy) {
            if (filled) { rr = SPAN(cl, cr, yy); if (rr != gfx_result::success) return rr; }
            else { rr = PT(cl, yy); if (rr != gfx_result::success) return rr;
                   if (cr != cl) { rr = PT(cr, yy); if (rr != gfx_result::success) return rr; } }
        }
        for (int yy = ly1 - 1; yy >= T; --yy) {
            if (filled) { rr = SPAN(cl, cr, yy); if (rr != gfx_result::success) return rr; }
            else { rr = PT(cl, yy); if (rr != gfx_result::success) return rr;
                   if (cr != cl) { rr = PT(cr, yy); if (rr != gfx_result::success) return rr; } }
        }
        return gfx_result::success;
    }
public:
    // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result ellipse(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return xellipse_impl(destination, rect, color, clip, false);
    }
    // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result ellipse(Destination& destination, const rect16& rect, PixelType color, const srect16* clip = nullptr) {
        return ellipse(destination, (srect16)rect, color, clip);
    }
    // draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_ellipse(Destination& destination, const srect16& rect, PixelType color, const srect16* clip = nullptr) {
        return xellipse_impl(destination, rect, color, clip, true);
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