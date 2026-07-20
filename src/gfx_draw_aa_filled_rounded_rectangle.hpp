#ifndef HTCW_GFX_DRAW_AA_FILLED_ROUNDED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_AA_FILLED_ROUNDED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"
namespace gfx {
namespace helpers {
class xdraw_aa_filled_rounded_rectangle {
    // Anti-aliased filled rounded rectangle. Same coverage model as xdraw_aa_line:
    //   coverage = clamp(0.5 - SDF), a +/-0.5 px ramp across the boundary.
    //
    // The shape is an axis-aligned box with UNIFORM CIRCULAR corner radius r. Its
    // signed distance is the classic "box shrunk by r, then inflated by a disk of
    // radius r":
    //   let a = |p - center|                    (per axis, absolute offset)
    //   let d = a - (halfsize - r)              (offset past the *inner* box)
    //   SDF  = length(max(d, 0)) + min(max(d.x, d.y), 0) - r
    // Inside the corner quadrant (d.x>0 && d.y>0) this reduces to
    //   SDF = distance_to_corner_center - r     (a circular arc; the only sqrt)
    // and everywhere else to
    //   SDF = max(d.x, d.y) - r                 (the straight edges / interior).
    //
    // Geometry / AA convention: the continuous box spans [x1-0.5 .. x2+0.5] x
    // [y1-0.5 .. y2+0.5], so pixel centers x1..x2 / y1..y2 are fully covered, the
    // four STRAIGHT edges land on a pixel boundary and stay crisp, and only the
    // corner arcs are anti-aliased. Corner centers are at (x1-0.5+r, y1-0.5+r) etc.
    //
    // COLOR CONSISTENCY (why the foreground is round-tripped once, up front):
    // the solid interior is painted with a fast rectangle blit while the corner
    // arcs are composited per-pixel. If those two paths start from different
    // foregrounds they disagree -- the per-pixel path quantizes the color through
    // the destination pixel type, so an unquantized fast fill comes out a shade
    // off at the corners. To avoid that, the color is round-tripped through the
    // destination format ONCE (color -> fgpx -> cfgpx) and every path draws from
    // that single foreground:
    //   alpha == 0   : nothing is drawn.
    //   alpha == 255 : interior via destination.fill(fgpx); corners via cfgpx at
    //                  full coverage. A raw fill and a full-coverage blend of the
    //                  same pixel are byte-identical.
    //   0 < a < 255  : interior via draw::filled_rectangle(fill_fg), where fill_fg
    //                  is cfgpx (the round-tripped RGB) carrying the source alpha,
    //                  so filled_rectangle's foreground matches the corner blend's.
    //
    // Precondition: dx*dx + dy*dy in the corner must fit in 32 bits, i.e. radius up
    // to ~180 px. Any real corner radius satisfies this with a wide margin.
    //
    // NOTE (non-circular corners): only the corner branch assumes a circle. Swapping
    // in an elliptical SDF there would make it approximate (no exact closed form).

    // Paint a solid rectangle the fastest way for the given alpha, from the shared
    // round-tripped foreground. Clamped to the clip region; no-op if empty.
    template <typename Destination>
    static gfx_result fill_solid(Destination& destination, const srect16& c,
                                 int32_t ax1, int32_t ay1, int32_t ax2, int32_t ay2,
                                 uint8_t opacity,
                                 typename Destination::pixel_type fgpx,
                                 rgba_pixel<32> fill_fg) {
   
        if (ax1 < c.x1) ax1 = c.x1;
        if (ay1 < c.y1) ay1 = c.y1;
        if (ax2 > c.x2) ax2 = c.x2;
        if (ay2 > c.y2) ay2 = c.y2;
        if (ax1 > ax2 || ay1 > ay2) return gfx_result::success;
        const rect16 rr((uint16_t)ax1, (uint16_t)ay1, (uint16_t)ax2, (uint16_t)ay2);
        if (255 == opacity) {
            return destination.fill(rr, fgpx);                    // fast opaque overwrite
        }
        // translucent: same round-tripped RGB as the corners, alpha-blended in one call
        return draw::filled_rectangle(destination, rr, fill_fg);
    }

