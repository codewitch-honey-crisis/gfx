#ifndef HTCW_GFX_DRAW_TEXT_HPP
#define HTCW_GFX_DRAW_TEXT_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_icon.hpp"
#include "gfx_font.hpp"
namespace gfx {
namespace helpers {
class xdraw_text {
    template <typename Destination, typename PixelType>
    struct draw_font_cb_state {
        Destination* dst;
        const PixelType* color;
        const srect16* clip;
    };
    template <typename Destination, typename PixelType>
    struct draw_font_helper {
        static gfx_result do_draw(spoint16 location,const const_bitmap<alpha_pixel<8>>& glyph_icon, void* state) {
            using st_t = draw_font_cb_state<Destination,PixelType>;
            st_t st = *(st_t*)state;
            const srect16 sr = srect16(location,(ssize16)glyph_icon.dimensions());
            if(st.clip==nullptr || sr.intersects(*st.clip)) {
                return xdraw_icon::icon(*st.dst,location,glyph_icon,*st.color);
            }
            return gfx_result::success;
        }
    };
    template <typename Destination, typename PixelType>
    static gfx_result text_impl(
        Destination& destination,
        const srect16& dest_rect,
        const text_info& info,
        PixelType color,
        const srect16* clip) {
        using st_t = draw_font_cb_state<Destination,PixelType>;
        st_t st;
        st.dst = &destination;
        st.color = &color;
        st.clip = clip;
        return info.text_font->draw(dest_rect,info,draw_font_helper<Destination,PixelType>::do_draw,&st);
    }
public:  
    // draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result text(
        Destination& destination,
        const srect16& dest_rect,
        const text_info& info,
        PixelType color,
        const srect16* clip = nullptr) {
        return text_impl(destination, dest_rect, info, color, clip);
    }
    // draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
    template <typename Destination, typename PixelType>
    inline static gfx_result text(
        Destination& destination,
        const rect16& dest_rect,
        const text_info& info,
        PixelType color,
        const srect16* clip = nullptr) {
        return text(destination, (srect16)dest_rect, info, color, clip);
    }
    
};
}
}
#endif