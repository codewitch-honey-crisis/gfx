#ifndef HTCW_GFX_SVG_DOC_HPP
#define HTCW_GFX_SVG_DOC_HPP
#include <stdlib.h>
#include <string.h>

#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include "gfx_pixel.hpp"
namespace gfx {
enum struct svg_paint_type : int8_t {
    undefined = -1,
    none = 0,
    color = 1,
    linear_gradient = 2,
    radial_gradient = 3
};
enum struct svg_gradient_units : uint8_t {
    user_space = 0,
    object_space = 1
};
enum struct svg_spread_type : uint8_t {
    pad = 0,
    reflect = 1,
    repeat = 2
};

enum struct svg_line_join : uint8_t {
    miter = 0,
    round = 1,
    bevel = 2
};

enum struct svg_line_cap : uint8_t {
    butt = 0,
    round = 1,
    square = 2
};

enum struct svg_fill_rule : uint8_t {
    non_zero = 0,
    even_odd = 1
};

enum struct svg_sweep_direction : uint8_t {
    left = 0,
    right = 1
};

struct svg_gradient_stop final {
    gfx::rgba_pixel<32> color;
    float offset;
};

struct svg_transform final {
    float data[6];
    void identity();
    void set_translation(float tx, float ty);
    void set_scale(float sx, float sy);
    void set_skew_x(float a);
    void set_skew_y(float a);
    void set_rotation(float a);
    void multiply(const svg_transform& rhs);
    void inverse(svg_transform& rhs);
    void premultiply(const svg_transform& rhs);
    void point(gfx::pointf* pt_d, gfx::pointf pt) const;
    void vector(gfx::pointf* pt_d, gfx::pointf pt) const;
};

struct svg_gradient final {
    svg_transform xform;
    svg_spread_type spread;
    gfx::pointf f;
    size_t stop_count;
    svg_gradient_stop stops[1];
};
enum struct svg_units : uint8_t {
    user = 0,
    px = 1,
    pt = 2,
    pc = 3,
    mm = 4,
    cm = 5,
    in = 6,
    percent = 7,
    em = 8,
    ex = 9,
    dpi = 10
};
struct svg_coordinate {
    float value;
    svg_units units;
}; 
struct svg_paint final {
    svg_paint_type type;
    union {
        gfx::rgba_pixel<32> color;
        svg_gradient* gradient;
    };
};
class svg_path final {
   public:
    float* points;      // Cubic bezier points: x0,y0, [cpx1,cpx1,cpx2,cpy2,x1,y1], ...
    int point_count;    // Total number of bezier points.
    char closed;        // Flag indicating if shapes should be treated as closed.
    gfx::rectf bounds;  // Tight bounding box of the shape [minx,miny,maxx,maxy].
    svg_path* next;     // Pointer to next path, or NULL if last element.
};
struct svg_gradient_info;
struct svg_shape final {
    char id[64];                     // Optional 'id' attr of the shape or its group
    svg_paint fill;                  // Fill paint
    svg_paint stroke;                // Stroke paint
    float opacity;                   // Opacity of the shape.
    float stroke_width;              // Stroke width (scaled).
    float stroke_dash_offset;        // Stroke dash offset (scaled).
    float stroke_dash_array[8];      // Stroke dash array (scaled).
    size_t stroke_dash_count;        // Number of dash values in dash array.
    svg_line_join stroke_line_join;  // Stroke join type.
    svg_line_cap stroke_line_cap;    // Stroke cap type.
    float miter_limit;               // Miter limit
    svg_fill_rule fill_rule;         // Fill rule, see svg_fill_rule.
    unsigned char flags;             // Logical or of NSVG_FLAGS_* flags
    gfx::rectf bounds;               // Tight bounding box of the shape [minx,miny,maxx,maxy].
    union {
        char fill_gradient_id[64];       // Optional 'id' of fill gradient
        svg_gradient_info* fill_gradient;       // Optional 'id' of fill gradient
    };
    union {
        
