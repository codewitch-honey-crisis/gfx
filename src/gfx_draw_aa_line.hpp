#ifndef HTCW_GFX_DRAW_AA_LINE_HPP
#define HTCW_GFX_DRAW_AA_LINE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"
namespace gfx {
namespace helpers {
class xdraw_aa_line {
    
    // Coverage is computed entirely in fixed point using only 32-bit words
    // Precondition: dx*dx + dy*dy must fit in 32 bits (each of |dx|,|dy| up to
    // ~46000 px). Every real display satisfies this with wide margin.
    //
    // The coverage model is coverage = clamp(0.5 - SDF), where SDF is the signed
    // distance to the stroke shape (negative inside), and the ramp is +/-0.5 px.
    // Internally dist16 = hw + SDF, so cov16 = band16 - dist16 = 0.5 - SDF. In the
    // body (perpendicular) region every cap yields the same SDF = perp - hw; only
    // the two end regions differ:
    //   round : SDF = dist_to_endpoint - hw           (a circular cap; exact sqrt)
    //   butt  : SDF = max(overshoot, perp - hw)       (flat cap; max-approx corner)
    //   square: SDF = max(overshoot, perp) - hw       (butt pushed out by hw)
    // The endpoint corners of butt/square use a max()-based box SDF (no sqrt), per
    // the requested approximation.
    template <typename Destination, typename PixelType>
    static gfx_result aa_line_impl(Destination& destination, const srect16& rect, PixelType color, int16_t width, line_cap cap, mask_draw_cache* cache, const srect16* clip) {
        if (width < 0) {
            return gfx_result::invalid_argument;
        }
        if (width == 0) {
            return gfx_result::success;  // nothing to draw
        }
        const uint8_t opacity = color.opacity8();
        // clip region = (caller clip, or the whole surface) cropped to the surface bounds
        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        srect16 bounds(spoint16(0, 0), ss);
        srect16 c = (nullptr != clip) ? clip->crop(bounds) : bounds;

        const int32_t x1 = rect.x1;
        const int32_t y1 = rect.y1;
        const int32_t x2 = rect.x2;
        const int32_t y2 = rect.y2;
        const int32_t dx = x2 - x1;
        const int32_t dy = y2 - y1;
        const int32_t len2 = dx * dx + dy * dy;

        // a zero-length stroke has no direction to extend; a butt cap draws nothing.
        // (round -> a dot, square -> a filled hw-square; both handled in the loop.)
        if (len2 == 0 && cap == line_cap::butt) {
            return gfx_result::success;
        }

        // fixed point 16.16 constants
        const int32_t half16 = 1 << 15;                   // 0.5
        const int32_t hw16 = (int32_t)width << 15;        // width / 2
        const int32_t hw_q8 = (int32_t)width << 7;        // width / 2, in Q8 (= hw16 >> 8)
        const int32_t band16 = hw16 + half16;             // hw + 0.5: max distance with coverage
        const int32_t band_px = (band16 + 0xFFFF) >> 16;  // ceil(band), for the cap cutoff
        const int32_t cap_cut2 = band_px * band_px;       // squared cap reach (round only)

        // per-segment length, in Q8 and rounded-integer forms (non-degenerate only)
        int32_t len_q8 = 0;   // |AB| * 256
        int32_t len_int = 0;  // round(|AB|)
        int32_t thresh = 0;   // |cross| above this has zero coverage (round(band)*(len+1))
        if (len2 != 0) {
            len_q8 = (int32_t)math::sqrt_ft32<8>((uint32_t)len2);
            len_int = (len_q8 + 128) >> 8;
            if (len_int < 1) len_int = 1;
            thresh = band_px * (len_int + 1);
        }

        // bounding box of the thick line, padded by the AA reach.
        // a square cap projects up to hw*sqrt2 past an endpoint on a diagonal line;
        // ~1.5*hw + 2 is a cheap, safe over-estimate. round/butt need only hw + 1.
        int pad;
        if (cap == line_cap::square) {
            pad = (int)(width >> 1) + (int)(width >> 2) + 2;
        } else {
            pad = (int)(width >> 1) + 1;
        }
        int minx = (x1 < x2 ? x1 : x2) - pad;
        int maxx = (x1 < x2 ? x2 : x1) + pad;
        int miny = (y1 < y2 ? y1 : y2) - pad;
        int maxy = (y1 < y2 ? y2 : y1) + pad;
        if (minx < c.x1) minx = c.x1;
        if (miny < c.y1) miny = c.y1;
        if (maxx > c.x2) maxx = c.x2;
        if (maxy > c.y2) maxy = c.y2;
        if (minx > maxx || miny > maxy) {
            return gfx_result::success;  // fully clipped away
        }

        // Use the caller's draw cache if supplied, otherwise a routine-local one.
        // The local instance is RAII: its buffer (if grown) is freed on return.
        mask_draw_cache local;
        mask_draw_cache* dc = (nullptr != cache) ? cache : &local;
        const int row_w = maxx - minx + 1;
        uint8_t* cov = dc->ensure((size_t)row_w);
        if (nullptr == cov) {
            return gfx_result::out_of_memory;
        }

        typename Destination::pixel_type fgpx;
        rgba_pixel<32> cfgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);
        convert_palette_to<Destination, rgba_pixel<32>>(destination, fgpx, &cfgpx);
        cfgpx.template channel<channel_name::A>(255);
        typename Destination::pixel_type bgpx, dpx;
        gfx_result r;

