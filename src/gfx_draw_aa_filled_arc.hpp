#ifndef HTCW_GFX_DRAW_FILLED_ARC_HPP
#define HTCW_GFX_DRAW_FILLED_ARC_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"

// 64-bit intermediates are used only to widen two products. Where the platform
// has no 64-bit word we fall back to an exact 32-bit form for the squared radius
// and a split-divide approximation for the angular test (see plane_dist below).
// The routine is fully functional either way; nothing is compiled out.
#if !defined(HTCW_MAX_WORD) || (HTCW_MAX_WORD >= 64)
#define GFX_FILLED_ARC_HAS64 1
#else
#define GFX_FILLED_ARC_HAS64 0
#endif

namespace gfx {
namespace helpers {
class xdraw_aa_filled_arc {
    // Anti-aliased FILLED arc -- a pie wedge (circular sector). Same coverage
    // model as xdraw_aa_line, xdraw_aa_rounded_rectangle and xdraw_aa_arc:
    // coverage = clamp(0.5 - SDF), a +/-0.5 px ramp around the signed distance to
    // the shape (SDF negative inside). All math is fixed point; only the
    // per-pixel radial distance uses a digit-by-digit sqrt.
    //
    // GEOMETRY
    //   The disc fills the largest square that fits in the UPPER-LEFT of the
    //   bounding rect: side S = min(box_w, box_h), radius R = S/2, centered on
    //   that square. Identical placement rule to xdraw_aa_arc, so a stroked arc
    //   and a filled arc drawn into the same rect are concentric and co-radial.
    //   The wedge is bounded by the disc plus two straight radial edges running
    //   from the CENTER out to the arc -- there is no hole and no stroke width,
    //   hence no width or line_cap parameter.
    //
    // ANGLES (degrees, LVGL convention)
    //   0 deg = east (3 o'clock); angle increases CLOCKWISE on screen (y down).
    //   The wedge is swept CLOCKWISE from start_angle to end_angle. Callers pass
    //   start < end; the drawn sweep is (end - start):
    //     sweep <= 0   -> nothing (empty/inverted span)
    //     sweep >= 360 -> the whole disc (angular test skipped)
    //     else         -> the [start, start+sweep] wedge.
    //   Negative / out-of-range start angles are normalized (e.g. -30 -> 330).
    //   Pac-Man facing east is start=30, end=330 (a reflex sweep of 300).
    //
    // SDF DECOMPOSITION (all in Q8 pixels)
    //   For a pixel let dist = |p - center|. The disc SDF is
    //     body = dist - R.
    //   Angular containment is decided WITHOUT atan2, using the two end "planes"
    //   (each the radial line through an endpoint, passing through the center).
    //   os/oe are the signed perpendicular distances past the start/end plane
    //   (>0 == outside that end). Because both planes pass exactly through the
    //   center, os and oe are EXACT distances to the wedge's straight edges --
    //   so those edges antialias exactly, with no cap approximation anywhere.
    //     convex sweep (<=180): wedge = disc AND half AND half
    //                           sdf = max(body, os, oe)
    //     reflex sweep (>180):  wedge = disc AND (half OR half)
    //                           sdf = max(body, min(os, oe))
    //   The only approximate points are the two outer corners where a straight
    //   edge meets the arc (max() slightly overestimates the outside distance)
    //   and, for a reflex sweep, the reentrant notch at the center (min()
    //   slightly underestimates). Both are sub-pixel and read as a hair of extra
    //   softness on those three points.
    //
    // PARITY (odd vs even S)
    //   The center is tracked at HALF-pixel resolution: ux_q8 = (2*px - cx2)<<7
    //   where cx2 = sx1+sx2 is twice the center x. For even S the center falls on
    //   a pixel boundary, for odd S on a pixel center; both are represented
    //   exactly and neither is rounded to an integer pixel. There is no
    //   parity-dependent distortion -- unlike a midpoint/Bresenham ellipse, which
    //   must pick an integer center pixel and goes out of round on odd diameters.
    //
    // EDGE INSET (uniform softness)
    //   With R exactly S/2 the disc is tangent to the box on all four sides, so
    //   at N/S/E/W the boundary lands exactly on the pixel grid: the outermost
    //   pixel is 255 and its neighbour 0, a hard step, while every off-axis pixel
    //   gets a proper ramp. Pulling R in a fraction of a pixel puts the boundary
    //   strictly inside the outer pixel in every direction:
    //     0 = exact tangency (crisp, four flat spots), 64 = 1/4 px (default),
    //     128 = 1/2 px (uniformly soft, visibly smaller).
    //   The straight edges are unaffected -- they already ramp at any angle.
    //
    // WORD SIZE
    //   With a 64-bit word available the angular test is computed at full width.
    //   Without one it divides each product by 32767 before subtracting, which
    //   costs up to 2 units of Q8 (~1/128 px) of error on the two straight edges
    //   -- at most 2/255 of a coverage step, i.e. below the output's own
    //   resolution. The radial term is exact in both cases. See plane_dist().
    //
    // Precondition: at the working radius, dist*dist must fit in 32 bits, i.e.
    // S up to ~360 (the 32-bit angular path is good to S ~510, so the sqrt is
    // always the binding limit). Every real display satisfies this with a wide
    // margin, matching the sibling routines' preconditions.

