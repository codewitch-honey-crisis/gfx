#ifndef HTCW_GFX_DRAW_AA_POLYGON_HPP
#define HTCW_GFX_DRAW_AA_POLYGON_HPP
#include "gfx_draw_common.hpp"
#include "gfx_mask_draw_cache.hpp"
#include "gfx_math.hpp"
namespace gfx {
namespace helpers {

// Anti-aliased, variable-width CLOSED polygon outline (stroke only, no fill).
//
// This is xdraw_aa_polyline with the two loose ends joined rather than capped.
// The stroke is composited in a single pass per scanline as a DISTANCE FIELD:
// each primitive reports a signed distance (dist16 = hw + SDF, 16.16 form), the
// per-pixel MINIMUM is kept, and that combined field is converted to coverage
// once at the end. The union of shapes has distance field min(members), so the
// anti-aliased boundary is computed on the true outer edge of the whole stroke --
// overlapping pieces neither double-darken nor leave half-lit creases.
//
// CLOSURE
//   The polygon is always closed. If the caller's last point already equals the
//   first it is dropped, so no degenerate zero-length segment is generated; the
//   closing edge is then implicit (last -> first). Because the outline has no
//   loose ends there is NO line_cap parameter: every vertex, including vertex 0,
//   gets a join. This is the whole difference from the polyline -- appending
//   path[0] to a polyline instead would butt-cap both ends at the start vertex
//   and leave a notch there.
//
// JOINS (identical semantics to xdraw_aa_polyline)
//   round  : the segment ends meeting at a vertex are round-capped; the
//            min-distance of the two capsules is exactly the round join.
//   bevel  : segment ends are butt-capped; a convex fill covers the outer notch.
//   miter  : as bevel, but the outer edges run out to their apex (quad). If the
//            apex is farther than miter_limit*(width/2) it falls back to bevel
//            (SVG semantics: miterLength/strokeWidth <= miter_limit).
//
// The bevel/miter fill is extended inward along both segment centerlines by hw so
// its edges land inside the solid interior of the segments; combined with the
// min-distance compositing this removes the butt-face crease at the joint. The
// outer corner (spike/chord) is unaffected.
//
// Coverage/SDF math is 32-bit (same envelope as the line routine). Join geometry
// uses a wide word when available; miter requires it and otherwise degrades to
// bevel. Bevel/round stay 32-bit by working in unit-direction, vertex-local
// coordinates.
class xdraw_aa_polygon {
#if defined(HTCW_MAX_WORD) && (HTCW_MAX_WORD >= 64)
    static constexpr bool has_wide = true;
    typedef int64_t jint;
#else
    static constexpr bool has_wide = false;
    typedef int32_t jint;
#endif

    // If bevel/miter joints render on the *inner* side of a bend, flip to -1.
    // Kept identical to xdraw_aa_polyline's k_join_side; change both together.
    static constexpr int k_join_side = 1;

    // sentinel "very far" distance: any pixel left at this reads as zero coverage.
    static constexpr int32_t k_far = 1 << 28;

    struct poly_edge {
        int32_t nx_q8, ny_q8; // outward unit normal, Q8 (component * 256)
        int32_t vx, vy;       // an endpoint of the edge, vertex-local Q8
    };

    static jint jsqrt(jint v) {
        if (v <= 0) return 0;
        jint x = v, y = (x + 1) / 2;
        while (y < x) { x = y; y = (x + v / x) / 2; }
        return x;
    }