    template <typename Destination, typename PixelType>
    static gfx_result aa_filled_rounded_rectangle_impl(Destination& destination, const srect16& rect, PixelType color, int16_t radius, mask_draw_cache* cache, const srect16* clip) {
        if (radius < 0) {
            return gfx_result::invalid_argument;
        }
        const uint8_t opacity = color.opacity8();
        if (0 == opacity) {
            return gfx_result::success; // fully transparent: nothing to draw
        }

        // clip region = (caller clip, or the whole surface) cropped to the surface bounds
        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        srect16 bounds(spoint16(0, 0), ss);
        srect16 c = (nullptr != clip) ? clip->crop(bounds) : bounds;

        // normalize corners so x1<=x2, y1<=y2
        const int32_t x1 = rect.x1 < rect.x2 ? rect.x1 : rect.x2;
        const int32_t x2 = rect.x1 < rect.x2 ? rect.x2 : rect.x1;
        const int32_t y1 = rect.y1 < rect.y2 ? rect.y1 : rect.y2;
        const int32_t y2 = rect.y1 < rect.y2 ? rect.y2 : rect.y1;

        // clamp the radius to the "pill" limit: at most half of the smaller side.
        int32_t r = radius;
        const int32_t maxr_x = (x2 - x1 + 1) / 2;
        const int32_t maxr_y = (y2 - y1 + 1) / 2;
        const int32_t maxr   = maxr_x < maxr_y ? maxr_x : maxr_y;
        if (r > maxr) r = maxr;

        // --- the single, shared, round-tripped foreground ---
        // color -> fgpx (destination format) -> cfgpx (rgba32 on the dest grid).
        // Every draw path below starts from these, so the fast fill and the corner
        // blend can never disagree on the foreground color.
        typename Destination::pixel_type fgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);
        rgba_pixel<32> cfgpx;
        convert_palette_to<Destination, rgba_pixel<32>>(destination, fgpx, &cfgpx);
        cfgpx.opacity8_inplace(255);          // corner fg: opaque; opacity applied via blend factor
        rgba_pixel<32> fill_fg = cfgpx;                        // interior fill fg: same RGB...
        fill_fg.template channel<channel_name::A>(opacity);    // ...carrying the source alpha
        typename Destination::pixel_type bgpx, dpx;
        gfx_result r_res;

        // r == 0 -> a plain crisp rectangle, painted the fast way for its alpha.
        if (r == 0) {
            return fill_solid(destination, c, x1, y1, x2, y2, opacity, fgpx, fill_fg);
        }

        // rows where the corners no longer cut in -> the full width is solid.
        const int32_t my1 = y1 + r; // first middle-band row
        const int32_t my2 = y2 - r; // last  middle-band row

        // paint the solid interior up front (fast, per alpha) as three rectangles:
        // the full-width middle band + top/bottom bars. Only the four corners remain.
        if (my1 <= my2) {
            r_res = fill_solid(destination, c, x1, my1, x2, my2, opacity, fgpx, fill_fg);
            if (gfx_result::success != r_res) return r_res;
        }
        if (x1 + r <= x2 - r) {
            r_res = fill_solid(destination, c, x1 + r, y1, x2 - r, y1 + r - 1, opacity, fgpx, fill_fg);
            if (gfx_result::success != r_res) return r_res;
            r_res = fill_solid(destination, c, x1 + r, y2 - r + 1, x2 - r, y2, opacity, fgpx, fill_fg);
            if (gfx_result::success != r_res) return r_res;
        }

        // AA work area: the shape padded by 1px (only the corners have an outside
        // fringe; the straight edges are crisp), clamped to the clip region.
        int minx = x1 - 1, maxx = x2 + 1, miny = y1 - 1, maxy = y2 + 1;
        if (minx < c.x1) minx = c.x1;
        if (miny < c.y1) miny = c.y1;
        if (maxx > c.x2) maxx = c.x2;
        if (maxy > c.y2) maxy = c.y2;
        if (minx > maxx || miny > maxy) {
            return gfx_result::success; // fully clipped away
        }

        // per-scanline coverage buffer (caller cache, or a routine-local RAII one)
        mask_draw_cache local;
        mask_draw_cache* dc = (nullptr != cache) ? cache : &local;
        const int row_w = maxx - minx + 1;
        uint8_t* cov = dc->ensure((size_t)row_w);
        if (nullptr == cov) {
            return gfx_result::out_of_memory;
        }