    // how far inside the bounding square the arc edge sits, in Q8 pixels.
    static constexpr const int32_t edge_inset_q8 = 64;

    // squared distance from center, Q16. Exact and 32-bit-only on every platform:
    // the magnitudes are taken first so the products are unsigned, which both
    // avoids signed overflow and buys the extra bit that lets the sum reach the
    // stated S ~360 precondition.
    static inline uint32_t rad_sq(int32_t ux_q8, int32_t uy_q8) {
        const uint32_t ax = (uint32_t)(ux_q8 < 0 ? -ux_q8 : ux_q8);
        const uint32_t ay = (uint32_t)(uy_q8 < 0 ? -uy_q8 : uy_q8);
        return ax * ax + ay * ay;
    }

    // signed perpendicular distance (Q8) from the pixel offset (ux,uy) to the
    // radial line at angle (sn,cs) -- positive on the clockwise side. That is
    // (ux*sn - uy*cs)/32767.
    static inline int32_t plane_dist(int32_t ux_q8, int32_t uy_q8, int32_t sn, int32_t cs) {
#if GFX_FILLED_ARC_HAS64
        return (int32_t)(((int64_t)ux_q8 * sn - (int64_t)uy_q8 * cs) / 32767);
#else
        // each product alone fits a signed 32-bit word for any S up to ~510; only
        // their difference can overflow, so divide first and subtract after. Both
        // divisions truncate toward zero, so the error is bounded by 2 Q8 units
        // and is symmetric about the line.
        return (ux_q8 * sn) / 32767 - (uy_q8 * cs) / 32767;
#endif
    }

    template <typename Destination, typename PixelType>
    static gfx_result aa_filled_arc_impl(Destination& destination, const srect16& rect, PixelType color, int16_t start_angle, int16_t end_angle, mask_draw_cache* cache, const srect16* clip) {
        const int32_t sweep = (int32_t)end_angle - (int32_t)start_angle;
        if (sweep <= 0) {
            return gfx_result::success; // empty / inverted span
        }
        const bool full = sweep >= 360; // whole disc: skip the angular test

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
        const int32_t rx1 = rect.x1 < rect.x2 ? rect.x1 : rect.x2;
        const int32_t rx2 = rect.x1 < rect.x2 ? rect.x2 : rect.x1;
        const int32_t ry1 = rect.y1 < rect.y2 ? rect.y1 : rect.y2;
        const int32_t ry2 = rect.y1 < rect.y2 ? rect.y2 : rect.y1;
        const int32_t box_w = rx2 - rx1 + 1;
        const int32_t box_h = ry2 - ry1 + 1;

        // largest upper-left square; the disc fills it.
        const int32_t S = box_w < box_h ? box_w : box_h;
        if (S <= 0) {
            return gfx_result::success;
        }
        const int32_t sx1 = rx1, sy1 = ry1;
        const int32_t sx2 = rx1 + S - 1, sy2 = ry1 + S - 1;

        // fixed point 16.16 / Q8 constants
        const int32_t half16 = 1 << 15;                 // 0.5 in 16.16
        int32_t R_q8 = (S << 7) - edge_inset_q8;        // radius (S/2) in Q8, inset
        if (R_q8 <= 0) R_q8 = S << 7;                   // 1px box: keep it visible
        const int32_t cx2 = sx1 + sx2;                  // 2 * center x
        const int32_t cy2 = sy1 + sy2;                  // 2 * center y

        // endpoint trig (0deg=east, clockwise, y down); sin/cos scaled by 32767.
        // both angles are reduced to [0,360) before the table lookup.
        int32_t s = start_angle % 360; if (s < 0) s += 360;
        const int32_t e = full ? s : ((s + sweep) % 360);
        const bool reflex = (!full) && (sweep > 180);
        const int32_t sin_s = math::sin((int16_t)s), cos_s = math::cos((int16_t)s);
        const int32_t sin_e = math::sin((int16_t)e), cos_e = math::cos((int16_t)e);

        // AA work area: the square padded by 1px, clamped to the clip region.
        int minx = sx1 - 1, maxx = sx2 + 1, miny = sy1 - 1, maxy = sy2 + 1;
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

        // single round-tripped foreground (dest-grid RGB, opaque; opacity applied
        // via the per-pixel blend factor). Only one draw path here, so no seam.
        typename Destination::pixel_type fgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);
        rgba_pixel<32> cfgpx;
        convert_palette_to<Destination, rgba_pixel<32>>(destination, fgpx, &cfgpx);
        cfgpx.opacity8_inplace(255);

