#ifndef HTCW_GFX_DRAW_FILLED_POLYGON
#define HTCW_GFX_DRAW_FILLED_POLYGON
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
#include "gfx_mask_draw_cache.hpp"

// number of vertical sub-samples per output row for the AA fill.
// must divide 255 exactly so that SS sub-rows sum to a full 255 with no
// clamping: legal values are 3, 5, 15, 17, 51, 85, 255.
// 5 is a good default; 15 visibly improves near-horizontal edges for 3x the
// edge-list work.
#ifndef HTCW_GFX_AA_POLY_SUBSAMPLES
#define HTCW_GFX_AA_POLY_SUBSAMPLES 5
#endif

namespace gfx {

namespace helpers {

class xdraw_filled_polygon {
    // Scanline crossing core, shared by both fills.
    //
    // A sample line is placed at a strictly fractional y, so no vertex can ever
    // sit exactly on it. That removes every vertex-on-scanline special case:
    // horizontal edges have equal endpoint y and are simply never counted, which
    // also means the divide below can never see dy == 0.
    //
    // Precondition: |vertex.y - drawn row| < 32768. True for any geometry within
    // a few thousand pixels of the surface. If callers may pass wildly
    // off-surface vertices, widen the numerator in edge_x() to int64_t.

    // x where edge (a,b) meets sample line ys16 (16.16), returned in Q8.
    static inline int32_t edge_x(const spoint16& a, const spoint16& b, int32_t ys16) {
        const int32_t ay16 = (int32_t)a.y << 16;
        const int32_t dy = (int32_t)b.y - (int32_t)a.y;
        const int32_t dx = (int32_t)b.x - (int32_t)a.x;
        const int32_t t16 = (ys16 - ay16) / dy;  // 0..65536
        return ((int32_t)a.x << 8) + ((t16 * dx) >> 8);
    }

    // ---- AA path ------------------------------------------------------------

    static const int32_t ss_count = HTCW_GFX_AA_POLY_SUBSAMPLES;
    static const int32_t ss_step = 255 / HTCW_GFX_AA_POLY_SUBSAMPLES;

    // accumulate a Q8 span [xs,xe) into the coverage row at weight ss_step.
    // spans within one sub-row are disjoint, and ss_count * ss_step == 255,
    // so cov[] provably never overflows a byte - no clamping needed.
    static void add_span(uint8_t* cov, int minx, int maxx, int32_t xs, int32_t xe,
                         int* dirty_lo, int* dirty_hi) {
        const int32_t lo = (int32_t)minx << 8;
        const int32_t hi = ((int32_t)maxx + 1) << 8;
        if (xs < lo) xs = lo;
        if (xe > hi) xe = hi;
        if (xe <= xs) return;
        const int ip0 = (int)(xs >> 8);
        const int ip1 = (int)(xe >> 8);
        if (ip0 - minx < *dirty_lo) *dirty_lo = ip0 - minx;
        if (ip0 == ip1) {
            cov[ip0 - minx] = (uint8_t)(cov[ip0 - minx] + (((xe - xs) * ss_step) >> 8));
            if (ip0 - minx > *dirty_hi) *dirty_hi = ip0 - minx;
            return;
        }
        cov[ip0 - minx] = (uint8_t)(cov[ip0 - minx] + (((256 - (xs & 255)) * ss_step) >> 8));
        for (int p = ip0 + 1; p < ip1; ++p) {
            cov[p - minx] = (uint8_t)(cov[p - minx] + ss_step);
        }
        int last = ip1 - 1;
        const int32_t frac = xe & 255;
        if (frac) {
            cov[ip1 - minx] = (uint8_t)(cov[ip1 - minx] + ((frac * ss_step) >> 8));
            last = ip1;
        }
        if (last - minx > *dirty_hi) *dirty_hi = last - minx;
    }

    // insertion sort - crossing counts are small and usually near-sorted
    static void sort_crossings(int32_t* a, int n) {
        for (int i = 1; i < n; ++i) {
            const int32_t v = a[i];
            int j = i - 1;
            while (j >= 0 && a[j] > v) {
                a[j + 1] = a[j];
                --j;
            }
            a[j + 1] = v;
        }
    }

