#ifndef HTCW_GFX_SPRITE_HPP
#define HTCW_GFX_SPRITE_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_line.hpp"
#include "gfx_sprite.hpp"
namespace gfx {
namespace helpers {
class xdraw_sprite {
    template <typename Destination, typename Sprite>
    static gfx_result sprite_impl(Destination& destination, const spoint16& location, const Sprite& source, const srect16* clip = nullptr) {
        static_assert(helpers::is_same<typename Destination::pixel_type, typename Sprite::pixel_type>::value, "Sprite and Destination pixel types must be the same");
        const size16 size = source.dimensions();
        const srect16 db;
        for (int y = 0; y < size.height; ++y) {
            int run_start=-1;
            typename Sprite::pixel_type px(0,true), opx(0,true);
            int x = 0;
            while(x<size.width) {
                run_start = -1;
                while(x<size.width) {
                    while(!source.hit_test(point16(x,y)) && x<size.width) {
                        ++x;
                    }
                    if(x<size.width) {
                        run_start = x;                        
                        source.point(point16(x,y),&px);
                        opx=px;
                        while(x<size.width && source.hit_test(point16(x,y)) && opx==px) {
                            opx=px;
                            ++x;
                            source.point(point16(x,y),&px);
                        }
                        xdraw_line::line(destination,srect16(run_start+location.x,y+location.y,x-1+location.x,y+location.y),opx,clip);
                    }
                }
            }
        }

        return gfx_result::success;
    }
public:
    // draws a sprite to the destination at the specified location with an optional clipping rectangle
    template <typename Destination, typename Sprite>
    static inline gfx_result sprite(Destination& destination, spoint16 location, const Sprite& sprite, const srect16* clip = nullptr) {
        return sprite_impl(destination, location, sprite, clip);
    }
    // draws a sprite to the destination at the location with an optional clipping rectangle
    template <typename Destination, typename Sprite>
    static inline gfx_result sprite(Destination& destination, point16 location, const Sprite& the_sprite, const srect16* clip = nullptr) {
        return sprite(destination, (spoint16)location, the_sprite, clip);
    }
};
}
}
#endif