        for (int py = miny; py <= maxy; ++py) {
            const int32_t uy_q8 = (2 * py - cy2) << 7;      // (py - cy) in Q8

            // rows entirely outside the disc contribute nothing; skip the sqrt work.
            const int32_t auy_q8 = uy_q8 < 0 ? -uy_q8 : uy_q8;
            if (auy_q8 - R_q8 >= 128) {                     // SDF >= 0.5 px for all x
                continue;
            }

            // pass 1: assemble this scanline's wedge coverage into the cache buffer
            for (int px = minx; px <= maxx; ++px) {
                const int32_t ux_q8 = (2 * px - cx2) << 7;  // (px - cx) in Q8

                // radial distance from center, and the disc (body) SDF.
                const uint32_t V = rad_sq(ux_q8, uy_q8);
                const int32_t dist_q8 = (int32_t)(math::sqrt_ft32<8>(V) >> 8);
                const int32_t body = dist_q8 - R_q8;

                int32_t sdf_q8;
                if (full) {
                    sdf_q8 = body;
                } else {
                    // signed distance past each end plane (>0 = outside). the planes
                    // pass through the center, so these are the true distances to
                    // the wedge's two straight edges.
                    const int32_t os =  plane_dist(ux_q8, uy_q8, sin_s, cos_s);
                    const int32_t oe = -plane_dist(ux_q8, uy_q8, sin_e, cos_e);
                    // convex sweep = intersection of half-planes; reflex = union.
                    const int32_t ends = reflex ? (os < oe ? os : oe)   // min: union
                                                : (os > oe ? os : oe);  // max: intersection
                    sdf_q8 = body > ends ? body : ends;                 // AND with the disc
                }

                const int32_t cov16 = half16 - (sdf_q8 << 8); // 0.5 - SDF, in 16.16
                uint32_t c8;
                if (cov16 <= 0) c8 = 0;
                else if (cov16 >= (1 << 16)) c8 = 255;        // solid interior
                else c8 = (uint32_t)(cov16 >> 8);             // 16.16 -> 0..255

                cov[px - minx] = (uint8_t)((c8 * (uint32_t)opacity) / 255u);
            }

            // pass 2: blend the covered pixels, each touched exactly once
            aa_rasterize_row(destination, {(int16_t)minx, (int16_t)py}, cov, row_w, fgpx);
        }
        return gfx_result::success;
    }

public:
    // draws an anti-aliased filled arc (pie wedge) inscribed in the largest
    // upper-left square of 'rect'. Angles are in degrees (0=east, clockwise); the
    // wedge sweeps clockwise from start_angle to end_angle. sweep >= 360 fills the
    // whole disc; sweep <= 0 draws nothing. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_arc(Destination& destination, const rect16& bounds, PixelType color, int16_t start_angle, int16_t end_angle, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_filled_arc_impl(destination, (srect16)bounds, color, start_angle, end_angle, cache, clip);
    }
    // draws an anti-aliased filled arc (pie wedge) inscribed in the largest
    // upper-left square of 'rect'. Angles are in degrees (0=east, clockwise); the
    // wedge sweeps clockwise from start_angle to end_angle. sweep >= 360 fills the
    // whole disc; sweep <= 0 draws nothing. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_arc(Destination& destination, const srect16& bounds, PixelType color, int16_t start_angle, int16_t end_angle, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_filled_arc_impl(destination, bounds, color, start_angle, end_angle, cache, clip);
    }
};
}
}
#undef GFX_FILLED_ARC_HAS64
#endif