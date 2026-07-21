#ifndef HTCW_GFX_DRAW_AA_ROUNDED_RECTANGLE_HPP
#define HTCW_GFX_DRAW_AA_ROUNDED_RECTANGLE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"
namespace gfx {
namespace helpers {
class xdraw_aa_rounded_rectangle {
    // Anti-aliased HOLLOW (stroked) rounded rectangle. Same coverage model as the
    // filled version and xdraw_aa_line: coverage = clamp(0.5 - SDF), a +/-0.5 px ramp.
    //
    // The border is the region between TWO rounded rectangles, both with rounded
    // (curved) corners:
    //   outer: the bounding rect, corner radius r
    //   inner: the bounding rect inset by 'width' on every side, corner radius ri
    // Border = inside(outer) AND outside(inner), whose signed distance is
    //   SDF = max( D_outer , -D_inner )
    // where each D is the filled rounded-box distance (negative inside):
    //   let a = |p - center|
    //   let d = a - (halfsize - radius)
    //   D    = length(max(d, 0)) + min(max(d.x, d.y), 0) - radius
    // (corner quadrant d.x>0 && d.y>0 -> D = dist_to_corner_center - radius, one sqrt;
    //  elsewhere D = max(d.x, d.y) - radius.)
    //
    // Both boxes are concentric, so the straight sides have uniform thickness =
    // width, with crisp inner and outer edges; only the corner arcs (outer AND inner)
    // are anti-aliased. The inner corner keeps radius ri (= r, clamped to the inner
    // box) so the INSIDE stays curved for any width -- it never collapses to a sharp
    // corner. As a consequence the border is a little thicker across the corners than
    // along the straights (the price of a curved inner corner). If you'd rather have
    // perfectly uniform thickness and accept sharp inner corners once width >= r, set
    // ri_q8 to (r - width) in Q8 and drop the inner-radius clamp.
    // A width that spans the box leaves no hole and yields a solid rounded rect.
    //
    // Precondition: dx*dx + dy*dy in a corner must fit in 32 bits, i.e. radius up to
    // ~180 px. Any real corner radius satisfies this with a wide margin.

    // Filled rounded-box signed distance in Q8, from absolute offsets (ax,ay) to the
    // box center, half-extents (bx,by) and corner radius r -- all in Q8 pixels.
    static inline int32_t rr_sdf_q8(int32_t ax, int32_t ay, int32_t bx_q8, int32_t by_q8, int32_t r_q8) {
        const int32_t dxq = ax - bx_q8 + r_q8;
        const int32_t dyq = ay - by_q8 + r_q8;
        if (dxq > 0 && dyq > 0) {
            const uint32_t V = (uint32_t)dxq * (uint32_t)dxq + (uint32_t)dyq * (uint32_t)dyq; // Q16
            return ((int32_t)math::sqrt_ft32<8>(V) >> 8) - r_q8;
        }
        return (dxq > dyq ? dxq : dyq) - r_q8;
    }