        char stroke_gradient_id[64];  // Optional 'id' of stroke gradient
        svg_gradient_info* stroke_gradient;
    };
    svg_transform xform;  // Root transformation for fill/stroke gradient
    svg_path* paths;          // Linked list of paths in the image.
    svg_shape* next;          // Pointer to next shape, or NULL if last element.
};

struct svg_image final {
    gfx::sizef dimensions;
    svg_shape* shapes;  // Linked list of shapes in the image.
};

enum struct svg_gradient_type : uint8_t {
    linear = 0,
    radial = 1
};
struct svg_linear_gradient_info final {
    svg_coordinate x1, y1, x2, y2;
};
struct svg_radial_gradient_info final {
    svg_coordinate center_x, center_y, focus_x, focus_y, radius;
};
struct svg_gradient_info final {
    svg_gradient_type type;
    union {
        svg_linear_gradient_info linear;
        svg_radial_gradient_info radial;
    };
    svg_gradient_units units;
    svg_spread_type spread;
    svg_transform xform;
    size_t stop_count;
    svg_gradient_stop* stops;
    svg_gradient_info* next;
};
struct svg_paint_info final {
    svg_paint_type type;
    union {
        rgba_pixel<32> color;
        svg_gradient_info* gradient;
    };
};
struct svg_shape_info final {
    svg_paint_info fill;    
    svg_paint_info stroke;
    float stroke_width;
    float stroke_dash_offset;
    float stroke_dash_array[8];
    size_t stroke_dash_count;
    svg_line_join stroke_line_join;
    svg_line_cap stroke_line_cap;
    float miter_limit;
    svg_fill_rule fill_rule;
    svg_transform xform;
    bool hidden;
    svg_shape_info();
};
class svg_path_builder final {
    void* (*m_allocator)(size_t);
    void* (*m_reallocator)(void*, size_t);
    void (*m_deallocator)(void*);
    float* m_begin;
    size_t m_size;
    size_t m_capacity;
    rectf m_cp;
    void do_free();
    void do_copy(const svg_path_builder& rhs);
    void do_move(svg_path_builder& rhs);
    gfx_result add_point(pointf pt);
    gfx_result move_to_impl(pointf pt);
    gfx_result line_to_impl(pointf pt);
    gfx_result cubic_bezier_to_impl(pointf pt, const rectf& cp);
   public:
    svg_path_builder(void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    svg_path_builder(const svg_path_builder& rhs);
    svg_path_builder& operator=(const svg_path_builder& rhs);
    svg_path_builder(svg_path_builder&& rhs);
    svg_path_builder& operator=(svg_path_builder&& rhs);
    ~svg_path_builder();
    size_t size() const;
    void clear(bool keep_capacity=true);
    size_t capacity() const;
    float* begin();
    float* end();
    const float* cbegin() const;
    const float* cend() const;
    gfx_result move_to(pointf location, bool relative=false);
    gfx_result line_to(pointf location, bool relative=false);
    gfx_result cubic_bezier_to(pointf location, const rectf& control_points, bool relative=false);
    gfx_result quad_bezier_to(pointf location, pointf control_point,bool relative=false);
    gfx_result arc_to(pointf location, sizef radius, float x_angle, bool large_arc, svg_sweep_direction sweep_direction, bool relative=false);
    gfx_result to_path(svg_path** out_path, bool closed = false, const svg_transform* xform = nullptr);
};
class svg_gradient_builder final {
    void* (*m_allocator)(size_t);
    void* (*m_reallocator)(void*, size_t);
    void (*m_deallocator)(void*);
    