        for (int py = miny; py <= maxy; ++py) {
            const int32_t fy = py - y1;
            // pass 1: assemble this scanline's coverage into the cache buffer (fixed point)
            for (int px = minx; px <= maxx; ++px) {
                const int32_t fx = px - x1;
                int32_t dist16;  // distance to the segment, 16.16

                if (len2 == 0) {
                    // degenerate segment (butt already returned above)
                    if (cap == line_cap::square) {
                        // filled hw-square: axis-aligned box SDF (max() = exact here)
                        const int32_t ax = fx < 0 ? -fx : fx;
                        const int32_t ay = fy < 0 ? -fy : fy;
                        const int32_t m = ax > ay ? ax : ay;      // Chebyshev distance to center
                        const int32_t sdf_q8 = (m << 8) - hw_q8;  // (m - hw) in Q8
                        dist16 = hw16 + sdf_q8 * 256;
                    } else {
                        // round: an anti-aliased filled dot of radius hw
                        const int32_t g2 = fx * fx + fy * fy;
                        if (g2 > cap_cut2) {
                            cov[px - minx] = 0;
                            continue;
                        }
                        dist16 = (int32_t)math::sqrt_ft32<8>((uint32_t)g2) << 8;
                    }
                } else {
                    const int32_t along = fx * dx + fy * dy;  // = t * len2
                    if (along > 0 && along < len2) {
                        // body: perpendicular distance = |cross| / |AB|
                        const int32_t cross = fx * dy - fy * dx;
                        const int32_t acr = cross < 0 ? -cross : cross;
                        if (acr > thresh) {
                            cov[px - minx] = 0;
                            continue;
                        }
                        int32_t dist_q8;
                        if (acr < (1 << 15)) {
                            // small |cross|: use fractional length (acr<<16 fits uint32)
                            dist_q8 = (int32_t)(((uint32_t)acr << 16) / (uint32_t)len_q8);
                        } else {
                            // large |cross| (long segment): integer length is precise enough
                            dist_q8 = (acr << 8) / len_int;
                        }
                        dist16 = dist_q8 << 8;
                    } else {
                        // end region: before A (along <= 0) or after B (along >= len2)
                        if (cap == line_cap::round) {
                            int32_t g2;
                            if (along <= 0) {
                                g2 = fx * fx + fy * fy;  // round cap at A
                            } else {
                                const int32_t gx = px - x2, gy = py - y2;  // round cap at B
                                g2 = gx * gx + gy * gy;
                            }
                            if (g2 > cap_cut2) {
                                cov[px - minx] = 0;
                                continue;
                            }
                            dist16 = (int32_t)math::sqrt_ft32<8>((uint32_t)g2) << 8;
                        } else {
                            // butt or square: rectangle SDF via a max()-based box.
                            // over = overshoot past the near end, in |AB| units (>= 0);
                            // acr  = perpendicular distance, in |AB| units.
                            const int32_t over = (along <= 0) ? -along : (along - len2);
                            const int32_t cross = fx * dy - fy * dx;
                            const int32_t acr = cross < 0 ? -cross : cross;
                            if (acr > thresh) {
                                cov[px - minx] = 0;
                                continue;
                            }
                            // convert both to Q8 pixels (same division scheme as the body)
                            int32_t perp_q8;
                            if (acr < (1 << 15)) {
                                perp_q8 = (int32_t)(((uint32_t)acr << 16) / (uint32_t)len_q8);
                            } else {
                                perp_q8 = (acr << 8) / len_int;
                            }
                            const int32_t over_q8 = (int32_t)(((uint32_t)over << 8) / (uint32_t)len_int);
                            int32_t sdf_q8;
                            if (cap == line_cap::square) {
                                // flat end pushed out by hw: max(over, perp) - hw
                                sdf_q8 = (over_q8 > perp_q8 ? over_q8 : perp_q8) - hw_q8;
                            } else {
                                // butt: flat end at the endpoint: max(over, perp - hw)
                                const int32_t pp = perp_q8 - hw_q8;
                                sdf_q8 = over_q8 > pp ? over_q8 : pp;
                            }
                            dist16 = hw16 + sdf_q8 * 256;  // dist = hw + SDF (SDF may be < 0)
                        }
                    }
                }
                const int32_t cov16 = band16 - dist16;  // coverage in 16.16
                uint8_t c8;
                if (cov16 <= 0)
                    c8 = 0;
                else if (cov16 >= (1 << 16))
                    c8 = 255;  // solid interior
                else
                    c8 = (uint8_t)(cov16 >> 8);  // 16.16 -> 0..255

                cov[px - minx] = c8 ;
            }
            r=aa_rasterize_row(destination,{(int16_t)minx,(int16_t)py},cov,row_w,fgpx,opacity);
            if(r!=gfx_result::success) {
                return r;
            }
        }
        return gfx_result::success;
    }

   public:
    // draws an anti-aliased line of the given width (>= 0), color and end cap.
    // cap: butt (flat, default), round, or square. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_line(Destination& destination, const rect16& rect, PixelType color, int16_t width, line_cap cap = line_cap::butt, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_line_impl(destination, (srect16)rect, color, width, cap, cache, clip);
    }
    // draws an anti-aliased line of the given width (>= 0), color and end cap.
    // cap: butt (flat, default), round, or square. clip: optional clipping rectangle.
    // cache: optional draw cache to reuse across calls.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_line(Destination& destination, const srect16& rect, PixelType color, int16_t width, line_cap cap = line_cap::butt, mask_draw_cache* cache = nullptr, const srect16* clip = nullptr) {
        return aa_line_impl(destination, rect, color, width, cap, cache, clip);
    }
};
}  // namespace helpers
}  // namespace gfx
#endif