    template <typename Destination, typename PixelType>
    static gfx_result aa_rounded_rectangle_impl(Destination& destination, const srect16& rect, PixelType color, int16_t radius, int16_t width, mask_draw_cache* cache, const srect16* clip) {
        if (radius < 0 || width < 0) {
            return gfx_result::invalid_argument;
        }
        if (width == 0) {
            return gfx_result::success; // zero-thickness border: nothing to draw
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

        const int32_t box_w = x2 - x1 + 1;
        const int32_t box_h = y2 - y1 + 1;

        // clamp outer radius to the "pill" limit, and border width to fit the box.
        int32_t r = radius;
        const int32_t maxr = (box_w < box_h ? box_w : box_h) / 2;
        if (r > maxr) r = maxr;
        int32_t w = width;
        const int32_t mindim = box_w < box_h ? box_w : box_h;
        if (w > mindim) w = mindim;

        // inner box (inset by w on all sides, concentric) and its corner radius.
        const int32_t inner_box_w = box_w - 2 * w;
        const int32_t inner_box_h = box_h - 2 * w;
        const bool has_inner = (inner_box_w > 0 && inner_box_h > 0);
        int32_t ri = r; // inner corner radius = outer radius, clamped to the inner box
        if (has_inner) {
            const int32_t inner_maxr = (inner_box_w < inner_box_h ? inner_box_w : inner_box_h) / 2;
            if (ri > inner_maxr) ri = inner_maxr;
        }

        // single round-tripped foreground (dest-grid RGB, opaque; opacity applied
        // via the per-pixel blend factor). Only one draw path here, so no seam.
        typename Destination::pixel_type fgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);
        rgba_pixel<32> cfgpx;
        convert_palette_to<Destination, rgba_pixel<32>>(destination, fgpx, &cfgpx);
        cfgpx.opacity8_inplace(255);
        typename Destination::pixel_type bgpx, dpx;
        gfx_result r_res;

        // AA work area: the shape padded by 1px (only the outer corners have an
        // outside fringe; straight edges are crisp), clamped to the clip region.
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
        const int32_t half16       = 1 << 15;                       // 0.5 in 16.16
        const int32_t bx_q8        = (int32_t)box_w << 7;           // outer half width  (Q8)
        const int32_t by_q8        = (int32_t)box_h << 7;           // outer half height (Q8)
        const int32_t r_q8         = r << 8;                        // outer radius (Q8)
        const int32_t inner_bx_q8  = (int32_t)inner_box_w << 7;     // inner half width  (Q8)
        const int32_t inner_by_q8  = (int32_t)inner_box_h << 7;     // inner half height (Q8)
        const int32_t ri_q8        = ri << 8;                       // inner radius (Q8)
        const int32_t cx2          = x1 + x2;                       // 2 * center x
        const int32_t cy2          = y1 + y2;                       // 2 * center y

        for (int py = miny; py <= maxy; ++py) {
            const int32_t uy_q8 = (2 * py - cy2) << 7;      // (py - cy) in Q8
            const int32_t ay    = uy_q8 < 0 ? -uy_q8 : uy_q8;

            // pass 1: assemble border coverage for this scanline
            for (int px = minx; px <= maxx; ++px) {
                const int32_t ux_q8 = (2 * px - cx2) << 7;  // (px - cx) in Q8
                const int32_t ax    = ux_q8 < 0 ? -ux_q8 : ux_q8;

                // border SDF = max(outer, -inner): inside the outer AND outside the inner
                const int32_t d_out = rr_sdf_q8(ax, ay, bx_q8, by_q8, r_q8);
                int32_t sdf_q8 = d_out;
                if (has_inner) {
                    const int32_t neg_in = -rr_sdf_q8(ax, ay, inner_bx_q8, inner_by_q8, ri_q8);
                    if (neg_in > sdf_q8) sdf_q8 = neg_in;
                }

                const int32_t cov16 = half16 - (sdf_q8 << 8); // 0.5 - SDF, in 16.16
                uint8_t c8;
                if (cov16 <= 0) c8 = 0;
                else if (cov16 >= (1 << 16)) c8 = 255;        // solid part of the border
                else c8 = (uint8_t)(cov16 >> 8);              // 16.16 -> 0..255

                cov[px - minx] = c8 * opacity / 255;
            }

            // pass 2: blend the covered pixels, each touched exactly once
            aa_rasterize_row(destination,{(int16_t)minx,(int16_t)py},cov,row_w,fgpx);
        }
        return gfx_result::success;
    }

public:
    // draws an anti-aliased hollow rounded rectangle with a uniform circular corner
    // radius (>= 0) and an inner border of thickness 'width' (>= 0) that fits inside
    // the bounding rect. Both outer and inner corners are curved. width < 0 (or
    // radius < 0) fails with invalid_argument.
    // clip: optional clipping rectangle. cache: optional draw cache to reuse.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_rounded_rectangle(Destination& destination, const rect16& rect, PixelType color, int16_t radius, int16_t width, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_rounded_rectangle_impl(destination, (srect16)rect, color, radius, width, cache, clip);
    }
    // draws an anti-aliased hollow rounded rectangle with a uniform circular corner
    // radius (>= 0) and an inner border of thickness 'width' (>= 0) that fits inside
    // the bounding rect. Both outer and inner corners are curved. width < 0 (or
    // radius < 0) fails with invalid_argument.
    // clip: optional clipping rectangle. cache: optional draw cache to reuse.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_rounded_rectangle(Destination& destination, const srect16& rect, PixelType color, int16_t radius, int16_t width, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_rounded_rectangle_impl(destination, rect, color, radius, width, cache, clip);
    }
};
}
}
#endif