#ifndef HTCW_GFX_DRAW_AA_ARC_HPP
#define HTCW_GFX_DRAW_AA_ARC_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"
namespace gfx {
namespace helpers {
class xdraw_aa_arc {
    // Anti-aliased circular ARC (a stroked, possibly-capped ring segment). Same
    // coverage model as xdraw_aa_line and xdraw_aa_rounded_rectangle:
    // coverage = clamp(0.5 - SDF), a +/-0.5 px ramp around the signed distance
    // to the shape (SDF negative inside). All math is fixed point; only the two
    // per-pixel radial distances use a digit-by-digit sqrt.
    //
    // GEOMETRY
    //   The circle is the largest square that fits in the UPPER-LEFT of the
    //   bounding rect: side S = min(box_w, box_h), so the OUTER radius sits right
    //   at that square's edge (R_out = S/2). The border is stroked INWARD by
    //   'width', giving inner radius R_in = R_out - width (exactly like the inward
    //   border of the rounded-rectangle routine). A width that reaches the center
    //   leaves no hole and yields a filled disc sector.
    //
    // ANGLES (degrees, LVGL convention)
    //   0 deg = east (3 o'clock); angle increases CLOCKWISE on screen (y down).
    //   The arc is swept CLOCKWISE from start_angle to end_angle. Callers pass
    //   start < end; the drawn sweep is (end - start):
    //     sweep <= 0   -> nothing (empty/inverted span)
    //     sweep >= 360 -> full ring (caps are irrelevant)
    //     else         -> the [start, start+sweep] segment, with end caps.
    //   Negative / out-of-range start angles are normalized (e.g. -30 -> 330).
    //
    // SDF DECOMPOSITION (all in Q8 pixels)
    //   Let Rc = centerline radius = R_out - hw, hw = width/2, and for a pixel let
    //   dist = |p - center|. The BODY (angularly inside the sweep) signed distance
    //   is the ring SDF:      band = |dist - Rc| - hw.
    //   Whether a pixel is angularly inside is decided WITHOUT atan2, using the two
    //   end "planes" (each the radial line through an endpoint). os/oe are the
    //   signed tangential distances past the start/end plane (>0 == outside that
    //   end); near the ring the plane normal is the local arc tangent, so os/oe are
    //   the arc-tangential overshoot -- the exact analogue of the line routine's
    //   'over'. A convex sweep (<=180) is the intersection of the two half-planes;
    //   a reflex sweep (>180) is their union, hence the branch below.
    //   In an end region the cap SDF matches xdraw_aa_line exactly:
    //     round : dist_to_endpoint - hw            (circular cap; exact sqrt)
    //     butt  : max(over, band)                  (flat radial end; max-approx)
    //     square: max(over, |dist-Rc|) - hw        (butt pushed out by hw)
    //   When a pixel is past both ends (the notch of a reflex sweep, or the far
    //   side of a convex one) the two caps are combined with min(): the nearest
    //   cap face is the true boundary there.
    //
    // Precondition: at the working radius, dist*dist must fit in 32 bits, i.e.
    // R_out up to ~180 px (diameter S up to ~360). Every real display satisfies
    // this with a wide margin, matching the sibling routines' preconditions.
    template <typename Destination, typename PixelType>
    static gfx_result aa_arc_impl(Destination& destination, const srect16& rect, PixelType color, int16_t start_angle, int16_t end_angle, int16_t width, line_cap cap, mask_draw_cache* cache, const srect16* clip) {
        if (width < 0) {
            return gfx_result::invalid_argument;
        }
        if (width == 0) {
            return gfx_result::success; // zero-thickness border: nothing to draw
        }
        const int32_t sweep = (int32_t)end_angle - (int32_t)start_angle;
        if (sweep <= 0) {
            return gfx_result::success; // empty / inverted span
        }
        const bool full = sweep >= 360; // full ring: skip the angular test, no caps

        const uint8_t opacity = helpers::pixel_get_alpha_255<PixelType,PixelType::has_alpha>::value(color);
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

        // largest upper-left square; the circle fills it, outer radius at its edge.
        const int32_t S = box_w < box_h ? box_w : box_h;
        if (S <= 0) {
            return gfx_result::success;
        }
        const int32_t sx1 = rx1, sy1 = ry1;
        const int32_t sx2 = rx1 + S - 1, sy2 = ry1 + S - 1;

        // clamp border width so the inner radius can't go negative (>= filled disc).
        int32_t w = width;
        if (2 * w > S) w = S / 2;
        if (w <= 0) {
            return gfx_result::success;
        }

        // fixed point 16.16 / Q8 constants
        const int32_t half16   = 1 << 15;          // 0.5 in 16.16
        const int32_t R_out_q8 = S << 7;           // outer radius (S/2) in Q8
        const int32_t hw_q8    = w << 7;           // half border (w/2) in Q8
        const int32_t Rc_q8    = R_out_q8 - hw_q8; // band centerline radius (Q8)
        const int32_t cx2      = sx1 + sx2;        // 2 * center x
        const int32_t cy2      = sy1 + sy2;        // 2 * center y

        // endpoint trig (0deg=east, clockwise, y down); sin/cos scaled by 32767.
        int32_t s = start_angle % 360; if (s < 0) s += 360;
        const int32_t e = s + (full ? 0 : sweep);
        const bool reflex = (!full) && (sweep > 180);
        const int32_t sin_s = math::sin((int16_t)s), cos_s = math::cos((int16_t)s);
        const int32_t sin_e = math::sin((int16_t)e), cos_e = math::cos((int16_t)e);

        // centerline endpoints as Q8 offsets from center (for round caps).
        const int32_t Psx_q8 = (int32_t)(((int64_t)Rc_q8 * cos_s) / 32767);
        const int32_t Psy_q8 = (int32_t)(((int64_t)Rc_q8 * sin_s) / 32767);
        const int32_t Pex_q8 = (int32_t)(((int64_t)Rc_q8 * cos_e) / 32767);
        const int32_t Pey_q8 = (int32_t)(((int64_t)Rc_q8 * sin_e) / 32767);

        // AA work area: the square padded by 1px (round/square caps stay within the
        // outer circle, so no extra pad is needed), clamped to the clip region.
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
        typename Destination::pixel_type bgpx, dpx;
        gfx_result r_res;

        for (int py = miny; py <= maxy; ++py) {
            const int32_t uy_q8 = (2 * py - cy2) << 7;      // (py - cy) in Q8

            // pass 1: assemble this scanline's arc coverage into the cache buffer
            for (int px = minx; px <= maxx; ++px) {
                const int32_t ux_q8 = (2 * px - cx2) << 7;  // (px - cx) in Q8

                // radial distance from center, and the ring (body) SDF.
                const uint32_t V = (uint32_t)((int64_t)ux_q8 * ux_q8 + (int64_t)uy_q8 * uy_q8);
                const int32_t dist_q8 = (int32_t)((int64_t)math::sqrt_ft32<8>(V) >> 8);
                const int32_t dr_q8  = dist_q8 - Rc_q8;
                const int32_t adr_q8 = dr_q8 < 0 ? -dr_q8 : dr_q8;   // |radial offset|
                const int32_t band   = adr_q8 - hw_q8;               // body SDF (Q8)

                int32_t sdf_q8;
                if (full) {
                    sdf_q8 = band;
                } else {
                    // signed tangential distance past each end plane (>0 = outside)
                    const int32_t os = (int32_t)(( (int64_t)ux_q8 * sin_s - (int64_t)uy_q8 * cos_s) / 32767);
                    const int32_t oe = (int32_t)((-(int64_t)ux_q8 * sin_e + (int64_t)uy_q8 * cos_e) / 32767);
                    // convex sweep = intersection of half-planes; reflex = union.
                    const bool in_body = reflex ? (os <= 0 || oe <= 0) : (os <= 0 && oe <= 0);
                    if (in_body) {
                        sdf_q8 = band;
                    } else {
                        // end region: combine the cap(s) of whichever end(s) we're past.
                        bool have = false; int32_t best = 0;
                        if (os > 0) {
                            int32_t cand;
                            if (cap == line_cap::round) {
                                const int32_t gx = ux_q8 - Psx_q8, gy = uy_q8 - Psy_q8;
                                const uint32_t Vc = (uint32_t)((int64_t)gx * gx + (int64_t)gy * gy);
                                cand = (int32_t)((int64_t)math::sqrt_ft32<8>(Vc) >> 8) - hw_q8;
                            } else if (cap == line_cap::square) {
                                cand = (os > adr_q8 ? os : adr_q8) - hw_q8;
                            } else { // butt
                                cand = os > band ? os : band;
                            }
                            best = cand; have = true;
                        }
                        if (oe > 0) {
                            int32_t cand;
                            if (cap == line_cap::round) {
                                const int32_t gx = ux_q8 - Pex_q8, gy = uy_q8 - Pey_q8;
                                const uint32_t Vc = (uint32_t)((int64_t)gx * gx + (int64_t)gy * gy);
                                cand = (int32_t)((int64_t)math::sqrt_ft32<8>(Vc) >> 8) - hw_q8;
                            } else if (cap == line_cap::square) {
                                cand = (oe > adr_q8 ? oe : adr_q8) - hw_q8;
                            } else { // butt
                                cand = oe > band ? oe : band;
                            }
                            if (!have || cand < best) { best = cand; have = true; }
                        }
                        sdf_q8 = best; // 'have' is always true here (past at least one end)
                    }
                }

                const int32_t cov16 = half16 - (sdf_q8 << 8); // 0.5 - SDF, in 16.16
                uint8_t c8;
                if (cov16 <= 0) c8 = 0;
                else if (cov16 >= (1 << 16)) c8 = 255;        // solid part of the border
                else c8 = (uint8_t)(cov16 >> 8);              // 16.16 -> 0..255

                cov[px - minx] = (uint8_t)(c8 * opacity / 255);
            }

            // pass 2: blend the covered pixels, each touched exactly once
            for (int i = 0; i < row_w; ++i) {
                const uint8_t c8 = cov[i];
                if (0 == c8) continue;
                const point16 p((uint16_t)(minx + i), (uint16_t)py);
                if (c8 < 255) {
                    r_res = destination.point(p, &bgpx);
                    if (gfx_result::success != r_res) return r_res;
                    dpx = fgpx.blend8(bgpx, c8);
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
    // draws an anti-aliased circular arc whose outer edge sits at the largest
    // upper-left square of 'rect', stroked inward by 'width' (>= 0). Angles are in
    // degrees (0=east, clockwise); the arc sweeps clockwise from start_angle to
    // end_angle. sweep >= 360 draws a full ring; sweep <= 0 draws nothing.
    // cap: butt (flat, default), round, or square. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_arc(Destination& destination, const rect16& rect, PixelType color, int16_t start_angle, int16_t end_angle, int16_t width, line_cap cap = line_cap::butt, mask_draw_cache* cache = nullptr,const srect16* clip = nullptr) {
        return aa_arc_impl(destination, (srect16)rect, color, start_angle, end_angle, width, cap, cache, clip);
    }
    // draws an anti-aliased circular arc whose outer edge sits at the largest
    // upper-left square of 'rect', stroked inward by 'width' (>= 0). Angles are in
    // degrees (0=east, clockwise); the arc sweeps clockwise from start_angle to
    // end_angle. sweep >= 360 draws a full ring; sweep <= 0 draws nothing.
    // cap: butt (flat, default), round, or square. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_arc(Destination& destination, const srect16& rect, PixelType color, int16_t start_angle, int16_t end_angle, int16_t width, line_cap cap = line_cap::butt, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_arc_impl(destination, rect, color, start_angle, end_angle, width, cap, cache, clip);
    }
};
}
}
#endif