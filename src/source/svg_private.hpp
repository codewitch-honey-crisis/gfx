#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../include/gfx_core.hpp"
#include "../include/gfx_positioning.hpp"
#include "../include/gfx_pixel.hpp"
#include "../include/gfx_svg.hpp"
namespace gfx {
enum svg_flags {
    SVG_FLAGS_VISIBLE = 0x01
};

struct svg_edge final {
    float x0, y0, x1, y1;
    int direction;
    svg_edge* next;
};

struct svg_point final {
    float x, y;
    float dx, dy;
    float len;
    float dmx, dmy;
    unsigned char flags;
};

struct svg_active_edge final {
    int x, dx;
    float ey;
    int dir;
    svg_active_edge* next;
};

struct svg_mem_page final {
    constexpr static const size_t mem_page_size = 1024;
    unsigned char mem[mem_page_size];
    int size;
    svg_mem_page* next;
};

struct svg_cached_paint final {
    svg_paint_type type;
    svg_spread_type spread;
    float xform[6];
    gfx::rgba_pixel<32> colors[256];
};
typedef void (*NSVGrasterizerWriteCallback)(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* state);
typedef void (*NSVGrasterizerReadCallback)(int x, int y, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* a, void* state);

struct NSVGrasterizer {
    void* (*allocator)(size_t);
    void* (*reallocator)(void*, size_t);
    void (*deallocator)(void*);
    float px, py;

    float tessTol;
    float distTol;

    svg_edge* edges;
    int nedges;
    int cedges;

    svg_point* points;
    int npoints;
    int cpoints;

    svg_point* points2;
    int npoints2;
    int cpoints2;

    svg_active_edge* freelist;
    svg_mem_page* pages;
    svg_mem_page* curpage;

    unsigned char* scanline;
    int cscanline;

    unsigned char* bitmap;
    int width, height, stride;
    NSVGrasterizerWriteCallback writeCallback;
    void* writeCallbackState;
    NSVGrasterizerReadCallback readCallback;
    void* readCallbackState;
};

gfx::gfx_result svg_parse_to_image(gfx::stream* stream, uint16_t dpi, svg_image_info** out_image, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*));
void svg_delete_paths(svg_path* path, void(deallocator)(void*));
void svg_delete_paint(svg_paint* paint, void(deallocator)(void*));
void svg_delete_shapes(svg_shape* shape, void(deallocator)(void*));
void svg_delete_image(svg_image_info* image, void(deallocator)(void*));
inline float svg_minf(float a, float b) { return a < b ? a : b; }
inline float svg_maxf(float a, float b) { return a > b ? a : b; }
int svg_pt_in_bounds(float* pt, float* bounds);
double svg_eval_bezier(double t, double p0, double p1, double p2, double p3);
void svg_curve_bounds(float* bounds, float* curve);
// Allocated rasterizer context.
gfx::gfx_result svg_create_rasterizer(NSVGrasterizer** out_rasterizer, void*(allocator)(size_t), void*(reallocator)(void*, size_t) , void(deallocator)(void*));

// Rasterizes SVG image, returns RGBA image (non-premultiplied alpha)
//   r - pointer to rasterizer context
//   image - pointer to image to rasterize
//   tx,ty - image offset (applied after scaling)
//   scale - image scale
//   dst - pointer to destination image data, 4 bytes per pixel (RGBA)
//   w - width of the image to render
//   h - height of the image to render
//   stride - number of bytes per scaleline in the destination buffer
// void nsvgRasterize(NSVGrasterizer* r,
//				   svg_image* image, float tx, float ty, float scale,
//				   unsigned char* dst, int w, int h, int stride);
gfx::gfx_result svg_rasterize(NSVGrasterizer* r,
                   svg_image_info* image, float tx, float ty, float scale,
                   unsigned char* dst, NSVGrasterizerReadCallback readCallback, void* readCallbackState, NSVGrasterizerWriteCallback writeCallback, void* writeCallbackState, int w, int h, int stride);
// Deletes rasterizer context.
void svg_delete_rasterizer(NSVGrasterizer* r);
float svg_clampf(float val, float min, float max);
float svg_sqr(float x);
float svg_vmag(float x, float y);
float svg_vecrat(float ux, float uy, float vx, float vy);
float svg_vecang(float ux, float uy, float vx, float vy);
void svg_xform_identity(float* t);
void svg_xform_set_translation(float* t, float tx, float ty);
void svg_xform_set_scale(float* t, float sx, float sy);
void svg_xform_set_skew_x(float* t, float a);
void svg_xform_set_skew_y(float* t, float a);
void svg_xform_set_rotation(float* t, float a);
void svg_xform_multiply(float* t, const float* s);
void svg_xform_inverse(float* inv, float* t);
void svg_xform_premultiply(float* t, const float* s);
void svg_xform_point(float* dx, float* dy, float x, float y, const float* t);
void svg_xform_vec(float* dx, float* dy, float x, float y, const float* t);
void svg_get_local_bounds(float* bounds, svg_shape* shape, float* xform);
}