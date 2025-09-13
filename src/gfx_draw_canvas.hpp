#ifndef HTCW_GFX_DRAW_CANVAS_HPP
#define HTCW_GFX_DRAW_CANVAS_HPP

#include "gfx_canvas.hpp"
#include "gfx_draw_common.hpp"
#include "gfx_draw_filled_rectangle.hpp"

namespace gfx {
namespace helpers {
template <typename Destination>
struct xdraw_canvas_state {
    Destination* dest;
    rect16 bounds;
};
// CALLBACK MODE WRITE
template <typename Destination>
static gfx_result xdraw_canvas_write_callback(const rect16& bounds,
                                              vector_pixel color, void* state) {
    using st_t = xdraw_canvas_state<Destination>;
    st_t& s = *(st_t*)state;
    typename Destination::pixel_type col;
    convert_palette_from(*s.dest, color, &col);
    if(!bounds.intersects(s.bounds)) {
        return gfx_result::success;
    }
    rect16 b = bounds.crop(s.bounds);
    return s.dest->fill(b, col);
}
// CALLBACK MODE READ
template <typename Destination>
static gfx_result xdraw_canvas_read_callback(point16 location,
                                             vector_pixel* out_color,
                                             void* state) {
    using st_t = xdraw_canvas_state<Destination>;
    st_t& s = *(st_t*)state;
    spoint16 pt = ((spoint16)location);
    if (((srect16)s.bounds).intersects(pt)) {
        typename Destination::pixel_type col;
        s.dest->point((point16)pt, &col);
        gfx_result res = convert_palette_to(*s.dest, col, out_color);
        if (res != gfx_result::success) {
            return res;
        }
    } else {
        *out_color = vector_pixel(0,true);
    }
    return gfx_result::success;
}

template<typename Destination, bool BltSpans> 
struct xdraw_canvas_binder {
    static gfx_result canvas(Destination& destination,
                             ::gfx::canvas& in_canvas,
                             spoint16 location, const srect16* clip) {
        if (!in_canvas.initialized()) {
            return gfx_result::invalid_state;
        }
        srect16 b(location, ssize16(in_canvas.dimensions().width,
                                    in_canvas.dimensions().height));
        if(clip!=nullptr) {
            b=b.crop(*clip);
        }
        if(!b.intersects((srect16)destination.bounds())) {
            in_canvas.callbacks({0,0},{0,0},nullptr,nullptr,nullptr,nullptr);
            in_canvas.direct(nullptr, {0, 0}, {0, 0}, nullptr, nullptr);
            return gfx_result::success;
        }
        in_canvas.direct(nullptr, {0, 0}, {0, 0}, nullptr, nullptr);
        if (!b.intersects((srect16)destination.bounds())) {
            in_canvas.callbacks({0,0},{0,0},nullptr,nullptr,nullptr,nullptr);
            return gfx_result::success;
        }
        using st_t = xdraw_canvas_state<Destination>;
        st_t* st = (st_t*)malloc(sizeof(st_t));
        if (st == nullptr) {
            return gfx_result::out_of_memory;
        }
        size16 dim = (size16)b.dimensions();

        st->dest = &destination;
        st->bounds = (rect16)b;
        in_canvas.callbacks(dim,spoint16(-b.x1,-b.y1),xdraw_canvas_read_callback<Destination>,xdraw_canvas_write_callback<Destination>,st,::free);
        return gfx_result::success;
    }
};
template<typename Destination> 
struct xdraw_canvas_binder<Destination,true> {
    static gfx_result canvas(Destination& destination,
                             canvas& in_canvas,
                             spoint16 location, const srect16* clip) {
       
        using pixel_t = typename Destination::pixel_type;
        if (!in_canvas.initialized()) {
            return gfx_result::invalid_state;
        }
        srect16 b(location, ssize16(in_canvas.dimensions().width,
                                    in_canvas.dimensions().height));
        if(clip!=nullptr) {
            b=b.crop(*clip);
        }
        if(!b.intersects((srect16)destination.bounds())) {
            in_canvas.callbacks({0,0},{0,0},nullptr,nullptr,nullptr,nullptr);
            in_canvas.direct(nullptr, {0, 0}, {0, 0}, nullptr, nullptr);
            return gfx_result::success;
        }
        in_canvas.callbacks({0,0},{0,0},nullptr,nullptr,nullptr,nullptr);
        if (!b.intersects((srect16)destination.bounds())) {
            in_canvas.direct(nullptr, {0, 0}, {0, 0}, nullptr, nullptr);
            return gfx_result::success;
        }
        if(helpers::is_same<pixel_t,rgba_pixel<32>>::value) {
            return in_canvas.direct(&destination,
                                (size16)b.dimensions(), spoint16(-b.x1,-b.y1),
                                helpers::xread_callback_rgba32p, helpers::xwrite_callback_rgba32);
        } else if(helpers::is_same<pixel_t,rgb_pixel<16>>::value) {
            return in_canvas.direct(&destination,
                            (size16)b.dimensions(), spoint16(-b.x1,-b.y1),
                            helpers::xread_callback_rgb16, helpers::xwrite_callback_rgb16);    
        } else if(helpers::is_same<pixel_t,gsc_pixel<8>>::value) {
            return in_canvas.direct(&destination,
                            (size16)b.dimensions(), spoint16(-b.x1,-b.y1),
                            helpers::xread_callback_gsc8, helpers::xwrite_callback_gsc8);    
        } 
        return in_canvas.direct(&destination,
                        (size16)b.dimensions(), spoint16(-b.x1,-b.y1),
                        helpers::xread_callback_any<Destination>, helpers::xwrite_callback_any<Destination>);    
    }
};

class xdraw_canvas {
   public:
    template <typename Destination>
    static gfx_result canvas(Destination& destination,
                             ::gfx::canvas& in_canvas, spoint16 canvas_location = spoint16::zero(),const srect16* clip = nullptr) {
        using binder_t = xdraw_canvas_binder<Destination,Destination::caps::blt_spans>;
        return binder_t::canvas(destination, in_canvas, canvas_location,clip);
    }
    template <typename Destination>
    static gfx_result canvas(Destination& destination,
                             ::gfx::canvas& in_canvas, point16 canvas_location, const srect16* clip = nullptr) {
        return canvas(destination, in_canvas, (spoint16)canvas_location,clip);
    }
};
}  // namespace helpers
}  // namespace gfx
#endif