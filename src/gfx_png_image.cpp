#include <string.h>
#include "gfx_png_image.hpp"
#include "pngle.h"
namespace gfx {
typedef struct {
    image_draw_callback callback;
    void* callback_state;
    const rect16* bounds;
    gfx_result error;
} pngle_user_state_t;
static const uint8_t png_image_sig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
static void png_image_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    pngle_user_state_t& st = *(pngle_user_state_t*)pngle_get_user_data(pngle);
    const spoint16 pt((long)x-st.bounds->x1,(long)y-st.bounds->y1);
    const ssize16 sz(w,h);
    const rgba_pixel<32> px(rgba[0],rgba[1],rgba[2],rgba[3]);
    srect16 bounds(pt,sz);
    if(bounds.intersects((srect16)*st.bounds)) {
        bounds = bounds.crop((srect16)*st.bounds);
        image_data data;
        data.is_fill = true;
        rect16 bnds = (rect16)bounds;
        data.fill.bounds = &bnds;
        data.fill.color = px;
        st.callback(data,st.callback_state);
    }
}
static inline gfx_result png_read_uint32(stream* stm,uint32_t* out_result)
{
    uint8_t p[4];
    if(sizeof(p)!=stm->read(p,sizeof(p))) {
        return gfx_result::io_error;
    }
	*out_result = (p[0] << 24)
	     | (p[1] << 16)
	     | (p[2] <<  8)
	     | (p[3] <<  0)
	;
    return gfx_result::success;
}

png_image::png_image() : m_stream(nullptr),m_dimensions(0,0) {

}
png_image::png_image(stream& stream, bool initialize) : m_stream(&stream), m_dimensions(0,0) {
    if(initialize) {
        this->initialize();
    }
}
png_image::~png_image() {
    deinitialize();
}
png_image::png_image(png_image&& rhs) : m_stream(rhs.m_stream),m_dimensions(rhs.m_dimensions) {
    rhs.m_stream = nullptr;
    rhs.m_dimensions = {0,0};
}
png_image& png_image::operator=(png_image&& rhs) {
    this->deinitialize();
    m_stream=rhs.m_stream;
    m_dimensions=rhs.m_dimensions;
    rhs.m_stream = nullptr;
    rhs.m_dimensions = {0,0};
    return *this;
}
gfx_result png_image::initialize() {
    if(initialized()) {
        return gfx_result::success;
    }
    if(m_stream==nullptr) {
        return gfx_result::invalid_state;
    }
    if(!m_stream->caps().read || !m_stream->caps().seek) {
        return gfx_result::invalid_argument;
    }
    m_stream->seek(0);
    uint8_t sig[8];
    if(sizeof(sig)!=m_stream->read(sig,sizeof(sig))) {
        return gfx_result::io_error;
    }
    if(0!=memcmp(sig,png_image_sig,sizeof(sig))) {
        return gfx_result::invalid_format;
    }
    gfx_result res;
    constexpr static const uint32_t ihdr = 0x49484452UL;
    uint32_t len=0;
    uint32_t type=0;
    do {
        m_stream->seek(len,seek_origin::current);
        res = png_read_uint32(m_stream,&len);
        if(res!=gfx_result::success) {
            return res;
        }
        res = png_read_uint32(m_stream,&type);
        if(res!=gfx_result::success) {
            return res;
        }

        
    } while(type!=ihdr);
    if(type!=ihdr) {
        return gfx_result::invalid_format;
    }
    uint32_t tmp;
    res = png_read_uint32(m_stream,&tmp);
    if(res!=gfx_result::success) {
        return res;
    }
    m_dimensions.width = tmp;
    res = png_read_uint32(m_stream,&tmp);
    if(res!=gfx_result::success) {
        return res;
    }
    m_dimensions.height = tmp;
    return gfx_result::success;
}
bool png_image::initialized() const {
    return m_dimensions.width!=0 && m_dimensions.height!=0;
}
void png_image::deinitialize() {
    m_dimensions = {0,0};
}
size16 png_image::dimensions() const {
    return m_dimensions;
}
gfx_result png_image::draw(const rect16& bounds, image_draw_callback callback, void* callback_state) const {
    if(!initialized()) {
        return gfx_result::invalid_state;
    }
    if(m_stream->caps().seek) {
        m_stream->seek(0);
    }
    pngle_t* p = pngle_new();
    uint8_t buf[256];
    if(p==nullptr) {
        return gfx_result::out_of_memory;
    }
    pngle_user_state_t ustate;
    ustate.callback = callback;
    ustate.callback_state=callback_state;
    ustate.bounds = &bounds;
    pngle_set_user_data(p,&ustate);
    pngle_set_draw_callback(p, png_image_on_draw);
    // Feed data to pngle
    int remain = 0;
    int len;
    while ((len = m_stream->read(buf + remain, sizeof(buf) - remain)) > 0) {
        ustate.error = gfx_result::success;
        int fed = pngle_feed(p, buf, remain + len);
        if (fed < 0) {
            gfx_result r= gfx_result::invalid_format;
            if(0==strncmp("Insufficient ",pngle_error(p),13)) {
                r = gfx_result::out_of_memory;
            } else if(0==strncmp("Unsupported ",pngle_error(p),12)) {
                r= gfx_result::not_supported;
            }
            pngle_destroy(p);
            return r;
        } else if(ustate.error!=gfx_result::success) {
            return ustate.error;
        }
        remain = remain + len - fed;
        if (remain > 0) memmove(buf, buf + fed, remain);
    }
    pngle_destroy(p);
    return gfx_result::success;
}
}