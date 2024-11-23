#ifndef HTCW_GFX_JPG_IMAGE_HPP
#define HTCW_GFX_JPG_IMAGE_HPP
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include "gfx_pixel.hpp"
#include "gfx_bitmap.hpp"
#include "gfx_image.hpp"
namespace gfx {
enum struct jpg_scale {
    scale_1_1=0,
    scale_1_2,
    scale_1_4,
    scale_1_8
};
class jpg_image : public image {
    static constexpr const size_t pool_size = 4096;
    stream* m_stream;
    size16 m_native_dimensions;
    jpg_scale m_scale;
    void* m_info;
    void* m_pool;
    void* m_bmp;
    jpg_image(const jpg_image& rhs)=delete;
    jpg_image& operator=(const jpg_image& rhs)=delete;
public:
    jpg_image();
    jpg_image(stream& stream, jpg_scale scale = jpg_scale::scale_1_1, bool initialize=false);
    virtual ~jpg_image();
    jpg_image(jpg_image&& rhs);
    jpg_image& operator=(jpg_image&& rhs);
    jpg_scale scale() const;
    void scale(jpg_scale value);
    size16 native_dimensions() const;
    virtual gfx_result initialize() override;
    virtual bool initialized() const override;
    virtual void deinitialize() override;
    virtual size16 dimensions() const override;
    virtual gfx_result draw(const rect16& bounds, image_draw_callback callback, void* callback_state=nullptr) const override;
};
}
#endif