    // signed distance of one segment A->B at (px,py), as dist16 (= hw + SDF, 16.16).
    // capA applies at the A end, capB at the B end, body between. k_far if the
    // pixel is beyond this primitive's reach. In a closed polygon both caps are
    // always the join end cap; the parameters are kept so the body of this
    // routine stays line-for-line comparable with xdraw_aa_polyline.
    static int32_t seg_dist16(int32_t px, int32_t py,
                              int32_t ax, int32_t ay, int32_t bx, int32_t by,
                              int32_t dx, int32_t dy, int32_t len2,
                              int32_t len_q8, int32_t len_int, int32_t thresh,
                              line_cap capA, line_cap capB,
                              int32_t hw16, int32_t hw_q8, int32_t cap_cut2) {
        const int32_t fx = px - ax;
        const int32_t fy = py - ay;
        const int32_t along = fx * dx + fy * dy;
        if (along > 0 && along < len2) {
            const int32_t cross = fx * dy - fy * dx;
            const int32_t acr = cross < 0 ? -cross : cross;
            if (acr > thresh) return k_far;
            int32_t dist_q8;
            if (acr < (1 << 15)) dist_q8 = (int32_t)(((uint32_t)acr << 16) / (uint32_t)len_q8);
            else                 dist_q8 = (acr << 8) / len_int;
            return dist_q8 << 8;
        }
        const line_cap cap = (along <= 0) ? capA : capB;
        if (cap == line_cap::round) {
            int32_t g2;
            if (along <= 0) { g2 = fx * fx + fy * fy; }
            else { const int32_t gx = px - bx, gy = py - by; g2 = gx * gx + gy * gy; }
            if (g2 > cap_cut2) return k_far;
            return (int32_t)math::sqrt_ft32<8>((uint32_t)g2) << 8;
        }
        const int32_t over  = (along <= 0) ? -along : (along - len2);
        const int32_t cross = fx * dy - fy * dx;
        const int32_t acr   = cross < 0 ? -cross : cross;
        if (acr > thresh) return k_far;
        int32_t perp_q8;
        if (acr < (1 << 15)) perp_q8 = (int32_t)(((uint32_t)acr << 16) / (uint32_t)len_q8);
        else                 perp_q8 = (acr << 8) / len_int;
        const int32_t over_q8 = (int32_t)(((uint32_t)over << 8) / (uint32_t)len_int);
        int32_t sdf_q8;
        if (cap == line_cap::square) sdf_q8 = (over_q8 > perp_q8 ? over_q8 : perp_q8) - hw_q8;
        else { const int32_t pp = perp_q8 - hw_q8; sdf_q8 = over_q8 > pp ? over_q8 : pp; }
        return hw16 + sdf_q8 * 256;
    }