    svg_gradient_info *m_gradient;
    void do_free();
    void do_copy(const svg_gradient_builder& rhs);
    void do_move(svg_gradient_builder& rhs);
public:
    svg_gradient_builder(void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    svg_gradient_builder(const svg_gradient_builder& rhs);
    svg_gradient_builder& operator=(const svg_gradient_builder& rhs);
    svg_gradient_builder(svg_gradient_builder&& rhs);
    svg_gradient_builder& operator=(svg_gradient_builder&& rhs);
    ~svg_gradient_builder();
    gfx_result add_stop(rgba_pixel<32> color, float offset);
    void clear();
    gfx_result to_linear_gradient(svg_gradient_info** out_gradient, svg_gradient_units units = svg_gradient_units::object_space, svg_coordinate x1 = {0, svg_units::percent}, svg_coordinate y1 = {0, svg_units::percent}, svg_coordinate x2 = {100, svg_units::percent}, svg_coordinate y2 = {100, svg_units::percent}, svg_spread_type spread = svg_spread_type::pad, const svg_transform* xform = nullptr);
    gfx_result to_radial_gradient(svg_gradient_info** out_gradient, svg_gradient_units units = svg_gradient_units::object_space, svg_coordinate center_x = {50, svg_units::percent}, svg_coordinate center_y = {50, svg_units::percent}, svg_coordinate focus_x = {50, svg_units::percent}, svg_coordinate focus_y = {50, svg_units::percent}, svg_coordinate radius = {50, svg_units::percent}, svg_spread_type spread = svg_spread_type::pad, const svg_transform* xform = nullptr);
};
class svg_doc_builder;
class svg_doc final {
    friend class svg_doc_builder;
    svg_image* m_doc_data;
    void*(*m_allocator)(size_t);
    void* (*m_reallocator)(void*,size_t);
    void (*m_deallocator)(void*);
    void do_move(svg_doc& rhs);
    void do_free();
    svg_doc(const svg_doc& rhs) = delete;
    svg_doc& operator=(const svg_doc& rhs) = delete;

   public:
    svg_doc(void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    ~svg_doc();
    svg_doc(svg_doc&& rhs);
    svg_doc(stream* svg_stream, uint16_t dpi = 96, void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    svg_doc& operator=(svg_doc&& rhs);
    static gfx_result read(stream* input, svg_doc* out_doc, uint16_t dpi = 96, void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    bool initialized() const;
    void draw(float scale, const srect16& rect, void(read_callback)(int x, int y, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* a, void* state), void* read_callback_state, void(write_callback)(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* state), void* write_callback_state, void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free) const;
    float scale(float line_height) const;
    float scale(sizef dimensions) const;
    float scale(ssize16 dimensions) const;
    float scale(size16 dimensions) const;
    sizef dimensions() const;
};
class svg_doc_builder final {
    rectf m_view_box;
    uint16_t m_dpi;
    float m_font_size;
    void* (*m_allocator)(size_t);
    void* (*m_reallocator)(void*, size_t);
    void (*m_deallocator)(void*);
    svg_path_builder m_builder;
    svg_shape *m_shape,*m_shape_tail;
    svg_gradient_info *m_gradients,*m_gradients_tail;
    float to_pixels(svg_coordinate coord, float orig, float length);
    void add_gradient(svg_gradient_info* grad);
    svg_gradient* create_gradient(const svg_gradient_info& info, const float* local_bounds, float* xform, svg_paint_type* paint_type);
    svg_shape* create_shape(svg_shape_info& info);
    gfx_result add_poly_impl(const pathf& path, bool closed, svg_shape_info& shape_info);
    void add_shape(svg_shape* shape);
    void do_free_gradient_infos();
    void do_free();
    void do_move(svg_doc_builder& rhs);
    svg_doc_builder(const svg_doc_builder& rhs)=delete;
    svg_doc_builder& operator=(const svg_doc_builder& rhs) = delete;
   public:
    svg_doc_builder(const rectf& view_box, uint16_t dpi = 96, float font_size = 25, void*(allocator)(size_t) = ::malloc, void*(reallocator)(void*, size_t) = ::realloc, void(deallocator)(void*) = ::free);
    svg_doc_builder(svg_doc_builder&& rhs);
    svg_doc_builder& operator=(svg_doc_builder&& rhs);
    ~svg_doc_builder();
    gfx_result add_path(svg_path* path,svg_shape_info& shape_info);
    gfx_result add_polygon(const pathf& path, svg_shape_info& shape_info);
    gfx_result add_polyline(const pathf& path, svg_shape_info& shape_info);
    gfx_result add_rectangle(const rectf& bounds, svg_shape_info& shape_info);
    gfx_result add_rounded_rectangle(const rectf& bounds, sizef radiuses, svg_shape_info& shape_info);
    gfx_result add_ellipse(pointf center, sizef radiuses, svg_shape_info& shape_info);
    gfx_result to_doc(svg_doc* out_doc);
};
}  // namespace gfx
#endif