    template <typename Destination, typename PixelType>
    static gfx_result aa_filled_polygon_impl(Destination& destination, const spath16& path,
                                             PixelType color, fill_rule rule,
                                             mask_draw_cache* cache, const srect16* clip) {
        const size_t n = path.size();
        if (n < 3) {
            return gfx_result::success;
        }
        const uint8_t opacity = color.opacity8();
        if (0 == opacity) {
            return gfx_result::success;
        }
        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        srect16 bounds(spoint16(0, 0), ss);
        srect16 c = (nullptr != clip) ? clip->crop(bounds) : bounds;
        srect16 pr = path.bounds().crop(c);
        if (pr.x1 > pr.x2 || pr.y1 > pr.y2) {
            return gfx_result::success;
        }
        const int minx = pr.x1, maxx = pr.x2;
        const int row_w = maxx - minx + 1;

        // one allocation for both the crossing list and the coverage row.
        // the int32 array goes first so it inherits the buffer's alignment.
        mask_draw_cache local;
        mask_draw_cache* dc = (nullptr != cache) ? cache : &local;
        const size_t xbytes = n * sizeof(int32_t);
        uint8_t* buf = dc->ensure(xbytes + (size_t)row_w);
        if (nullptr == buf) {
            return gfx_result::out_of_memory;
        }
        int32_t* cross = (int32_t*)buf;
        uint8_t* cov = buf + xbytes;

        typename Destination::pixel_type fgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);

        const spoint16* pts = path.begin();
        gfx_result r;

        for (int py = pr.y1; py <= pr.y2; ++py) {
            memset(cov, 0, (size_t)row_w);
            int dirty_lo = row_w, dirty_hi = -1;

            for (int32_t s = 0; s < ss_count; ++s) {
                // y = py + (2s+1)/(2*ss_count), in 16.16. never an integer.
                const int32_t ys16 = ((int32_t)py << 16) +
                                     (int32_t)(((2 * s + 1) << 16) / (2 * ss_count));
                int nc = 0;
                for (size_t i = 0; i < n; ++i) {
                    const spoint16& a = pts[i];
                    const spoint16& b = pts[(i + 1 == n) ? 0 : i + 1];
                    const bool a_above = ((int32_t)a.y << 16) < ys16;
                    if (a_above == (((int32_t)b.y << 16) < ys16)) {
                        continue;  // no crossing (covers horizontal edges: dy != 0 below)
                    }
                    // pack winding direction into bit 0 so a single sort orders both
                    cross[nc++] = (edge_x(a, b, ys16) << 1) | (a_above ? 1 : 0);
                }
                if (nc < 2) continue;
                sort_crossings(cross, nc);

                if (fill_rule::even_odd == rule) {
                    for (int k = 0; k + 1 < nc; k += 2) {
                        add_span(cov, minx, maxx, cross[k] >> 1, cross[k + 1] >> 1,
                                 &dirty_lo, &dirty_hi);
                    }
                } else {
                    int w = 0;
                    int32_t xs = 0;
                    for (int k = 0; k < nc; ++k) {
                        const int pw = w;
                        w += (cross[k] & 1) ? 1 : -1;
                        if (0 == pw && 0 != w) {
                            xs = cross[k] >> 1;
                        } else if (0 != pw && 0 == w) {
                            add_span(cov, minx, maxx, xs, cross[k] >> 1,
                                     &dirty_lo, &dirty_hi);
                        }
                    }
                }
            }
            if (dirty_hi < dirty_lo) {
                continue;  // row is empty - skip the blend entirely
            }
            r = aa_rasterize_row(destination,
                                 spoint16((int16_t)(minx + dirty_lo), (int16_t)py),
                                 cov + dirty_lo, (size_t)(dirty_hi - dirty_lo + 1),
                                 fgpx, opacity);
            if (gfx_result::success != r) {
                return r;
            }
        }
        return gfx_result::success;
    }

    // ---- non-AA path --------------------------------------------------------

    // Find the next crossing strictly after (last_x, last_edge) in (x, edge)
    // order. O(N) per crossing, so O(N^2) per row - but with zero storage, which
    // keeps this function allocation-free. For the vertex counts a raster fill
    // realistically sees, N^2 still beats the old O(N) per *pixel* test by a
    // wide margin.
    static bool next_crossing(const spoint16* pts, size_t n, int32_t ys16,
                              int32_t last_x, int last_edge,
                              int32_t* out_x, int* out_edge, int* out_dir) {
        bool found = false;
        for (size_t i = 0; i < n; ++i) {
            const spoint16& a = pts[i];
            const spoint16& b = pts[(i + 1 == n) ? 0 : i + 1];
            const bool a_above = ((int32_t)a.y << 16) < ys16;
            if (a_above == (((int32_t)b.y << 16) < ys16)) {
                continue;
            }
            const int32_t x = edge_x(a, b, ys16);
            if (x < last_x || (x == last_x && (int)i <= last_edge)) {
                continue;
            }
            if (!found || x < *out_x || (x == *out_x && (int)i < *out_edge)) {
                found = true;
                *out_x = x;
                *out_edge = (int)i;
                *out_dir = a_above ? 1 : -1;
            }
        }
        return found;
    }

