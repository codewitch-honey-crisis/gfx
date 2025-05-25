#include <gfx_canvas.hpp>
#include <gfx_math.hpp>
#include <ml_reader.hpp>

#include "plutovg.h"
#define CHND ((plutovg_canvas_t*)m_info)
namespace gfx {


canvas::canvas(void*(*allocator)(size_t), void*(*reallocator)(void*,size_t), void(*deallocator)(void*)) : m_info(nullptr), m_style(nullptr), m_dimensions(0,0),m_global_clip(NAN,NAN,NAN,NAN),m_write_callback(nullptr),m_read_callback(nullptr), m_free_callback_state(nullptr),m_allocator(allocator),m_reallocator(reallocator),m_deallocator(deallocator) {

}
canvas::canvas(size16 dimensions,void*(*allocator)(size_t), void*(*reallocator)(void*,size_t), void(*deallocator)(void*)) : m_info(nullptr),m_style(nullptr), m_dimensions(dimensions),m_global_clip(NAN,NAN,NAN,NAN), m_write_callback(nullptr),m_read_callback(nullptr), m_free_callback_state(nullptr),m_allocator(allocator),m_reallocator(reallocator),m_deallocator(deallocator) {

}
canvas::canvas(canvas&& rhs) {
    m_dimensions = rhs.m_dimensions;
    rhs.m_dimensions=size16::zero();
    m_info = rhs.m_info;
    rhs.m_info = nullptr;
    m_style = rhs.m_style;
    rhs.m_style = nullptr;
    m_write_callback = rhs.m_write_callback;
    rhs.m_write_callback = nullptr;
    m_read_callback = rhs.m_read_callback;
    rhs.m_read_callback = nullptr;
    m_callback_state = rhs.m_callback_state;
    m_free_callback_state = rhs.m_free_callback_state;
    rhs.m_free_callback_state = nullptr;
    m_global_clip = rhs.m_global_clip;
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
}

canvas::~canvas() {
    deinitialize();
}
canvas& canvas::operator=(canvas&& rhs) {
    deinitialize();
    m_dimensions = rhs.m_dimensions;
    rhs.m_dimensions=size16::zero();
    m_info = rhs.m_info;
    rhs.m_info = nullptr;
    m_style = rhs.m_style;
    rhs.m_style = nullptr;
    m_write_callback = rhs.m_write_callback;
    rhs.m_write_callback = nullptr;
    m_read_callback = rhs.m_read_callback;
    rhs.m_read_callback = nullptr;
    m_callback_state = rhs.m_callback_state;
    m_free_callback_state = rhs.m_free_callback_state;
    rhs.m_free_callback_state = nullptr;
    m_global_clip = rhs.m_global_clip;
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
    return *this;
}
vector_on_write_callback_type canvas::on_write_callback() const {
    return m_write_callback;
}
vector_on_read_callback_type canvas::on_read_callback() const {
    return m_read_callback;
}

void* canvas::callback_state() const {
    return m_callback_state;
}
gfx_result canvas::callbacks(size16 dimensions, spoint16 offset,vector_on_read_callback_type read_callback, vector_on_write_callback_type write_callback, void* callback_state, void(*free_callback_state)(void*)) {
    if(m_callback_state!=nullptr && m_free_callback_state!=nullptr) {
        m_free_callback_state(m_callback_state);
        m_callback_state = nullptr;
    }
    if(write_callback==nullptr) {
        m_read_callback = nullptr;
        m_write_callback = nullptr;
        return gfx_result::success;
    }
    plutovg_canvas_set_direct(CHND,nullptr,0,0,0,0,nullptr,nullptr);
    spoint16 bmp_loc(-offset.x, -offset.y);
        
    rectf clip_rect;
    clip_rect.x1 =
        math::max_((float)bmp_loc.x, 0.f);
    clip_rect.y1 =
        math::max_((float)bmp_loc.y, 0.f);
    clip_rect.x2 = math::min_((float)bmp_loc.x + dimensions.width-1.f,
                                (float)m_dimensions.width-1.f);
    clip_rect.y2 = math::min_((float)bmp_loc.y + dimensions.height-1.f,
                                (float)m_dimensions.height-1.f);
    plutovg_rect_t r={clip_rect.x1,clip_rect.y1,clip_rect.width(),clip_rect.height()};
    plutovg_canvas_global_clip(CHND,&r);
    m_read_callback = read_callback;
    m_write_callback = write_callback;
    m_callback_state = callback_state;
    m_free_callback_state = free_callback_state;
    plutovg_canvas_set_callbacks(CHND,write_callback,read_callback,callback_state);
    return gfx_result::success;

}
                         
gfx_result canvas::direct(blt_span* bmp_data, size16 dimensions, spoint16 offset,on_direct_read_callback_type on_read,on_direct_write_callback_type on_write) {
    if(bmp_data==nullptr) {
        plutovg_canvas_set_direct(CHND,nullptr,offset.x,offset.y,0,0,on_read,on_write);
        
        plutovg_rect_t r={0.f,0.f,(float)m_dimensions.width,(float)m_dimensions.height};
        plutovg_canvas_global_clip(CHND,&r);
        return gfx_result::success;
    }
    if(on_read==nullptr || on_write==nullptr) {
        return gfx_result::invalid_argument;
    }
    spoint16 bmp_loc(offset.x, offset.y);
        
    rectf clip_rect;
    clip_rect.x1 =
        math::max_((float)bmp_loc.x, 0.f);
    clip_rect.y1 =
        math::max_((float)bmp_loc.y, 0.f);
    clip_rect.x2 = math::min_((float)bmp_loc.x + dimensions.width-1.f,
                                (float)m_dimensions.width-1.f);
    clip_rect.y2 = math::min_((float)bmp_loc.y + dimensions.height-1.f,
                                (float)m_dimensions.height-1.f);
    plutovg_rect_t r={clip_rect.x1,clip_rect.y1,clip_rect.width(),clip_rect.height()};
    plutovg_canvas_global_clip(CHND,&r);
    plutovg_canvas_set_direct(CHND,bmp_data,-offset.x,-offset.y,dimensions.width,dimensions.height,on_read,on_write);
    return gfx_result::success;
}
blt_span* canvas::direct() const {
    
    return plutovg_canvas_get_direct(CHND);
}

gfx_result canvas::initialize() {
    if(m_info!=nullptr) {
        return gfx_result::success;
    }
    if(m_dimensions==size16::zero()) {
        return gfx_result::invalid_state;
    }
    m_info = plutovg_canvas_create(m_dimensions.width,m_dimensions.height,m_allocator,m_reallocator,m_deallocator);
    if(m_info==nullptr) {
        return gfx_result::out_of_memory;
    }
    m_style = (canvas_style*)m_allocator(sizeof(canvas_style));
    if(m_style==nullptr) {
        plutovg_canvas_destroy(CHND);
        m_info = nullptr;
        return gfx_result::out_of_memory;
    }
    m_style->fill_color.template channel<channel_name::A,channel_name::R,channel_name::G,channel_name::B>(255,0,0,0);
    m_style->fill_paint_type = paint_type::solid;
    m_style->fill_opacity = 1;
    m_style->stroke_color.template channel<channel_name::A,channel_name::R,channel_name::G,channel_name::B>(255,0,0,0);
    m_style->stroke_paint_type = paint_type::none;
    m_style->stroke_dash.values_size = 0;
    m_style->stroke_dash.offset = 0;
    m_style->stroke_line_cap = line_cap::butt;
    m_style->stroke_line_join = line_join::miter;
    m_style->stroke_miter_limit = 1;
    m_style->stroke_opacity = 1;
    m_style->stroke_width = 1;
    m_style->font_size = 12.0f;
    m_style->fill_rule = fill_rule::non_zero;
    if(m_global_clip.x1!=m_global_clip.x1) {
        m_global_clip = rectf(pointf(0.f,0.f),sizef(m_dimensions.width,m_dimensions.height));
    }   
    plutovg_rect_t r = { m_global_clip.x1,m_global_clip.y1,m_global_clip.width(),m_global_clip.height() };
    plutovg_canvas_global_clip(CHND,&r);

    return gfx_result::success;
}
bool canvas::initialized() const {
    return m_info!=nullptr;
}
void canvas::deinitialize() {
    if(m_info!=nullptr) {
        plutovg_canvas_destroy(CHND);
        m_info = nullptr;
    }
    if(m_style!=nullptr) {
        m_deallocator(m_style);
        m_style = nullptr;
    }
    if(m_callback_state!=nullptr && m_free_callback_state!=nullptr) {
        m_free_callback_state(m_callback_state);
        m_callback_state = nullptr;
        m_free_callback_state = nullptr;
    }
    m_read_callback = nullptr;
    m_write_callback=nullptr;
}
size16 canvas::dimensions() const {
    return m_dimensions;
}
void canvas::dimensions(size16 value) {
    if(initialized() || value == size16::zero()) {
        return; // gfx_result::invalid_state;
    }
    m_dimensions = value;
}
rect16 canvas::bounds() const {
    return m_dimensions.bounds();
}
vector_pixel canvas::stroke_color() const {
    if(!initialized()) {
        return vector_pixel();
    }
    return m_style->stroke_color;
}
void canvas::stroke_color(vector_pixel value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_color=value;
    m_style->stroke_paint_type=paint_type::solid;
}
float canvas::stroke_opacity() const {
    if(!initialized()) {
        return 0.0f;
    }
    return m_style->stroke_opacity;
}
void canvas::stroke_opacity(float value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_opacity=value;
}
gradient canvas::stroke_gradient() const {
    if(!initialized()) {
        gradient res;
        res.transform = ::gfx::matrix::create_identity();
        res.spread = spread_method::pad;
        res.stops = nullptr;
        res.stops_size = 0;
        res.type = gradient_type::linear;
        res.linear.x1 = 0;
        res.linear.y1 = 0;
        res.linear.x2 = 0;
        res.linear.y2 = 0;
        return res;
    }
    return m_style->stroke_gradient;
}
void canvas::stroke_gradient(const gradient& value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_gradient=value;
    m_style->stroke_paint_type=paint_type::gradient;
}
texture canvas::stroke_texture() const {
    if(!initialized()) {
        texture res;
        res.transform = ::gfx::matrix::create_identity();
        res.opacity = 0.0f;
        res.direct = nullptr;
        res.dimensions = size16::zero();
        res.on_direct_read_callback = nullptr;
        res.on_read_callback = nullptr;
        res.type = texture_type::plain;
        return res;
    }
    return m_style->stroke_texture;
}
void canvas::stroke_texture(const texture& value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_texture=value;
    m_style->stroke_paint_type=paint_type::texture;
}
float canvas::stroke_width() const {
    if(!initialized()) {
        return 1;
    }
    return m_style->stroke_width;
}
void canvas::stroke_width(float value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_width = value;
}
line_cap canvas::stroke_line_cap() const {
    if(!initialized()) {
        return line_cap::butt;
    }
    return m_style->stroke_line_cap;
}
void canvas::stroke_line_cap(line_cap value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_line_cap = value;
}
line_join canvas::stroke_line_join() const {
    if(!initialized()) {
        return line_join::miter;
    }
    return m_style->stroke_line_join;
}
void canvas::stroke_line_join(line_join value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_line_join=value;
}
float canvas::stroke_miter_limit() const {
    if(!initialized()) {
        return 1;
    }
    return m_style->stroke_miter_limit;
}
void canvas::stroke_miter_limit(float value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_miter_limit = value;
}
dash canvas::stroke_dash() const {
    if(!initialized()) {
        dash res;
        res.offset = 0;
        res.values = nullptr;
        res.values_size = 0;
        return res;
    }
    return m_style->stroke_dash;
}
void canvas::stroke_dash(const dash& value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_dash = value;
}
paint_type canvas::stroke_paint_type() const {
    if(!initialized()) {
        return paint_type::none;
    }
    return m_style->stroke_paint_type;
}
void canvas::stroke_paint_type(paint_type value) {
    if(!initialized()) {
        return;
    }
    m_style->stroke_paint_type = value;
}

vector_pixel canvas::fill_color() const {
    if(!initialized()) {
        return vector_pixel();
    }
    return m_style->fill_color;
}
void canvas::fill_color(vector_pixel value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_color=value;
    m_style->fill_paint_type=paint_type::solid;
}
float canvas::fill_opacity() const {
    if(!initialized()) {
        return 0.0f;
    }
    return m_style->fill_opacity;
}
void canvas::fill_opacity(float value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_opacity=value;
}
gradient canvas::fill_gradient() const {
    if(!initialized()) {
        gradient res;
        res.transform = ::gfx::matrix::create_identity();
        res.spread = spread_method::pad;
        res.stops = nullptr;
        res.stops_size = 0;
        res.type = gradient_type::linear;
        res.linear.x1 = 0;
        res.linear.y1 = 0;
        res.linear.x2 = 0;
        res.linear.y2 = 0;
        return res;
    }
    return m_style->fill_gradient;
}
void canvas::fill_gradient(const gradient& value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_gradient=value;
    m_style->fill_paint_type=paint_type::gradient;
}
texture canvas::fill_texture() const {
    if(!initialized()) {
        texture res;
        res.transform = ::gfx::matrix::create_identity();
        res.opacity = 0.0f;
        res.direct = nullptr;
        res.dimensions = size16::zero();
        res.on_direct_read_callback = nullptr;
        res.on_read_callback = nullptr;
        res.type = texture_type::plain;
        return res;
    }
    return m_style->fill_texture;
}
void canvas::fill_texture(const texture& value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_texture=value;
    m_style->fill_paint_type=paint_type::texture;
}
paint_type canvas::fill_paint_type() const {
    if(!initialized()) {
        return paint_type::none;
    }
    return m_style->fill_paint_type;
}
void canvas::fill_paint_type(paint_type value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_paint_type = value;
}

void canvas::font(stream& ttf_stream, size_t index) {
    if(!initialized()) {
        return;
    }
    plutovg_font_face_t* ff = plutovg_font_face_load_from_stream(ttf_stream,index,nullptr,nullptr);
    plutovg_canvas_set_font_face(CHND,ff);
}
float canvas::font_size() const {
    if(!initialized()) {
        return 0.0f;
    }
    return m_style->font_size;
    
}
void canvas::font_size(float value) {
    if(!initialized()) {
        return;
    }
    m_style->font_size = value;
}
gfx::fill_rule canvas::fill_rule() const {
    if(!initialized()) {
        return fill_rule::non_zero;
    }
    return m_style->fill_rule;
}
void canvas::fill_rule(gfx::fill_rule value) {
    if(!initialized()) {
        return;
    }
    m_style->fill_rule = value;
}
gfx::compositing_mode canvas::compositing_mode() const {
    if(!initialized()) {
        return compositing_mode::source;
    }
    return (gfx::compositing_mode)(int)plutovg_canvas_get_operator(CHND);
}
void canvas::compositing_mode(gfx::compositing_mode value) {
    if(!initialized()) {
        return;
    }
    plutovg_canvas_set_operator(CHND,(plutovg_operator_t)(int)value);
}
float canvas::opacity() const {
    if(!initialized()) {
        return 0.0f;
    }
    return plutovg_canvas_get_opacity(CHND);
}
void canvas::opacity(float value) {
    if(!initialized()) {
        return;
    }
    plutovg_canvas_set_opacity(CHND,value);
}
canvas_style canvas::style() {
    canvas_style result;
    if(!initialized()) {
        result.fill_color.template channel<channel_name::A,channel_name::R,channel_name::G,channel_name::B>(255,0,0,0);
        result.fill_paint_type = paint_type::solid;
        result.fill_opacity = 1;
        result.stroke_color.template channel<channel_name::A,channel_name::R,channel_name::G,channel_name::B>(255,0,0,0);
        result.stroke_paint_type = paint_type::none;
        result.stroke_dash.values_size = 0;
        result.stroke_dash.offset = 0;
        result.stroke_line_cap = line_cap::butt;
        result.stroke_line_join = line_join::miter;
        result.stroke_miter_limit = 1;
        result.stroke_opacity = 1;
        result.stroke_width = 1;
        result.font_size = 12.0f;
        result.fill_rule = fill_rule::non_zero;
        return result;
    }
    result = *((canvas_style*)m_style);
    return result;
}
void canvas::style(const canvas_style& style) {
    if(!initialized()) {
        return;
    }
    *((canvas_style*)m_style)=style;
}
::gfx::matrix canvas::transform() const {
    if(!initialized()) {
        return ::gfx::matrix::create_identity();
    }
    ::gfx::matrix res;
    plutovg_canvas_get_matrix(CHND,&res);
    return res;
}
void canvas::transform(const ::gfx::matrix& rhs) {
    if(!initialized()) {
        return;
    }
    plutovg_canvas_set_matrix(CHND,&rhs);
}
rectf canvas::fill_bounds() const {
    if(!initialized()) return {0,0,0,0};
    plutovg_rect_t r;
    plutovg_canvas_fill_extents(CHND,&r);
    return rectf(r.x,r.y,r.x+r.w-1,r.y+r.h-1);
}
rectf canvas::stroke_bounds() const {
    if(!initialized()) return {0,0,0,0};
    plutovg_rect_t r;
    if(!plutovg_canvas_stroke_extents(CHND,&r)) {
        return {NAN,NAN,NAN,NAN};
    }
    return rectf(r.x,r.y,r.x+r.w-1,r.y+r.h-1);
}
rectf canvas::clip_bounds() const {
    if(!initialized()) return {0,0,0,0};
    plutovg_rect_t r;
    plutovg_canvas_clip_extents(CHND,&r);
    return rectf(r.x,r.y,r.x+r.w-1,r.y+r.h-1);
}
gfx_result canvas::move_to(pointf location) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_move_to(CHND,location.x,location.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::line_to(pointf location) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_line_to(CHND,location.x,location.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::quad_to(pointf point1, pointf point2) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_quad_to(CHND,point1.x,point1.y,point2.x,point2.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::cubic_to(pointf point1, pointf point2, pointf point3) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_cubic_to(CHND,point1.x,point1.y,point2.x,point2.y,point3.x,point3.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::arc_to(sizef radiuses,float angle, bool large_arc, bool sweep, pointf location) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_arc_to(CHND,radiuses.width,radiuses.height,angle,large_arc,sweep,location.x,location.y)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::close_path() {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_close_path(CHND)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
void canvas::clear_path() {
    if(!initialized()) return;
    plutovg_canvas_new_path(CHND);
}
gfx_result canvas::rectangle(const rectf& bounds) {
    if(!initialized()) return gfx_result::invalid_state;
    rectf n = bounds.normalize();
    if(!plutovg_canvas_rect(CHND,n.x1,n.y1,n.x2-n.x1+1,n.y2-n.y1+1)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::rounded_rectangle(const rectf& bounds, sizef radiuses) {
    if(!initialized()) return gfx_result::invalid_state;
    rectf n = bounds.normalize();
    if(!plutovg_canvas_round_rect(CHND,n.x1,n.y1,n.x2-n.x1+1,n.y2-n.y1+1,radiuses.width,radiuses.height)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::ellipse(pointf center, sizef radiuses) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_ellipse(CHND,center.x,center.y,radiuses.width,radiuses.height)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::circle(pointf center, float radius) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_circle(CHND,center.x,center.y,radius)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::arc(pointf center, float radius,float a0, float a1, bool ccw) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_arc(CHND,center.x,center.y,radius,a0,a1,ccw)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::text(pointf location, const canvas_text_info& info) {
    if(!initialized()) return gfx_result::invalid_state;
    if(info.ttf_font==nullptr) {
        return gfx_result::invalid_argument;
    }
    if(info.ttf_font->caps().seek) {
        info.ttf_font->seek(0);
    }
    plutovg_font_face_t* face = plutovg_font_face_load_from_stream(*info.ttf_font,(int)info.ttf_font_face,nullptr,nullptr);
    if(face==nullptr) {
        return gfx_result::invalid_format; // TODO: might be out of memory
    }
    plutovg_canvas_set_font_face(CHND,face);
    plutovg_canvas_set_font_size(CHND,info.font_size);
    plutovg_canvas_add_text(CHND,info.text,info.text_byte_count,info.encoding,location.x,location.y);
    plutovg_canvas_set_font_face(CHND,nullptr);
    plutovg_font_face_destroy(face);
    return gfx_result::success;
}
gfx_result canvas::path(const canvas_path& path) {
    if(!initialized()) return gfx_result::invalid_state;
    if(!plutovg_canvas_add_path(CHND,(plutovg_path_t*)path.m_info)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::reserve(size_t points) {
    if(!plutovg_path_reserve(plutovg_canvas_get_path(CHND), points*2)) {
        return gfx_result::out_of_memory;
    }
    return gfx_result::success;
}
gfx_result canvas::render(bool preserve,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*)) {
    if(!initialized()) return gfx_result::invalid_state;
    if(allocator==nullptr) allocator = m_allocator;
    if(reallocator==nullptr) reallocator = m_reallocator;
    if(deallocator==nullptr) deallocator = m_deallocator;
    plutovg_canvas_set_fill_rule(CHND,(plutovg_fill_rule_t)(int)m_style->fill_rule);
    plutovg_canvas_set_font_size(CHND,m_style->font_size);
    bool paint_fill = m_style->fill_paint_type!=paint_type::none;
    bool paint_stroke = m_style->stroke_paint_type!=paint_type::none;
    
    if(m_style->fill_paint_type==paint_type::solid) {
        plutovg_color_t col;
        plutovg_canvas_set_opacity(CHND,m_style->fill_opacity);
        col.a = m_style->fill_color.template channelr<channel_name::A>();
        col.r = m_style->fill_color.template channelr<channel_name::R>();
        col.g = m_style->fill_color.template channelr<channel_name::G>();
        col.b = m_style->fill_color.template channelr<channel_name::B>();
        if(!plutovg_canvas_set_color(CHND,&col)) {
            return gfx_result::out_of_memory;
        }
    } else if(m_style->fill_paint_type==paint_type::gradient) {
        plutovg_canvas_set_opacity(CHND,m_style->fill_opacity);
        if(m_style->fill_gradient.type==gradient_type::linear) {
            if(!plutovg_canvas_set_linear_gradient(CHND,
                                        m_style->fill_gradient.linear.x1,
                                        m_style->fill_gradient.linear.y1,
                                        m_style->fill_gradient.linear.x2,
                                        m_style->fill_gradient.linear.y2,
                                        (plutovg_spread_method_t)(int)m_style->fill_gradient.spread,
                                        m_style->fill_gradient.stops,
                                        (int)m_style->fill_gradient.stops_size,
                                        &m_style->fill_gradient.transform)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_set_radial_gradient(CHND,m_style->fill_gradient.radial.cx,m_style->fill_gradient.radial.cy,m_style->fill_gradient.radial.cr,m_style->fill_gradient.radial.fx,
                                        m_style->fill_gradient.radial.fy,m_style->fill_gradient.radial.fr,(plutovg_spread_method_t)(int)m_style->fill_gradient.spread,
                                        m_style->fill_gradient.stops,
                                        (int)m_style->fill_gradient.stops_size,
                                        &m_style->fill_gradient.transform)) {
                return gfx_result::out_of_memory;
            }
        }
    } else if(m_style->fill_paint_type==paint_type::texture) {
        plutovg_canvas_set_opacity(CHND,m_style->fill_opacity);
        if(m_style->fill_texture.direct!=nullptr) {
            if(!plutovg_canvas_set_texture_direct(CHND,m_style->fill_texture.dimensions.width,
                                m_style->fill_texture.dimensions.height,
                                m_style->fill_texture.direct,
                                m_style->fill_texture.on_direct_read_callback,
                                (plutovg_texture_type_t)(int)m_style->fill_texture.type,
                                m_style->fill_texture.opacity,
                                &m_style->fill_texture.transform)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_set_texture(CHND,
                            m_style->fill_texture.dimensions.width,
                            m_style->fill_texture.dimensions.height,
                            m_style->fill_texture.on_read_callback,
                            m_style->fill_texture.on_read_callback_state,
                            (plutovg_texture_type_t)(int)m_style->fill_texture.type,
                            m_style->fill_texture.opacity,
                            &m_style->fill_texture.transform)){
                return gfx_result::out_of_memory;
            }
        }
    }
    
    if(paint_fill) {
        if(paint_stroke || preserve) {
            if(!plutovg_canvas_fill_preserve(CHND,allocator,reallocator,deallocator)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_fill(CHND,allocator,reallocator,deallocator)) {
                return gfx_result::out_of_memory;
            }
        }
    }
    if(paint_stroke) {
        plutovg_canvas_set_line_width(CHND,m_style->stroke_width);
        if(m_style->stroke_dash.values_size>0) {
            plutovg_canvas_set_dash(CHND,m_style->stroke_dash.offset,m_style->stroke_dash.values,m_style->stroke_dash.values_size);
        }
        plutovg_canvas_set_line_cap(CHND,(plutovg_line_cap_t)(int)m_style->stroke_line_cap);
        plutovg_canvas_set_line_join(CHND,(plutovg_line_join_t)(int)m_style->stroke_line_join);
        plutovg_canvas_set_miter_limit(CHND,m_style->stroke_miter_limit);
    }
    if(m_style->stroke_paint_type==paint_type::solid) {
        plutovg_color_t col;
        plutovg_canvas_set_opacity(CHND,m_style->stroke_opacity);
        col.a = m_style->stroke_color.template channelr<channel_name::A>();
        col.r = m_style->stroke_color.template channelr<channel_name::R>();
        col.g = m_style->stroke_color.template channelr<channel_name::G>();
        col.b = m_style->stroke_color.template channelr<channel_name::B>();
        if(!plutovg_canvas_set_color(CHND,&col)) {
            return gfx_result::out_of_memory;
        }
    } else if(m_style->stroke_paint_type==paint_type::gradient) {
        plutovg_canvas_set_opacity(CHND,m_style->stroke_opacity);
        if(m_style->stroke_gradient.type==gradient_type::linear) {
            if(!plutovg_canvas_set_linear_gradient(CHND,
                                        m_style->stroke_gradient.linear.x1,
                                        m_style->stroke_gradient.linear.y1,
                                        m_style->stroke_gradient.linear.x2,
                                        m_style->stroke_gradient.linear.y2,
                                        (plutovg_spread_method_t)(int)m_style->stroke_gradient.spread,
                                        m_style->stroke_gradient.stops,
                                        (int)m_style->stroke_gradient.stops_size,
                                        &m_style->stroke_gradient.transform)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_set_radial_gradient(CHND,
                                        m_style->stroke_gradient.radial.cx,
                                        m_style->stroke_gradient.radial.cy,
                                        m_style->stroke_gradient.radial.cr,
                                        m_style->stroke_gradient.radial.fx,
                                        m_style->stroke_gradient.radial.fy,
                                        m_style->stroke_gradient.radial.fr,
                                        (plutovg_spread_method_t)(int)m_style->stroke_gradient.spread,
                                        m_style->stroke_gradient.stops,
                                        (int)m_style->stroke_gradient.stops_size,
                                        &m_style->stroke_gradient.transform)) {
                return gfx_result::out_of_memory;
            }
        }
    } else if(m_style->stroke_paint_type==paint_type::texture) {
        plutovg_canvas_set_opacity(CHND,m_style->stroke_opacity);
        if(m_style->stroke_texture.direct!=nullptr) {
            if(!plutovg_canvas_set_texture_direct(CHND,m_style->stroke_texture.dimensions.width,
                                m_style->stroke_texture.dimensions.height,
                                m_style->stroke_texture.direct,
                                m_style->stroke_texture.on_direct_read_callback,
                                (plutovg_texture_type_t)(int)m_style->stroke_texture.type,
                                m_style->stroke_texture.opacity,
                                &m_style->stroke_texture.transform)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_set_texture(CHND,
                            m_style->stroke_texture.dimensions.width,
                            m_style->stroke_texture.dimensions.height,
                            m_style->stroke_texture.on_read_callback,
                            m_style->stroke_texture.on_read_callback_state,
                            (plutovg_texture_type_t)(int)m_style->stroke_texture.type,
                            m_style->stroke_texture.opacity,
                            &m_style->stroke_texture.transform)) {
                return gfx_result::out_of_memory;
            }
        }
    }
    if(paint_stroke) {
        if(preserve) {
            if(!plutovg_canvas_stroke_preserve(CHND,allocator,reallocator,deallocator)) {
                return gfx_result::out_of_memory;
            }
        } else {
            if(!plutovg_canvas_stroke(CHND,allocator,reallocator,deallocator)) {
                return gfx_result::out_of_memory;
            }
        }
    }
    
    

    return gfx_result::success;
}
}