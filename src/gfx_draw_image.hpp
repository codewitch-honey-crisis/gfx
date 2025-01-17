#ifndef HTCW_GFX_DRAW_IMAGE_HPP
#define HTCW_GFX_DRAW_IMAGE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_bitmap.hpp"
#include "gfx_image.hpp"
#include "gfx_draw_filled_rectangle.hpp"
#include "gfx_draw_bitmap.hpp"
namespace gfx {
namespace helpers {
class xdraw_image {
    template<typename Destination>
    struct image_cb_state {
        Destination* dst;
        const srect16* dst_rect;
        const rect16* src_rect;
        const srect16* clip;
    };
    
    template <typename Destination>
    static gfx_result image_impl(Destination& destination, const srect16& destination_rect, const ::gfx::image& source_image, const rect16& source_rect, const srect16* clip) {
        if(!source_image.initialized()) {
            return gfx_result::invalid_state;
        }
        rect16 src_rect = source_rect.crop(source_image.bounds());
        image_cb_state<Destination> st;
        st.dst = &destination;
        st.dst_rect = &destination_rect;
        st.src_rect = &src_rect;
        st.clip = clip;
        gfx_result r=source_image.draw(src_rect,
        [](const image_data& data, void* state) {
            image_cb_state<Destination>& s=*(image_cb_state<Destination>*)state;
            if(data.is_fill) {
                srect16 sr = *s.dst_rect;
                if(s.clip!=nullptr) {
                    if(!sr.intersects(*s.clip)) {
                        return gfx_result::success;
                    }
                    sr=sr.crop(*s.clip);
                }
                const srect16 b = (srect16)data.fill.bounds->offset(s.dst_rect->point1());
                if(sr.intersects(b)) {
                    return xdraw_filled_rectangle::filled_rectangle(*s.dst,b,data.fill.color,&sr);
                }
                return gfx_result::success;
            } else {
                srect16 sr = *s.dst_rect;
                if(s.clip!=nullptr) {
                    if(!sr.intersects(*s.clip)) {
                        return gfx_result::success;
                    }
                    sr=sr.crop(*s.clip);
                }
                return xdraw_bitmap::bitmap(*s.dst,((srect16)data.bitmap.region->bounds()).offset(data.bitmap.location.x,data.bitmap.location.y).offset(s.dst_rect->point1()), *data.bitmap.region,data.bitmap.region->bounds(), bitmap_resize::crop,nullptr,&sr);
            }
        }
        ,&st);
        return r;
    }
public:
    // draws an image from the specified stream to the specified destination rectangle with the an optional clipping rectangle
    template <typename Destination>
    static inline gfx_result image(Destination& destination, const srect16& destination_rect, const ::gfx::image& source_image, const rect16& source_rect = rect16(0, 0, 65535, 65535),const srect16* clip = nullptr) {
        return image_impl(destination, destination_rect, source_image, source_rect, clip);
    }
    // draws an image from the specified stream to the specified destination rectangle with the an optional clipping rectangle
    template <typename Destination>
    static inline gfx_result image(Destination& destination, const rect16& destination_rect, const ::gfx::image& source_image, const rect16& source_rect = rect16(0, 0, 65535, 65535), const srect16* clip = nullptr) {
        return image(destination,(srect16)destination_rect, source_image, source_rect, clip);
    }
};
}
}
#endif