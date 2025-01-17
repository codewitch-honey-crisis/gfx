#ifndef HTCW_GFX_DRAW_POLYGON
#define HTCW_GFX_DRAW_POLYGON
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
namespace gfx {
namespace helpers {
class xdraw_polygon {
    template <typename Destination, typename PixelType>
    static gfx_result polygon_impl(Destination& destination, const spath16& path, PixelType color, const srect16* clip) {
        gfx_result r;
        if (path.size() == 0) {
            return gfx::gfx_result::success;
        }
        const spoint16* pb = path.begin();
        const spoint16* pe = path.end() - 1;
        const spoint16* p = pb;
        size_t path_size = path.size();
        while (p != pe) {
            const spoint16 p1 = *p;
            ++p;
            const spoint16 p2 = *p;
            const srect16 sr(p1, p2);
            r = xdraw_line::line(destination, sr, color, clip);
            if (gfx_result::success != r) {
                return r;
            }
        }
        return xdraw_line::line(destination, srect16((*pe), (*pb)), color, clip);
    }
public:
    // draws a polygon with the specified path and color, with an optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result polygon(Destination& destination, const spath16& path, PixelType color, const srect16* clip = nullptr) {
        return polygon_impl(destination, path, color, clip);
    }
};
}
}
#endif
