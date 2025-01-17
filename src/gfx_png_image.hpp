#ifndef HTCW_GFX_PNG_IMAGE_HPP
#define HTCW_GFX_PNG_IMAGE_HPP
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include "gfx_pixel.hpp"
#include "gfx_bitmap.hpp"
#include "gfx_image.hpp"
namespace gfx {
class png_image : public image {
    stream* m_stream;
    size16 m_dimensions;
    png_image(const png_image& rhs)=delete;
    png_image& operator=(const png_image& rhs)=delete;
public:
    png_image();
    png_image(stream& stream, bool initialize=false);
    virtual ~png_image();
    png_image(png_image&& rhs);
    png_image& operator=(png_image&& rhs);
    virtual gfx_result initialize() override;
    virtual bool initialized() const override;
    virtual void deinitialize() override;
    virtual size16 dimensions() const override;
    virtual gfx_result draw(const rect16& bounds, image_draw_callback callback, void* callback_state=nullptr) const override;
};
}
#endif