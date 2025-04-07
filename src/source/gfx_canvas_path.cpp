#include "gfx_canvas.hpp"
#include "plutovg.h"
#define PHND ((plutovg_path_t*)m_info)

namespace gfx {
canvas_path::canvas_path(void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*)) : m_info(nullptr),m_allocator(allocator),m_reallocator(reallocator),m_deallocator(deallocator) {

}
canvas_path::~canvas_path() {
    deinitialize();
}
canvas_path::canvas_path(canvas_path&& rhs) {
    m_info = rhs.m_info;
    rhs.m_info = nullptr;
}
canvas_path& canvas_path::operator=(canvas_path&& rhs) {
    deinitialize();
    m_info = rhs.m_info;
    rhs.m_info = nullptr;
    return *this;
}
gfx_result canvas_path::initialize() {
    if(m_info==nullptr) {
        plutovg_path* p = plutovg_path_create(m_allocator,m_reallocator,m_deallocator);
        if(p==nullptr) {
            return gfx_result::out_of_memory;
        }
        m_info = p;
    }
    return gfx_result::success;
}
bool canvas_path::initialized() const {
    return m_info!=nullptr;
}
void canvas_path::deinitialize() {
    if(m_info!=nullptr) {
        plutovg_path_destroy(PHND,m_deallocator);
        m_info = nullptr;
    }
}
gfx_result canvas_path::move_to(pointf location) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(plutovg_path_move_to(PHND,location.x,location.y)) {
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}
gfx_result canvas_path::line_to(pointf location) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(plutovg_path_line_to(PHND,location.x,location.y)) {
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}
gfx_result canvas_path::quad_to(pointf point1,pointf point2) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(plutovg_path_quad_to(PHND,point1.x,point1.y,point2.x,point2.y)) {
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}
gfx_result canvas_path::cubic_to(pointf point1,pointf point2, pointf point3) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(plutovg_path_cubic_to(PHND,point1.x,point1.y,point2.x,point2.y,point3.x,point3.y)) {
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}
gfx_result canvas_path::arc_to(sizef radiuses,float angle, bool large_arc, bool sweep, pointf location) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_arc_to(PHND,radiuses.width,radiuses.height,angle,large_arc,sweep,location.x,location.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::rectangle(const rectf& bounds) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_rect(PHND,bounds.x1,bounds.y1,bounds.width(),bounds.height())) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::rounded_rectangle(const rectf& bounds, sizef radiuses) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_round_rect(PHND,bounds.x1,bounds.y1,bounds.width(),bounds.height(),radiuses.width,radiuses.height)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::ellipse(pointf center, sizef radiuses) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_ellipse(PHND,center.x,center.y,radiuses.width,radiuses.height)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::circle(pointf center, float radius) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_circle(PHND,center.x,center.y,radius)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::arc(pointf center, float radius, float start_angle, float end_angle, bool direction) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_arc(PHND,center.x,center.y,radius,start_angle,end_angle,direction)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::text(pointf location, const canvas_text_info& info) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    plutovg_font_face_t* face = plutovg_font_face_load_from_stream(*info.ttf_font,(int)info.ttf_font_face,nullptr,nullptr);
    if(face==nullptr) {
        return gfx_result::out_of_memory;
    }
    float advance_width = 0.f;
    int32_t cp;
    size_t length = info.text_byte_count;
    text_handle text = info.text;
    const uint8_t* data = (const uint8_t*)text;
    while(length) {
        size_t l = length;
        if(::gfx::gfx_result::success!=info.encoding->to_utf32((::gfx::text_handle)data,&cp,&l)) {
            return gfx_result::io_error;
        }
        data+=l;
        length-=l;
        advance_width += plutovg_font_face_get_glyph_path(face, info.font_size, location.x + advance_width, location.y,(plutovg_codepoint_t)cp, PHND);
    }
    
    plutovg_font_face_destroy(face);
    return gfx_result::success;
    
}
gfx_result canvas_path::path(const canvas_path& value, const ::gfx::matrix* transform) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_add_path(PHND,(plutovg_path_t*)value.m_info,transform)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
rectf canvas_path::bounds(bool precise) const {
    plutovg_rect_t r;
    plutovg_path_extents(PHND,&r,precise);
    return rectf(r.x,r.y,r.x+r.w-1,r.y+r.h-1);
}
float canvas_path::length(bool precise) const {
    return plutovg_path_extents(PHND,nullptr,precise);
}
gfx_result canvas_path::close() {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_close(PHND)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas_path::reserve(size_t elements) {
    gfx_result res=initialize();
    if(res!=gfx_result::success) {
        return res;
    }
    if(!plutovg_path_reserve(PHND,elements)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
void canvas_path::clear() {
    plutovg_path_reset(PHND);
}
}