        // fixed point 16.16 / Q8 constants
        const int32_t half16 = 1 << 15;                     // 0.5 in 16.16
        const int32_t bx_q8  = (int32_t)(x2 - x1 + 1) << 7; // half width  in Q8 px
        const int32_t by_q8  = (int32_t)(y2 - y1 + 1) << 7; // half height in Q8 px
        const int32_t r_q8   = r << 8;                      // radius in Q8 px
        const int32_t cx2    = x1 + x2;                     // 2 * center x
        const int32_t cy2    = y1 + y2;                     // 2 * center y

        for (int py = miny; py <= maxy; ++py) {
            // middle-band rows are fully painted and have crisp vertical edges
            // (nothing to anti-alias outside [x1..x2]) -> skip them entirely.
            if (py >= my1 && py <= my2) continue;

            // per-row (y) quantities for the SDF
            const int32_t uy_q8 = (2 * py - cy2) << 7;      // (py - cy) in Q8
            const int32_t ay    = uy_q8 < 0 ? -uy_q8 : uy_q8;
            const int32_t dyq   = ay - by_q8 + r_q8;        // offset past inner box, y

            // solid interior span [ix1..ix2] for this row -- already painted above.
            int32_t ix1, ix2;
            if (py >= y1 && py <= y2)   { ix1 = x1 + r; ix2 = x2 - r; } // top/bottom bar
            else                        { ix1 = 1;      ix2 = 0;      } // fringe (empty)

            // pass 1: assemble corner coverage for this scanline
            for (int px = minx; px <= maxx; ++px) {
                if (px >= ix1 && px <= ix2) {
                    cov[px - minx] = 0; // solid interior already drawn -> skip
                    continue;
                }
                const int32_t ux_q8 = (2 * px - cx2) << 7;  // (px - cx) in Q8
                const int32_t ax    = ux_q8 < 0 ? -ux_q8 : ux_q8;
                const int32_t dxq   = ax - bx_q8 + r_q8;    // offset past inner box, x

                int32_t sdf_q8;
                if (dxq > 0 && dyq > 0) {
                    // corner quadrant: circular arc, SDF = dist_to_corner_center - r
                    const uint32_t V = (uint32_t)dxq * (uint32_t)dxq +
                                       (uint32_t)dyq * (uint32_t)dyq;   // Q16
                    const int32_t outer_q8 = (int32_t)math::sqrt_ft32<8>(V) >> 8;
                    sdf_q8 = outer_q8 - r_q8;
                } else {
                    // straight edge / interior: SDF = max(dx, dy) - r
                    sdf_q8 = (dxq > dyq ? dxq : dyq) - r_q8;
                }

                const int32_t cov16 = half16 - (sdf_q8 << 8); // 0.5 - SDF, in 16.16
                uint8_t c8;
                if (cov16 <= 0) c8 = 0;
                else if (cov16 >= (1 << 16)) c8 = 255;        // solid interior
                else c8 = (uint8_t)(cov16 >> 8);              // 16.16 -> 0..255

                cov[px - minx] = c8 * opacity / 255;
            }
            // pass 2: blend the covered corner pixels, each touched exactly once
            for (int i = 0; i < row_w; ++i) {
                const uint8_t c8 = cov[i];
                if (0 == c8) continue;
                const point16 p((uint16_t)(minx + i), (uint16_t)py);
                if(c8<255) {
                    r_res = destination.point(p, &bgpx);
                    if (gfx_result::success != r_res) return r_res;
                    dpx = fgpx.blend8(bgpx, c8);;
                } else {
                    dpx = fgpx;
                }
                r_res = destination.point(p, dpx);   // write result
                if (gfx_result::success != r_res) return r_res;
            }
        
        }
        return gfx_result::success;
    }

public:
    // draws an anti-aliased filled rounded rectangle with a uniform circular corner
    // radius (>= 0). radius is clamped to at most half of the smaller side.
    // clip: optional clipping rectangle. cache: optional draw cache to reuse.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_rounded_rectangle(Destination& destination, const rect16& rect, PixelType color, int16_t radius, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_filled_rounded_rectangle_impl(destination, (srect16)rect, color, radius, cache, clip);
    }
    // draws an anti-aliased filled rounded rectangle with a uniform circular corner
    // radius (>= 0). radius is clamped to at most half of the smaller side.
    // clip: optional clipping rectangle. cache: optional draw cache to reuse.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_rounded_rectangle(Destination& destination, const srect16& rect, PixelType color, int16_t radius, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_filled_rounded_rectangle_impl(destination, rect, color, radius, cache, clip);
    }
};
}
}
#endif