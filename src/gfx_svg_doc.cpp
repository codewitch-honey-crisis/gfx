#include <gfx_math.hpp>
#include <gfx_svg.hpp>
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
//#define SVG_DUMP_PARSE
#include <string.h>
#include "svg_private.hpp"

//#include "nanosvgrast.h"


using namespace gfx;


void svg_doc::do_move(svg_doc& rhs) {
    m_doc_data = rhs.m_doc_data;
    m_deallocator = rhs.m_deallocator;
    rhs.m_doc_data = nullptr;
}
void svg_doc::do_free() {
    if (m_doc_data != nullptr) {
        svg_image* img = (svg_image*)m_doc_data;
        svg_delete_image(img,m_deallocator);
        m_doc_data = nullptr;
    }
}
svg_doc::svg_doc(void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) : m_doc_data(nullptr), m_allocator(allocator), m_reallocator(reallocator), m_deallocator(deallocator) {
}
svg_doc::~svg_doc() {
    do_free();
}
svg_doc::svg_doc(svg_doc&& rhs) {
    do_move(rhs);
}
svg_doc& svg_doc::operator=(svg_doc&& rhs) {
    do_free();
    do_move(rhs);
    return *this;
}
svg_doc::svg_doc(stream* svg_stream, uint16_t dpi, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    svg_doc tmp;
    if(gfx_result::success == svg_doc::read(svg_stream,&tmp,dpi,allocator,reallocator,deallocator)) {
        do_move(tmp);
    }
}
gfx_result svg_doc::read(stream* svg_stream, svg_doc* out_doc, uint16_t dpi, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    if (out_doc == nullptr || svg_stream == nullptr || !svg_stream->caps().read) {
        return gfx_result::invalid_argument;
    }
    out_doc->do_free();
    svg_image* img;
     
    gfx_result res = svg_parse_to_image(svg_stream, dpi, &img, allocator, reallocator, deallocator);
    if(res!=gfx_result::success) {
        return res;
    }
    out_doc->m_doc_data = img;
    out_doc->m_allocator = allocator;
    out_doc->m_reallocator = reallocator;
    out_doc->m_deallocator = deallocator;
    return res;
}

bool svg_doc::initialized() const {
    return m_doc_data != nullptr;
}
sizef svg_doc::dimensions() const {
    if(m_doc_data==nullptr) {
        return {0,0};
    }
    svg_image* img = (svg_image*)m_doc_data;
    return {img->dimensions.width, img->dimensions.height};
}
float svg_doc::scale(float line_height) const {
    if(m_doc_data==nullptr) {
        return NAN;
    }
    return line_height/dimensions().height;
}
float svg_doc::scale(sizef dimensions) const {
    if(m_doc_data==nullptr) {
        return NAN;
    }
    float rw = this->dimensions().width/dimensions.width;
    float rh = this->dimensions().height/dimensions.height;
    if(rw>=rh) {
        return 1.0/rw;
    }
    return 1.0/rh;
}
float svg_doc::scale(ssize16 dimensions) const {
    return scale(sizef(dimensions.width,dimensions.height));
}   
float svg_doc::scale(size16 dimensions) const {
    return scale(sizef(dimensions.width,dimensions.height));
}   
void svg_doc::draw(float scale, const srect16& rect, void(read_callback)(int x, int y, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* a, void* state), void* read_callback_state, void(write_callback)(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* state), void* write_callback_state, void*(allocator)(size_t),void*(reallocator)(void*,size_t),void(deallocator)(void*)) const {
    if (initialized()) {
        NSVGrasterizer* rasterizer;
        if (gfx_result::success == svg_create_rasterizer(&rasterizer, allocator, reallocator, deallocator)) {
            svg_rasterize(rasterizer, (svg_image*)m_doc_data, rect.x1, rect.y1, scale, nullptr, read_callback, read_callback_state, write_callback, write_callback_state, rect.width(), rect.height(), rect.width() * 4);
            svg_delete_rasterizer(rasterizer);
        }
    }
}
