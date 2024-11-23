#ifndef HTCW_GFX_IMAGE_HPP
#define HTCW_GFX_IMAGE_HPP
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include "gfx_pixel.hpp"
#include "gfx_bitmap.hpp"
namespace gfx {
struct image_bitmap_data final {
    point16 location;
    const const_bitmap<rgba_pixel<32>>* region;
};
struct image_fill_data final {
    const rect16* bounds;
    rgba_pixel<32> color;
};
struct image_data final {
    bool is_fill;
    image_bitmap_data bitmap;
    image_fill_data fill;
};

typedef gfx_result(*image_draw_callback)(const image_data& data, void* state);
class image {
public:
    virtual gfx_result initialize()=0;
    virtual bool initialized() const=0;
    virtual void deinitialize()=0;
    virtual size16 dimensions() const=0;
    virtual rect16 bounds() const;
    virtual gfx_result draw(const gfx::rect16& bounds, image_draw_callback callback, void* callback_state=nullptr) const=0;
};
}
#endif