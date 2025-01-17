#ifndef HTCW_GFX_VECTOR_CORE_HPP
#define HTCW_GFX_VECTOR_CORE_HPP
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_bitmap.hpp"
#include "gfx_positioning.hpp"
namespace gfx {
// ARGB8888 pixel format
using vector_pixel = pixel<
    channel_traits<channel_name::A,8,0,255,255>,
    channel_traits<channel_name::R,8>,
    channel_traits<channel_name::G,8>,
    channel_traits<channel_name::B,8>
>;
struct matrix {
    float a,b,c,d,e,f;
    matrix(float a, float b, float c, float d, float e, float f);
    matrix();
    matrix(const matrix& rhs);
    matrix& operator=(const matrix& rhs);
    matrix(matrix&& rhs);
    matrix& operator=(matrix&& rhs);
    matrix operator*(const matrix& rhs);
    matrix& operator*=(const matrix& rhs);
    static bool invert(const matrix& rhs, matrix* out_inverted);
    void map(float x,float y,float* out_xx, float* out_yy) const;
    void map_point(const pointf& src,pointf* out_dst) const;
    void map_points(const pointf* src,pointf* out_dst, size_t size) const;
    void map_rect(const rectf& src, rectf* out_dst) const;
    matrix translate(float x, float y);
    matrix scale(float xs, float ys);
    matrix skew(float xa, float ya);
    matrix skew_x(float angle);
    matrix skew_y(float angle);
    matrix rotate(float angle);
    matrix& translate_inplace(float x, float y);
    matrix& scale_inplace(float xs, float ys);
    matrix& skew_inplace(float xa, float ya);
    matrix& skew_x_inplace(float angle);
    matrix& skew_y_inplace(float angle);
    matrix& rotate_inplace(float angle);
    static matrix create_identity();
    static matrix create_translate(float x, float y);
    static matrix create_scale(float xs, float ys);
    static matrix create_skew(float xa, float ya);
    static matrix create_skew_x(float angle);
    static matrix create_skew_y(float angle);
    static matrix create_rotate(float angle);
    static matrix create_fit_to(sizef dimensions,const rectf& bounds);
private:
    void copy_from(const matrix& rhs);
};

typedef gfx_result(*vector_on_write_callback_type)(const rect16& bounds, vector_pixel color, void* state);
typedef gfx_result(*vector_on_read_callback_type)(point16 location, vector_pixel* out_color, void* state);

enum struct paint_type {
    none = 0,
    solid,
    gradient,
    texture
};

enum struct texture_type {
    plain = 0,
    tiled
};
class texture;
namespace helpers {   
    static void texture_set_callback(texture* txt, const_blt_span* source,on_direct_read_callback_type read_callback);
    static void texture_set_callback(texture* txt, void* source,vector_on_read_callback_type read_callback);
    template<typename Source>
    static gfx_result read_callback(point16 location,vector_pixel* out_pixel, void* state) {
        Source& dst = *(Source*)state;
        typename Source::pixel_type px;
        dst.point(location,&px);
        convert_palette_to(dst,px,out_pixel);
        return gfx_result::success;
        
    }
    template<typename Source,bool BltSpans>
    struct canvas_texture_bind_helper {
        static gfx_result set(Source& source,texture* out_texture) {
            texture_set_callback(out_texture,&source,read_callback<Source>);
            return gfx_result::success;    
        }
    };
    template<typename Source>
    struct canvas_texture_bind_helper<Source,true> {
        static gfx_result set(Source& source,texture* out_texture) {
            bool direct = false;
            if(Source::caps::blt_spans) {
                if(helpers::is_same<gfx::rgb_pixel<16>,typename Source::pixel_type>::value) {
                    texture_set_callback(out_texture,static_cast<const_blt_span*>(&source),helpers::xread_callback_rgb16);
                    direct=true;
                } else if(helpers::is_same<gfx::rgba_pixel<32>,typename Source::pixel_type>::value) {
                    texture_set_callback(out_texture,static_cast<const_blt_span*>(&source),helpers::xread_callback_rgba32);
                    direct=true;
                } else if(Source::pixel_type::byte_alignment!=0) {
                    texture_set_callback(out_texture,static_cast<const_blt_span*>(&source),helpers::xread_callback_any<Source>);
                    direct=true;
                }
            }
            if(!direct) {
                texture_set_callback(out_texture,&source,read_callback<Source>);
            }
            return gfx_result::success;   
        }
    };
}
class texture {   
public:
    texture_type type;
    float opacity;
    ::gfx::matrix transform;
    size16 dimensions;
    vector_on_read_callback_type on_read_callback;
    void* on_read_callback_state;
    const_blt_span* direct;
    on_direct_read_callback_type on_direct_read_callback;
    template<typename Source>
    static gfx_result create_from(Source& source,texture* out_texture) {
        // "unused" functions (shut GCC up)
        (void)(0 ? ((void)helpers::texture_set_callback(nullptr,nullptr,(::gfx::vector_on_read_callback_type) nullptr), 0) : 0);
        (void)(0 ? ((void)helpers::xread_callback_rgba32(nullptr,nullptr,0), 0) : 0);
        (void)(0 ? ((void)helpers::xread_callback_rgba32p(nullptr,nullptr,0), 0) : 0);
        (void)(0 ? ((void)helpers::xread_callback_rgb16(nullptr,nullptr,0), 0) : 0);
        (void)(0 ? ((void)helpers::xwrite_callback_rgba32(nullptr,nullptr,0), 0) : 0);
        (void)(0 ? ((void)helpers::xwrite_callback_rgb16(nullptr,nullptr,0), 0) : 0);
        if(out_texture==nullptr) {
            return gfx_result::invalid_argument;
        }
        out_texture->type = texture_type::plain;
        out_texture->dimensions = source.dimensions();
        out_texture->opacity = 1.f;
        out_texture->transform = matrix::create_identity();
        out_texture->direct = nullptr;
        return helpers::canvas_texture_bind_helper<Source,Source::caps::blt_spans>::set(source,out_texture);
    }

};
namespace helpers {
    static void texture_set_callback(gfx::texture* txt, gfx::const_blt_span* source,gfx::on_direct_read_callback_type read_callback) {
        txt->direct=source;
        txt->on_direct_read_callback = read_callback;
    }
    static void texture_set_callback(gfx::texture* txt, void* source,gfx::vector_on_read_callback_type read_callback) {
        txt->on_read_callback = read_callback;
        txt->on_read_callback_state = source;
    }

}
struct gradient_stop {
    float offset;
    vector_pixel color;
};
enum struct gradient_type {
    linear = 0,
    radial = 1
};
enum struct spread_method {
    pad = 0,
    reflect,
    repeat
};
struct gradient {
    gradient_type type;
    spread_method spread;
    ::gfx::matrix transform;
    union {
        struct {
            float x1,y1,x2,y2;
        } linear;
        struct {
            float cx,cy,fx,fy,cr,fr;
        } radial;
    };
    gradient_stop* stops;
    size_t stops_size;
};
struct dash {
    float offset;
    float* values;
    size_t values_size;
};
enum struct line_cap {
    butt = 0, ///< Flat edge at the end of the stroke.
    round, ///< Rounded ends at the end of the stroke.
    square ///< Square ends at the end of the stroke.
};
enum struct line_join {
    miter=0, ///< Miter join with sharp corners.
    round, ///< Rounded join.
    bevel ///< Beveled join with a flattened corner.
};
struct stroke_style {
    float width;
    line_cap cap;
    line_join join;
    float miter_limit;
};
enum struct fill_rule {
    non_zero = 0,
    even_odd
};
enum compositing_mode {
    source = 0,
    source_over,
    destination_inside,
    destination_outside
};
}
#endif