    // Build the convex-ish join fill at vertex V (prev P, next N) in V-local Q8
    // coordinates. Vertices run: inner anchor in the incoming segment, the
    // incoming outer corner, [miter apex], the outgoing outer corner, inner
    // anchor in the outgoing segment. The two inner anchors are pushed hw along
    // the centerlines so the fill overlaps solid interior. Returns edge count, or
    // 0 to skip (collinear/degenerate).
    static int build_join(int32_t Vx, int32_t Vy, int32_t Px, int32_t Py,
                          int32_t Nx, int32_t Ny, int32_t hw_q8, int mlim,
                          bool use_miter, poly_edge* edges) {
        const int32_t dinx = Vx - Px, diny = Vy - Py;
        const int32_t doutx = Nx - Vx, douty = Ny - Vy;
        const int32_t lin2  = dinx * dinx + diny * diny;
        const int32_t lout2 = doutx * doutx + douty * douty;
        if (lin2 == 0 || lout2 == 0) return 0;
        int32_t linI  = (int32_t)((math::sqrt_ft32<8>((uint32_t)lin2)  + 128) >> 8);
        int32_t loutI = (int32_t)((math::sqrt_ft32<8>((uint32_t)lout2) + 128) >> 8);
        if (linI < 1)  linI = 1;
        if (loutI < 1) loutI = 1;
        const int32_t cin  = (dinx  * 256) / linI,  sin_ = (diny  * 256) / linI;
        const int32_t cout = (doutx * 256) / loutI, sout = (douty * 256) / loutI;
        int32_t turn = cin * sout - sin_ * cout; // ~ sin(angle) * 65536
        if (turn == 0) return 0;
        int s = (turn > 0) ? -1 : 1;
        s *= k_join_side;
        // outer offset corners at V (V-local Q8); left-normal of unit dir = (-sin,cos)
        const int32_t inx  = -s * ((hw_q8 * sin_) / 256), iny  = s * ((hw_q8 * cin) / 256);
        const int32_t outx = -s * ((hw_q8 * sout) / 256), outy = s * ((hw_q8 * cout) / 256);
        // inner anchors pushed hw into the solid interior of each segment
        const int32_t pinx = -((hw_q8 * cin) / 256),  piny = -((hw_q8 * sin_) / 256);
        const int32_t poutx = ((hw_q8 * cout) / 256), pouty = ((hw_q8 * sout) / 256);

        int32_t vx[6], vy[6];
        int n = 0;
        vx[n] = pinx; vy[n] = piny; ++n;   // inside incoming
        vx[n] = inx;  vy[n] = iny;  ++n;    // incoming outer corner
        if (use_miter) {
            const jint denom = (jint)cin * sout - (jint)sin_ * cout;
            if (denom != 0) {
                const int32_t Dx = outx - inx, Dy = outy - iny;
                const jint tnum = (jint)Dx * sout - (jint)Dy * cout;
                const jint tq8  = (jint)256 * tnum / denom;
                const jint apx  = (jint)inx + (tq8 * cin)  / 256;
                const jint apy  = (jint)iny + (tq8 * sin_) / 256;
                const jint ad2  = apx * apx + apy * apy;
                const jint lim  = (jint)mlim * hw_q8;
                if (ad2 <= lim * lim) { vx[n] = (int32_t)apx; vy[n] = (int32_t)apy; ++n; }
            }
        }
        vx[n] = outx;  vy[n] = outy;  ++n;  // outgoing outer corner
        vx[n] = poutx; vy[n] = pouty; ++n;  // inside outgoing

        int32_t cx = 0, cy = 0;
        for (int i = 0; i < n; ++i) { cx += vx[i]; cy += vy[i]; }
        cx /= n; cy /= n;
        for (int j = 0; j < n; ++j) {
            const int32_t x0 = vx[j], y0 = vy[j];
            const int32_t x1 = vx[(j + 1) % n], y1 = vy[(j + 1) % n];
            const int32_t ex = x1 - x0, ey = y1 - y0;
            int32_t nrx = -ey, nry = ex;
            const jint dc = (jint)nrx * (cx - x0) + (jint)nry * (cy - y0);
            if (dc > 0) { nrx = ey; nry = -ex; } // orient outward (centroid inside)
            jint elen = jsqrt((jint)ex * ex + (jint)ey * ey);
            if (elen < 1) elen = 1;
            edges[j].nx_q8 = (int32_t)(((jint)nrx * 256) / elen);
            edges[j].ny_q8 = (int32_t)(((jint)nry * 256) / elen);
            edges[j].vx = x0; edges[j].vy = y0;
        }
        return n;
    }

    // signed distance (dist16) of the convex fill at a vertex-local pixel.
    static int32_t poly_dist16(int32_t plx_q8, int32_t ply_q8, const poly_edge* edges,
                               int ne, int32_t hw16) {
        jint maxd = -((jint)1 << 30);
        for (int j = 0; j < ne; ++j) {
            const jint d = ((jint)edges[j].nx_q8 * (plx_q8 - edges[j].vx) +
                            (jint)edges[j].ny_q8 * (ply_q8 - edges[j].vy)) / 256;
            if (d > maxd) maxd = d;
        }
        return hw16 + (int32_t)maxd * 256;
    }