    template <typename Destination, typename PixelType>
    static gfx_result filled_polygon_impl(Destination& destination, const spath16& path,
                                          PixelType color, fill_rule rule,
                                          const srect16* clip) {
        const size_t n = path.size();
        if (n < 3) {
            return gfx_result::success;
        }
        srect16 pr = path.bounds();
        if (nullptr != clip) {
            pr = pr.crop(*clip);
        }
        if (pr.x1 > pr.x2 || pr.y1 > pr.y2) {
            return gfx_result::success;
        }
        const spoint16* pts = path.begin();
        gfx_result r;

        for (int py = pr.y1; py <= pr.y2; ++py) {
            const int32_t ys16 = ((int32_t)py << 16) + 32768;  // pixel centre
            int32_t last_x = INT32_MIN;
            int last_edge = -1;
            int w = 0, parity = 0;
            int32_t xs = 0;
            for (;;) {
                int32_t x;
                int edge, dir;
                if (!next_crossing(pts, n, ys16, last_x, last_edge, &x, &edge, &dir)) {
                    break;
                }
                last_x = x;
                last_edge = edge;
                bool was_in, now_in;
                if (fill_rule::even_odd == rule) {
                    was_in = 0 != parity;
                    parity ^= 1;
                    now_in = 0 != parity;
                } else {
                    was_in = 0 != w;
                    w += dir;
                    now_in = 0 != w;
                }
                if (!was_in && now_in) {
                    xs = x;
                } else if (was_in && !now_in) {
                    // pixels whose centre lies in [xs, x)
                    int px0 = (int)((xs - 128 + 255) >> 8);
                    int px1 = (int)((x - 128 - 1) >> 8);
                    if (px0 < pr.x1) px0 = pr.x1;
                    if (px1 > pr.x2) px1 = pr.x2;
                    if (px0 <= px1) {
                        r = xdraw_filled_rectangle::filled_rectangle(destination,
                                             srect16((int16_t)px0, (int16_t)py,
                                                     (int16_t)px1, (int16_t)py),
                                             color, clip);
                        if (gfx_result::success != r) {
                            return r;
                        }
                    }
                }
            }
        }
        return gfx_result::success;
    }

   public:
    // draws a filled polygon with the specified path and color, with an optional
    // fill rule and clipping rectangle. aliased; no allocation.
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_polygon(Destination& destination, const spath16& path,
                                            PixelType color,
                                            fill_rule rule = fill_rule::even_odd,
                                            const srect16* clip = nullptr) {
        return filled_polygon_impl(destination, path, color, rule, clip);
    }

    // draws an anti-aliased, alpha-blended filled polygon with the specified path
    // and color. rule selects non-zero or even-odd winding. cache: optional draw
    // cache to reuse across calls. clip: optional clipping rectangle.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_polygon(Destination& destination, const spath16& path,
                                               PixelType color,
                                               fill_rule rule = fill_rule::even_odd,
                                               mask_draw_cache* cache = nullptr,
                                               const srect16* clip = nullptr) {
        return aa_filled_polygon_impl(destination, path, color, rule, cache, clip);
    }

     // draws a filled polygon with the specified path and color, with an optional
    // fill rule and clipping rectangle. aliased; no allocation.
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_polygon(Destination& destination, const path16& path,
                                            PixelType color,
                                            fill_rule rule = fill_rule::even_odd,
                                            const srect16* clip = nullptr) {
        return filled_polygon_impl(destination, (spath16)path, color, rule, clip);
    }

    // draws an anti-aliased, alpha-blended filled polygon with the specified path
    // and color. rule selects non-zero or even-odd winding. cache: optional draw
    // cache to reuse across calls. clip: optional clipping rectangle.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_filled_polygon(Destination& destination, const path16& path,
                                               PixelType color,
                                               fill_rule rule = fill_rule::even_odd,
                                               mask_draw_cache* cache = nullptr,
                                               const srect16* clip = nullptr) {
        return aa_filled_polygon_impl(destination, (spath16)path, color, rule, cache, clip);
    }
};
}  // namespace helpers
}  // namespace gfx
#endif