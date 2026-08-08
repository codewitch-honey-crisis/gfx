#ifndef HTCW_GFX_DRAW_DITHERED
#define HTCW_GFX_DRAW_DITHERED
#include <memory.h>

#include "gfx_dither_cache.hpp"
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_pixel.hpp"
namespace gfx {
namespace helpers {
class xdraw_dithered {
    // The FS core. Walks the destination in scan order; for each pixel: convert
    // source -> destination mapped_pixel_type, apply incoming per-channel error
    // (clamped, via pixel::diffuse), pick nearest palette index, write it, read the
    // chosen color back, compute outgoing error (via pixel::diffuse_error), and
    // spread it 7/16 right, 3/16 down-left, 5/16 down, 1/16 down-right through a
    // two-row int32 carry held in the cache. Size mismatch is a crop (no resample).
    template <typename Destination, typename Source>
    static gfx_result dither_fs(
        Destination& destination, const srect16& dst_rect,
        Source& source, const rect16& source_rect,
        dither_cache* cache, const srect16* clip) {
        static_assert(
            Destination::pixel_type::template has_channel_names<channel_name::index>::value,
            "The destination pixel type must be indexed");
        srect16 clipped = dst_rect;
        if (clip != nullptr) {
            clipped = dst_rect.crop(*clip);
        }
        gfx_result r;
        rect16 dest_rect;
        if (!draw_translate(clipped, &dest_rect)) {
            return gfx_result::success;
        }
        rect16 srcr = source_rect.normalize().crop(source.bounds());
        rect16 dstr = dest_rect.normalize().crop(destination.bounds());

        const int w = (int)math::min_(dstr.width(), srcr.width());
        const int h = (int)math::min_(dstr.height(), srcr.height());
        if (w <= 0 || h <= 0) {
            return gfx_result::success;
        }

        const typename Destination::palette_type* pal = destination.palette();
        if (nullptr == pal) {
            return gfx_result::no_palette;
        }
        using dst_index_px = typename Destination::pixel_type;
        using mapped_px = typename Destination::palette_type::mapped_pixel_type;
        constexpr const size_t NCH = mapped_px::channels;  // includes nop channels

        // --- error carry: two rows of (w * NCH) int32, current + next -----------
        dither_cache local;
        dither_cache* dc = (nullptr != cache) ? cache : &local;
        dither_cache::error_element* err_curr;
        dither_cache::error_element* err_next;
        if (!dc->error_rows(w, NCH, &err_curr, &err_next)) {
            return gfx_result::out_of_memory;
        }

        for (int y = 0; y < h; ++y) {
            memset(err_next, 0, (size_t)w * NCH * sizeof(dither_cache::error_element));

            for (int x = 0; x < w; ++x) {
                // 1. source pixel -> destination mapped color
                typename Source::pixel_type spx;
                r = source.point(point16(srcr.x1 + x, srcr.y1 + y), &spx);
                if (gfx_result::success != r) return r;
                mapped_px want;
                r = convert(spx, &want);  // INFER: gfx::convert(src, &mapped)
                if (gfx_result::success != r) return r;

                // 2. apply incoming error (clamped) -> corrected color
                mapped_px corrected;
                r = want.diffuse(&err_curr[x * NCH], &corrected);
                if (gfx_result::success != r) return r;

                // 3. nearest palette index for the corrected color
                dst_index_px chosen;
                r = pal->nearest(corrected, &chosen);
                if (gfx_result::success != r) return r;

                // 4. write the index
                r = destination.point(point16(dstr.x1 + x, dstr.y1 + y), chosen);
                if (gfx_result::success != r) return r;

                // 5. actual color of the chosen entry
                mapped_px got;
                r = pal->map(chosen, &got);
                if (gfx_result::success != r) return r;

                // 6. outgoing error = corrected - got, per channel
                int32_t e[mapped_px::channels];
                r = corrected.diffuse_error(got, e);
                if (gfx_result::success != r) return r;

                // 7. diffuse (integer /16). Loop over channels generically.
                for (size_t c = 0; c < NCH; ++c) {
                    const int32_t ev = e[c];
                    if (x + 1 < w) {
                        err_curr[(x + 1) * NCH + c] += ev * 7 / 16;  // right
                        err_next[(x + 1) * NCH + c] += ev * 1 / 16;  // down-right
                    }
                    if (x - 1 >= 0) {
                        err_next[(x - 1) * NCH + c] += ev * 3 / 16;  // down-left
                    }
                    err_next[x * NCH + c] += ev * 5 / 16;  // down
                }
            }
            dither_cache::error_element* t = err_curr;
            err_curr = err_next;
            err_next = t;
        }
        return gfx_result::success;
    }

   public:
    // Creates a Floyd-Steinberg dithered bitmap by dithering `source`
    template <typename Destination, typename Source>
    inline static gfx_result dithered(Destination destination, const srect16& bounds,
                                      const Source& source, const rect16 source_bounds = {0, 0, 32767, 32767}, dither_cache* cache = nullptr, const srect16* clip = nullptr) {
        static_assert(
            Destination::pixel_type::template has_channel_names<channel_name::index>::value,
            "PixelType must be indexed");
        return dither_fs(
            destination, bounds, source, source_bounds, cache, clip);
    }
    // Creates a Floyd-Steinberg dithered bitmap by dithering `source`
    template <typename Destination, typename Source>
    inline static gfx_result dithered(Destination destination, const rect16& bounds,
                                      const Source& source, const rect16 source_bounds = {0, 0, 32767, 32767}, dither_cache* cache = nullptr, const srect16* clip = nullptr) {
        static_assert(
            Destination::pixel_type::template has_channel_names<channel_name::index>::value,
            "PixelType must be indexed");
        return dither_fs(
            destination, (srect16)bounds, source, source_bounds, cache, clip);
    }
};
}  // namespace helpers
}  // namespace gfx
#endif