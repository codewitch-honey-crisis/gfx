#ifndef HTCW_GFX_CANVAS_HPP
#define HTCW_GFX_CANVAS_HPP
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_palette.hpp"
#include "gfx_bitmap.hpp"
#include "gfx_font.hpp"
#include "gfx_encoding.hpp"
#include "gfx_vector_core.hpp"
#include <string.h>
namespace gfx {
    
struct canvas_text_info final {
    text_handle text;
    size_t text_byte_count;
    stream* ttf_font;
    size_t ttf_font_face;
    float font_size;
    const text_encoder* encoding;
    inline canvas_text_info() : text(nullptr),ttf_font(nullptr),ttf_font_face(0),encoding(&text_encoding::utf8) {}
    inline canvas_text_info(stream& ttf_font) : ttf_font(&ttf_font) {
        text = nullptr;
        text_byte_count = 0;
        ttf_font_face = 0;
        encoding = &text_encoding::utf8;
    }
    inline canvas_text_info(const text_handle text, size_t text_byte_count, stream& ttf_font, const text_encoder& encoding = text_encoding::utf8) {
        this->text = text;
        this->text_byte_count = text_byte_count;
        this->ttf_font = &ttf_font;
        this->ttf_font_face = 0;
        this->encoding = &encoding;
    }
    inline canvas_text_info(const char* text, stream& ttf_font, const text_encoder& encoding = text_encoding::utf8) {
        this->text = (text_handle)text;
        this->text_byte_count = strlen(text);
        this->text_byte_count = text_byte_count;
        this->ttf_font = &ttf_font;
        this->ttf_font_face = 0;
        this->encoding = &encoding;
    }
    inline void text_sz(const char* text) {
        this->text = (text_handle)text;
        if(text!=nullptr) {
            this->text_byte_count=strlen(text);
        } else {
            this->text_byte_count = 0;
        }
    }
};
class canvas;
class canvas_path {
    friend class canvas;
    void* m_info;
    void*(*m_allocator)(size_t);
    void*(*m_reallocator)(void*,size_t);
    void(*m_deallocator)(void*);
    canvas_path(const canvas_path& rhs)=delete;
    canvas_path& operator=(const canvas_path& rhs)=delete;
public: 
    canvas_path(void*(*allocator)(size_t)=::malloc,void*(*reallocator)(void*,size_t)=::realloc,void(*deallocator)(void*)=::free);
    virtual ~canvas_path();
    canvas_path(canvas_path&& rhs);
    canvas_path& operator=(canvas_path&& rhs);
    gfx_result initialize();
    bool initialized() const;
    void deinitialize();
    gfx_result move_to(pointf location);
    gfx_result line_to(pointf location);
    gfx_result quad_to(pointf point1, pointf point2);
    gfx_result cubic_to(pointf point1, pointf point2, pointf point3);
    gfx_result arc_to(sizef radiuses,float angle, bool large_arc, bool sweep, pointf location);
    gfx_result rectangle(const rectf& bounds);
    gfx_result rounded_rectangle(const rectf& bounds, sizef radiuses);
    gfx_result ellipse(pointf center, sizef radiuses);
    gfx_result circle(pointf center, float radius);
    gfx_result arc(pointf center, float radius, float start_angle, float end_angle, bool direction);
    gfx_result text(pointf location, const canvas_text_info& info);
    gfx_result path(const canvas_path& value, const ::gfx::matrix* transform = nullptr);
    rectf bounds(bool precise = false) const;
    float length(bool precise = false) const;
    gfx_result close();
    gfx_result reserve(size_t elements);
    void clear();

};
struct canvas_style {
    float stroke_opacity;
    //union {
        vector_pixel stroke_color;
        gradient stroke_gradient;
        texture stroke_texture;
    //};
    dash stroke_dash;
    paint_type stroke_paint_type;
    float stroke_width;
    line_cap stroke_line_cap;
    line_join stroke_line_join;
    float stroke_miter_limit;
    float fill_opacity;
    paint_type fill_paint_type;
    //union {
        vector_pixel fill_color;
        gradient fill_gradient;
        texture fill_texture;
    //};
    gfx::fill_rule fill_rule;
    float font_size;
};
class canvas final {
    void* m_info;
    canvas_style* m_style;
    size16 m_dimensions;
    rectf m_global_clip;
    vector_on_write_callback_type m_write_callback;
    vector_on_read_callback_type m_read_callback;
    void* m_callback_state;
    void(*m_free_callback_state)(void*);
    void*(*m_allocator)(size_t);
    void*(*m_reallocator)(void*, size_t);
    void(*m_deallocator)(void*);
    canvas(const canvas& rhs)=delete;
    canvas& operator=(const canvas& rhs)=delete;
public:
    canvas(void*(*allocator)(size_t)=::malloc, void*(*reallocator)(void*,size_t)=::realloc, void(*deallocator)(void*)=::free);
    canvas(size16 dimensions,void*(*allocator)(size_t)=::malloc, void*(*reallocator)(void*,size_t)=::realloc, void(*deallocator)(void*)=::free);
    canvas(canvas&& rhs);
    ~canvas();
    canvas& operator=(canvas&& rhs);
    gfx_result initialize();
    bool initialized() const;
    void deinitialize();
    vector_on_write_callback_type on_write_callback() const;
    vector_on_read_callback_type on_read_callback() const;
    void* callback_state() const;
    blt_span*  direct() const;
    gfx_result callbacks(size16 dimensions, spoint16 offset,vector_on_read_callback_type read_callback, vector_on_write_callback_type write_callback, void* callback_state, void(*free_callback_state)(void*)=nullptr);
    gfx_result direct(blt_span* target,size16 dimensions, spoint16 offset,on_direct_read_callback_type on_read,on_direct_write_callback_type on_write);
    size16 dimensions() const;
    void dimensions(size16 value);
    rect16 bounds() const;
    vector_pixel stroke_color() const;
    void stroke_color(vector_pixel value);
    float stroke_opacity() const;
    void stroke_opacity(float value);
    gradient stroke_gradient() const;
    void stroke_gradient(const gradient& value);
    texture stroke_texture() const;
    void stroke_texture(const texture& value);
    float stroke_width() const;
    void stroke_width(float value);
    float stroke_miter_limit() const;
    void stroke_miter_limit(float value);
    line_cap stroke_line_cap() const;
    void stroke_line_cap(line_cap value);
    line_join stroke_line_join() const;
    void stroke_line_join(line_join value);
    dash stroke_dash() const;
    void stroke_dash(const dash& value);
    paint_type stroke_paint_type() const;
    void stroke_paint_type(paint_type value);
    vector_pixel fill_color() const;
    void fill_color(vector_pixel value);
    float fill_opacity() const;
    void fill_opacity(float value);
    gradient fill_gradient() const;
    void fill_gradient(const gradient& value);
    texture fill_texture() const;
    void fill_texture(const texture& value);
    paint_type fill_paint_type() const;
    void fill_paint_type(paint_type value);
    inline void font(stream& ttf_stream, size_t index);
    float font_size() const;
    void font_size(float value);
    gfx::fill_rule fill_rule() const;
    void fill_rule(gfx::fill_rule value);
    gfx::compositing_mode compositing_mode() const;
    void compositing_mode(gfx::compositing_mode value);
    float opacity() const;
    void opacity(float value);
    canvas_style style();
    void style(const canvas_style& style);
    ::gfx::matrix transform() const;
    void transform(const ::gfx::matrix& value);
    rectf fill_bounds() const;
    rectf stroke_bounds() const;
    rectf clip_bounds() const;
    gfx_result move_to(pointf location);
    gfx_result line_to(pointf location);
    gfx_result quad_to(pointf point1, pointf point2);
    gfx_result cubic_to(pointf point1, pointf point2, pointf point3);
    gfx_result arc_to(sizef radiuses,float angle, bool large_arc, bool sweep, pointf location);
    gfx_result close_path();
    gfx_result reserve(size_t points);
    void clear_path();
    gfx_result rectangle(const rectf& bounds);
    gfx_result rounded_rectangle(const rectf& bounds, sizef radiuses);
    gfx_result ellipse(pointf center, sizef radiuses);
    gfx_result circle(pointf center, float radius);
    gfx_result arc(pointf center, float radius, float start_angle, float end_angle, bool direction);
    gfx_result text(pointf location, const canvas_text_info& info);
    gfx_result path(const canvas_path& value);
    gfx_result render(bool preserve=false,void*(*allocator)(size_t)=nullptr,void*(*reallocator)(void*,size_t)=nullptr,void(*deallocator)(void*)=nullptr);
    gfx_result render_svg(stream& document, const matrix& transform=matrix::create_identity(), float dpi = 96.f,void*(*allocator)(size_t)=nullptr,void*(*reallocator)(void*,size_t)=nullptr,void(*deallocator)(void*)=nullptr);
    static gfx_result svg_dimensions(stream& document, sizef* out_dimensions, float dpi= 96.f);
    gfx_result render_tvg(stream& document, const matrix& transform=matrix::create_identity(),void*(*allocator)(size_t)=nullptr,void*(*reallocator)(void*,size_t)=nullptr,void(*deallocator)(void*)=nullptr);
    static gfx_result tvg_dimensions(stream& document, sizef* out_dimensions);
};

};
#endif