    template <typename Destination, typename PixelType>
    static gfx_result aa_polygon_impl(Destination& destination, const spath16& path,
                                      PixelType color, int16_t width,
                                      line_join join, int16_t miter_limit,
                                      mask_draw_cache* cache, const srect16* clip) {
        if (width < 0) return gfx_result::invalid_argument;
        if (width == 0) return gfx_result::success;
        size_t np = path.size();
        if (np == 0) return gfx_result::success;

        // drop an explicit closing point so the closing edge is implicit and no
        // zero-length segment is produced. np is the vertex count from here on.
        if (np > 1 && path.begin()[np - 1] == path.begin()[0]) --np;
        if (np < 3) return gfx_result::success; // needs a triangle to be a polygon

        const uint8_t opacity = color.opacity8();

        ssize16 ss;
        draw_translate(destination.dimensions(), &ss);
        srect16 bounds(spoint16(0, 0), ss);
        srect16 c = (nullptr != clip) ? clip->crop(bounds) : bounds;

        const int32_t half16 = 1 << 15;
        const int32_t hw16   = (int32_t)width << 15;
        const int32_t hw_q8  = (int32_t)width << 7;
        const int32_t band16 = hw16 + half16;
        const int32_t band_px  = (band16 + 0xFFFF) >> 16;
        const int32_t cap_cut2 = band_px * band_px;

        int mlim = miter_limit < 1 ? 1 : (int)miter_limit;
        const bool round_join = (join == line_join::round);
        const bool poly_join  = !round_join;
        const bool use_miter  = (join == line_join::miter) && has_wide;
        // every segment end meets a join -- there are no free ends to cap.
        const line_cap join_end_cap = round_join ? line_cap::round : line_cap::butt;

        // no square-cap term in the pad: a closed outline has no caps at all.
        int pad = (int)(width >> 1) + 1;
        if (use_miter) {
            int pm = (int)(width >> 1) * mlim + 2;
            if (pm > pad) pad = pm;
        }

        srect16 pb = path.bounds();
        int minx = pb.left()  - pad, maxx = pb.right()  + pad;
        int miny = pb.top()   - pad, maxy = pb.bottom() + pad;
        if (minx < c.x1) minx = c.x1;
        if (miny < c.y1) miny = c.y1;
        if (maxx > c.x2) maxx = c.x2;
        if (maxy > c.y2) maxy = c.y2;
        if (minx > maxx || miny > maxy) return gfx_result::success;

        mask_draw_cache local;
        mask_draw_cache* dc = (nullptr != cache) ? cache : &local;
        const int row_w = maxx - minx + 1;
        // per-scanline distance accumulator (min over primitives). int32 per pixel.
        int32_t* dist = reinterpret_cast<int32_t*>(dc->ensure((size_t)row_w * sizeof(int32_t)));
        if (nullptr == dist) return gfx_result::out_of_memory;

        typename Destination::pixel_type fgpx;
        rgba_pixel<32> cfgpx;
        convert_palette_from(destination, color, &fgpx, nullptr);
        convert_palette_to<Destination, rgba_pixel<32>>(destination, fgpx, &cfgpx);
        cfgpx.template channel<channel_name::A>(255);

        // a closed polygon has exactly as many segments as vertices: the last one
        // wraps from path[np-1] back to path[0].
        const size_t nseg = np;

        for (int py = miny; py <= maxy; ++py) {
            for (int i = 0; i < row_w; ++i) dist[i] = k_far;
            int rlo = row_w;   // lowest covered index this scanline (inclusive)
            int rhi = -1;      // highest covered index (rhi < rlo => empty row)

            // segments
            for (size_t sidx = 0; sidx < nseg; ++sidx) {
                const spoint16 pa = path[sidx];
                const spoint16 pbt = path[(sidx + 1) % np];   // wraps to close
                const int32_t ax = pa.x, ay = pa.y, bx = pbt.x, by = pbt.y;
                const int32_t dx = bx - ax, dy = by - ay;
                const int32_t len2 = dx * dx + dy * dy;
                if (len2 == 0) continue;

                const int32_t symin = (ay < by ? ay : by) - pad;
                const int32_t symax = (ay < by ? by : ay) + pad;
                if (py < symin || py > symax) continue;

                int32_t len_q8  = (int32_t)math::sqrt_ft32<8>((uint32_t)len2);
                int32_t len_int = (len_q8 + 128) >> 8;
                if (len_int < 1) len_int = 1;
                const int32_t thresh = band_px * (len_int + 1);

                // both ends of every segment meet a join, including segment 0's
                // start and segment nseg-1's end.
                const line_cap capA = join_end_cap;
                const line_cap capB = join_end_cap;

                int xs = (ax < bx ? ax : bx) - pad;
                int xe = (ax < bx ? bx : ax) + pad;
                if (xs < minx) xs = minx;
                if (xe > maxx) xe = maxx;
                for (int px = xs; px <= xe; ++px) {
                    const int32_t d16 = seg_dist16(px, py, ax, ay, bx, by, dx, dy, len2,
                                                   len_q8, len_int, thresh, capA, capB,
                                                   hw16, hw_q8, cap_cut2);
                    const int idx = px - minx;
                    int32_t& acc = dist[idx];
                    if (d16 < acc) acc = d16;
                    if (d16 < band16) { if (idx < rlo) rlo = idx; if (idx > rhi) rhi = idx; }
                }
            }

            // bevel / miter join fills -- at EVERY vertex, k == 0 included, whose
            // previous vertex wraps around to path[np-1].
            if (poly_join) {
                for (size_t k = 0; k < np; ++k) {
                    const spoint16 pv = path[k];
                    const spoint16 pp = path[(k + np - 1) % np];
                    const spoint16 pn = path[(k + 1) % np];
                    const int32_t Vx = pv.x, Vy = pv.y;
                    if (py < Vy - pad || py > Vy + pad) continue;
                    poly_edge edges[6];
                    const int ne = build_join(Vx, Vy, pp.x, pp.y, pn.x, pn.y,
                                              hw_q8, mlim, use_miter, edges);
                    if (ne == 0) continue;
                    int xs = Vx - pad, xe = Vx + pad;
                    if (xs < minx) xs = minx;
                    if (xe > maxx) xe = maxx;
                    const int32_t ply_q8 = (py - Vy) << 8;
                    for (int px = xs; px <= xe; ++px) {
                        const int32_t plx_q8 = (px - Vx) << 8;
                        const int32_t d16 = poly_dist16(plx_q8, ply_q8, edges, ne, hw16);
                        const int idx = px - minx;
                        int32_t& acc = dist[idx];
                        if (d16 < acc) acc = d16;
                        if (d16 < band16) { if (idx < rlo) rlo = idx; if (idx > rhi) rhi = idx; }
                    }
                }
            }

            // dist[] holds signed distance (16.16); convert the covered span to
            // coverage bytes in place, then blit only that span. In-place is safe
            // L->R: cov byte at offset i only lands on a dist entry already read.
            if (rhi >= rlo) {
                uint8_t* cov = reinterpret_cast<uint8_t*>(dist);
                for (int i = rlo; i <= rhi; ++i) {
                    const int32_t cov16 = band16 - dist[i];   // 0.5 - SDF, in 16.16
                    uint8_t c8;
                    if (cov16 <= 0)              c8 = 0;      // outside / gaps left at k_far
                    else if (cov16 >= (1 << 16)) c8 = 255;   // solid interior
                    else                         c8 = (uint8_t)(cov16 >> 8);
                    cov[i] = (uint8_t)(c8 * opacity / 255);   // fold opacity
                }
                aa_rasterize_row(destination, {(int16_t)(minx + rlo), (int16_t)py},
                                 cov + rlo, (size_t)(rhi - rlo + 1), fgpx);
            }
        }
        return gfx_result::success;
    }

public:
    // draws an anti-aliased closed polygon outline of the given width (>= 0),
    // color and join. The path is closed automatically; an explicit duplicate of
    // the first point as the last point is accepted and ignored. There is no cap
    // parameter -- a closed outline has no free ends.
    // miter_limit: SVG ratio (miterLength/strokeWidth); past it, miter -> bevel.
    // cache: optional draw cache to reuse across calls. clip: optional clip rectangle.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_polygon(Destination& destination, const path16& path,
                                        PixelType color, int16_t width,
                                        line_join join = line_join::miter,
                                        int16_t miter_limit = 4,
                                        mask_draw_cache* cache = nullptr,
                                        const srect16* clip = nullptr) {
        return aa_polygon_impl(destination, (spath16)path, color, width, join, miter_limit, cache, clip);
    }
    // draws an anti-aliased closed polygon outline of the given width (>= 0),
    // color and join. The path is closed automatically; an explicit duplicate of
    // the first point as the last point is accepted and ignored. There is no cap
    // parameter -- a closed outline has no free ends.
    // miter_limit: SVG ratio (miterLength/strokeWidth); past it, miter -> bevel.
    // cache: optional draw cache to reuse across calls. clip: optional clip rectangle.
    template <typename Destination, typename PixelType>
    inline static gfx_result aa_polygon(Destination& destination, const spath16& path,
                                        PixelType color, int16_t width,
                                        line_join join = line_join::miter,
                                        int16_t miter_limit = 4,
                                        mask_draw_cache* cache = nullptr,
                                        const srect16* clip = nullptr) {
        return aa_polygon_impl(destination, path, color, width, join, miter_limit, cache, clip);
    }
};
}
}
#endif