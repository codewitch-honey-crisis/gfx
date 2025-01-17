#ifndef HTCW_GFX_DRAW_FILLED_POLYGON
#define HTCW_GFX_DRAW_FILLED_POLYGON
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
namespace gfx {
namespace helpers {
class xdraw_filled_polygon {
    template <typename Destination, typename PixelType>
    static gfx_result filled_polygon_impl(Destination& destination, const spath16& path, PixelType color, const srect16* clip) {
        gfx_result r;
        if (0 == path.size()) {
            return gfx_result::success;
        }
        srect16 pr = path.bounds();
        if (nullptr != clip) {
            pr = pr.crop(*clip);
        }
        for (int y = pr.y1; y <= pr.y2; ++y) {
            int run_start = -1;
            for (int x = pr.x1; x <= pr.x2; ++x) {
                const spoint16 pt(x, y);
                if (path.intersects(pt, true)) {
                    if (-1 == run_start) {
                        run_start = x;
                    }
                } else {
                    if (-1 != run_start) {
                        r = xdraw_line::line(destination, srect16(run_start, y, x - 1, y), color, clip);
                        if (gfx_result::success != r) {
                            return r;
                        }
                        run_start = -1;
                    }
                }
            }
            if (-1 != run_start) {
                r = xdraw_line::line(destination, srect16(run_start, y, pr.x2, y), color, clip);
                if (gfx_result::success != r) {
                    return r;
                }
            }
        }
        return gfx_result::success;
    }
public:
    // draws a polygon with the specified path and color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result filled_polygon(Destination& destination, const spath16& path, PixelType color, const srect16* clip = nullptr) {
        return filled_polygon_impl(destination, path, color, clip);
    }
};
}
}
#endif
