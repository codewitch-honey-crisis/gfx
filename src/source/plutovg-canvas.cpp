#include "plutovg-private.h"
#include "plutovg-utils.h"
#include "gfx_encoding.hpp"
int plutovg_version(void)
{
    return PLUTOVG_VERSION;
}

const char* plutovg_version_string(void)
{
    return PLUTOVG_VERSION_STRING;
}
void plutovg_canvas_global_clip(plutovg_canvas_t* canvas, const plutovg_rect_t* rect) {
    canvas->clip_rect.x = plutovg_max(0,rect->x);
    canvas->clip_rect.y = plutovg_max(0,rect->y);
    float awidth = canvas->width-canvas->clip_rect.x;
    float aheight = canvas->height-canvas->clip_rect.y;
    canvas->clip_rect.w = plutovg_min(rect->w,awidth);
    canvas->clip_rect.h = plutovg_min(rect->h,aheight);
}
static void plutovg_stroke_data_reset(plutovg_stroke_data_t* stroke)
{
    plutovg_array_clear(stroke->dash.array);
    stroke->dash.offset = 0.f;
    stroke->style.width = 1.f;
    stroke->style.cap = PLUTOVG_LINE_CAP_BUTT;
    stroke->style.join = PLUTOVG_LINE_JOIN_MITER;
    stroke->style.miter_limit = 10.f;
}

static bool plutovg_stroke_data_copy(plutovg_stroke_data_t* stroke, const plutovg_stroke_data_t* source)
{
    plutovg_array_clear(stroke->dash.array);
    if(!plutovg_array_append(stroke->dash.array, source->dash.array)) {
        return false;
    }
    stroke->dash.offset = source->dash.offset;
    stroke->style.width = source->style.width;
    stroke->style.cap = source->style.cap;
    stroke->style.join = source->style.join;
    stroke->style.miter_limit = source->style.miter_limit;
    return true;
}

static void plutovg_state_reset(plutovg_state_t* state,void(*deallocator)(void*))
{
    plutovg_paint_destroy(state->paint,deallocator);
    state->color.a = 255;
    state->color.r = 0;
    state->color.g = 0;
    state->color.b = 0;
    state->matrix=::gfx::matrix::create_identity();
    plutovg_stroke_data_reset(&state->stroke);
    plutovg_span_buffer_reset(&state->clip_spans);
    plutovg_font_face_destroy(state->font_face);
    state->paint = NULL;
    state->font_face = NULL;
    state->font_size = 12.f;
    state->op = PLUTOVG_OPERATOR_SRC_OVER;
    state->winding = PLUTOVG_FILL_RULE_NON_ZERO;
    state->clipping = false;
    state->opacity = 1.f;
}

static void plutovg_state_copy(plutovg_state_t* state, const plutovg_state_t* source)
{
    plutovg_stroke_data_copy(&state->stroke, &source->stroke);
    plutovg_span_buffer_copy(&state->clip_spans, &source->clip_spans);
    state->paint = plutovg_paint_reference(source->paint);
    state->font_face = plutovg_font_face_reference(source->font_face);
    state->matrix = source->matrix;
    state->font_size = source->font_size;
    state->op = source->op;
    state->winding = source->winding;
    state->clipping = source->clipping;
    state->opacity = source->opacity;
}

static plutovg_state_t* plutovg_state_create(void*(*allocator)(size_t),void(*deallocator)(void*))
{
    plutovg_state_t* state = (plutovg_state_t*)allocator(sizeof(plutovg_state_t));
    if(state==nullptr) {
        return nullptr;
    }
    //memset(state, 0, sizeof(plutovg_state_t));
    state->font_face = nullptr;
    state->next = nullptr;
    state->op = plutovg_operator_t::PLUTOVG_OPERATOR_SRC;
    state->clipping = false;
    state->opacity = 0.f;
    state->paint = nullptr;
    state->winding =PLUTOVG_FILL_RULE_NON_ZERO;
    state->clip_spans.spans.data = nullptr;
    state->clip_spans.spans.size = 0;
    state->clip_spans.spans.capacity = 0;
    state->stroke.dash.array.data = nullptr;
    state->stroke.dash.array.size = 0;
    state->stroke.dash.array.capacity = 0;
    plutovg_state_reset(state,deallocator);
    return state;
}

static void plutovg_state_destroy(plutovg_state_t* state,void(*deallocator)(void*))
{
    plutovg_paint_destroy(state->paint,deallocator);
    plutovg_array_destroy(state->stroke.dash.array);
    //plutovg_span_buffer_destroy(&state->clip_spans);
    //deallocator(state);
}

plutovg_canvas* plutovg_canvas_create(int width, int height, void*(*allocator)(size_t), void*(*reallocator)(void*,size_t), void(*deallocator)(void*))
{
   plutovg_canvas_t* out_result = (plutovg_canvas_t*)allocator(sizeof(plutovg_canvas_t));
   if(out_result==nullptr) {
        return nullptr;
   }
    out_result->ref_count = 1;
    //out_result->surface = plutovg_surface_reference(surface);
    out_result->width = width;
    out_result->height = height;
    out_result->path = plutovg_path_create(allocator,reallocator,deallocator);
    out_result->state = plutovg_state_create(allocator,deallocator);
    out_result->freed_state = NULL;
    out_result->clip_rect = PLUTOVG_MAKE_RECT(0, 0, width, height);
    out_result->read_callback = NULL;
    out_result->write_callback = NULL;
    out_result->direct = NULL;
    out_result->direct_on_read = NULL;
    out_result->direct_on_write = NULL;
    out_result->direct_width = 0;
    out_result->direct_height = 0;
    plutovg_span_buffer_init(&out_result->clip_spans,allocator,reallocator,deallocator);
    plutovg_span_buffer_init(&out_result->fill_spans,allocator,reallocator,deallocator);
    out_result->allocator = allocator;
    out_result->reallocator = reallocator;
    out_result->deallocator = deallocator;
    return out_result;
}
void plutovg_canvas_set_callbacks(plutovg_canvas_t* canvas,plutovg_write_callback_t write_callback,plutovg_read_callback_t read_callback, void* state) {
    canvas->write_callback = write_callback;
    canvas->read_callback = read_callback;
    canvas->callback_state = state;
}
::gfx::blt_span* plutovg_canvas_get_direct(plutovg_canvas_t* canvas) {
    return canvas->direct;
}
::gfx::on_direct_read_callback_type plutovg_canvas_get_direct_on_read(plutovg_canvas_t* canvas) {
    return canvas->direct_on_read;
}
::gfx::on_direct_write_callback_type plutovg_canvas_get_direct_on_write(plutovg_canvas_t* canvas) {
    return canvas->direct_on_write;
}

int plutovg_canvas_get_direct_width(plutovg_canvas_t* canvas) {
    return canvas->direct_width;
}

int plutovg_canvas_get_direct_height(plutovg_canvas_t* canvas) {
    return canvas->direct_height;
}
int plutovg_canvas_get_direct_offset_x(plutovg_canvas_t* canvas) {
    return canvas->direct_offset_x;
}
int plutovg_canvas_get_direct_offset_y(plutovg_canvas_t* canvas) {
    return canvas->direct_offset_y;
}

void plutovg_canvas_set_direct(plutovg_canvas_t* canvas, ::gfx::blt_span* direct, int offset_x, int offset_y, int width, int height, ::gfx::on_direct_read_callback_type on_read,::gfx::on_direct_write_callback_type on_write) {
    canvas->direct = direct;
    canvas->direct_on_read = on_read;
    canvas->direct_on_write = on_write;
    canvas->direct_offset_x = offset_x;
    canvas->direct_offset_y = offset_y;
    canvas->direct_width = (uint16_t)width;
    canvas->direct_height = (uint16_t)height;
}

plutovg_canvas_t* plutovg_canvas_reference(plutovg_canvas_t* canvas)
{
    if(canvas == NULL)
        return NULL;
    ++canvas->ref_count;
    return canvas;
}

void plutovg_canvas_destroy(plutovg_canvas_t* canvas)
{
    if(canvas == NULL)
        return;
    if(--canvas->ref_count == 0) {
        while(canvas->state) {
            plutovg_state_t* state = canvas->state;
            canvas->state = state->next;
            plutovg_state_destroy(state,canvas->deallocator);
        }

        while(canvas->freed_state) {
            plutovg_state_t* state = canvas->freed_state;
            canvas->freed_state = state->next;
            plutovg_state_destroy(state,canvas->deallocator);
        }

        plutovg_span_buffer_destroy(&canvas->fill_spans);
        plutovg_span_buffer_destroy(&canvas->clip_spans);
        //plutovg_surface_destroy(canvas->surface);
        plutovg_path_destroy(canvas->path,canvas->deallocator);
        canvas->deallocator(canvas);
    }
}

int plutovg_canvas_get_reference_count(const plutovg_canvas_t* canvas)
{
    if(canvas == NULL)
        return 0;
    return canvas->ref_count;
}


bool plutovg_canvas_save(plutovg_canvas_t* canvas)
{
    plutovg_state_t* new_state = canvas->freed_state;
    if(new_state == NULL)
        new_state = plutovg_state_create(canvas->allocator,canvas->deallocator);
    else
        canvas->freed_state = new_state->next;
    if(new_state==nullptr) {
        return false;
    }
    plutovg_state_copy(new_state, canvas->state);
    new_state->next = canvas->state;
    canvas->state = new_state;
    return true;
}

void plutovg_canvas_restore(plutovg_canvas_t* canvas)
{
    if(canvas->state->next == NULL)
        return;
    plutovg_state_t* old_state = canvas->state;
    canvas->state = old_state->next;
    plutovg_state_reset(old_state,canvas->deallocator);
    old_state->next = canvas->freed_state;
    canvas->freed_state = old_state;
}

bool plutovg_canvas_set_rgb(plutovg_canvas_t* canvas, float r, float g, float b)
{
    return plutovg_canvas_set_rgba(canvas, r, g, b, 1.0);
}

bool plutovg_canvas_set_rgba(plutovg_canvas_t* canvas, float r, float g, float b, float a)
{
    plutovg_color_init_rgba(&canvas->state->color, r, g, b, a);
    return plutovg_canvas_set_paint(canvas, NULL);
}

bool plutovg_canvas_set_color(plutovg_canvas_t* canvas, const plutovg_color_t* color)
{
    return plutovg_canvas_set_rgba(canvas, color->r, color->g, color->b, color->a);
}

bool plutovg_canvas_set_linear_gradient(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2, plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix)
{
    plutovg_paint_t* paint = plutovg_paint_create_linear_gradient(x1, y1, x2, y2, spread, stops, nstops, matrix,canvas->allocator);
    if(paint==nullptr) {
        return false;
    }
    if(!plutovg_canvas_set_paint(canvas, paint)) {
        plutovg_paint_destroy(paint,canvas->deallocator);
        return false;
    }
    plutovg_paint_destroy(paint,canvas->deallocator);
    return true;
}

bool plutovg_canvas_set_radial_gradient(plutovg_canvas_t* canvas, float cx, float cy, float cr, float fx, float fy, float fr, plutovg_spread_method_t spread, const plutovg_gradient_stop_t* stops, int nstops, const ::gfx::matrix* matrix)
{
    plutovg_paint_t* paint = plutovg_paint_create_radial_gradient(cx, cy, cr, fx, fy, fr, spread, stops, nstops, matrix,canvas->allocator);
    if(paint==nullptr) {
        return false;
    }
    if(!plutovg_canvas_set_paint(canvas, paint)) {
        plutovg_paint_destroy(paint,canvas->deallocator);
        return false;
    }
    plutovg_paint_destroy(paint,canvas->deallocator);
    return true;
}
//PLUTOVG_API void plutovg_canvas_set_texture(plutovg_canvas_t* canvas, int width, int height,const ::gfx::canvas_on_read_callback_type* callback, void* callback_state, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix);
//PLUTOVG_API void plutovg_canvas_set_texture_direct(plutovg_canvas_t* canvas, int width, int height,const ::gfx::const_blt_span* direct, const ::gfx::canvas_on_direct_read_callback_type* direct_callback, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix);

bool plutovg_canvas_set_texture(plutovg_canvas_t* canvas, int width, int height,::gfx::vector_on_read_callback_type callback, void* callback_state, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix)
{
    plutovg_paint_t* paint = plutovg_paint_create_texture(width,height,callback,callback_state, type, opacity, matrix,canvas->allocator);
    if(paint==nullptr) {
        return false;
    }
    if(!plutovg_canvas_set_paint(canvas, paint)) {
        plutovg_paint_destroy(paint,canvas->deallocator );
        return false;
    }
    plutovg_paint_destroy(paint,canvas->deallocator);
    return true;
}
bool plutovg_canvas_set_texture_direct(plutovg_canvas_t* canvas, int width, int height,const ::gfx::const_blt_span* direct, const ::gfx::on_direct_read_callback_type direct_callback, plutovg_texture_type_t type, float opacity, const ::gfx::matrix* matrix)
{
    plutovg_paint_t* paint = plutovg_paint_create_texture_direct(width,height,direct, direct_callback, type, opacity, matrix,canvas->allocator);
    if(paint==nullptr) {
        return false;
    }
    if(!plutovg_canvas_set_paint(canvas, paint)) {
        plutovg_paint_destroy(paint,canvas->deallocator);    
        return false;
    }
    plutovg_paint_destroy(paint,canvas->deallocator);
    return true;
}
bool plutovg_canvas_set_paint(plutovg_canvas_t* canvas, plutovg_paint_t* paint)
{
    paint = plutovg_paint_reference(paint);
    plutovg_paint_destroy(canvas->state->paint,canvas->deallocator);
    canvas->state->paint = paint;
    return true;
}

plutovg_paint_t* plutovg_canvas_get_paint(const plutovg_canvas_t* canvas, plutovg_color_t* color)
{
    if(color)
        *color = canvas->state->color;
    return canvas->state->paint;
}

void plutovg_canvas_set_font(plutovg_canvas_t* canvas, plutovg_font_face_t* face, float size)
{
    plutovg_canvas_set_font_face(canvas, face);
    plutovg_canvas_set_font_size(canvas, size);
}

void plutovg_canvas_set_font_face(plutovg_canvas_t* canvas, plutovg_font_face_t* face)
{
    face = plutovg_font_face_reference(face);
    plutovg_font_face_destroy(canvas->state->font_face);
    canvas->state->font_face = face;
}

plutovg_font_face_t* plutovg_canvas_get_font_face(const plutovg_canvas_t* canvas)
{
    return canvas->state->font_face;
}

void plutovg_canvas_set_font_size(plutovg_canvas_t* canvas, float size)
{
    canvas->state->font_size = size;
}

float plutovg_canvas_get_font_size(const plutovg_canvas_t* canvas)
{
    return canvas->state->font_size;
}

void plutovg_canvas_set_fill_rule(plutovg_canvas_t* canvas, plutovg_fill_rule_t winding)
{
    canvas->state->winding = winding;
}

plutovg_fill_rule_t plutovg_canvas_get_fill_rule(const plutovg_canvas_t* canvas)
{
    return canvas->state->winding;
}

void plutovg_canvas_set_operator(plutovg_canvas_t* canvas, plutovg_operator_t op)
{
    canvas->state->op = op;
}

plutovg_operator_t plutovg_canvas_get_operator(const plutovg_canvas_t* canvas)
{
    return canvas->state->op;
}

void plutovg_canvas_set_opacity(plutovg_canvas_t* canvas, float opacity)
{
    canvas->state->opacity = plutovg_clamp(opacity, 0.f, 1.f);
}

float plutovg_canvas_get_opacity(const plutovg_canvas_t* canvas)
{
    return canvas->state->opacity;
}

void plutovg_canvas_set_line_width(plutovg_canvas_t* canvas, float line_width)
{
    canvas->state->stroke.style.width = line_width;
}

float plutovg_canvas_get_line_width(const plutovg_canvas_t* canvas)
{
    return canvas->state->stroke.style.width;
}

void plutovg_canvas_set_line_cap(plutovg_canvas_t* canvas, plutovg_line_cap_t line_cap)
{
    canvas->state->stroke.style.cap = line_cap;
}

plutovg_line_cap_t plutovg_canvas_get_line_cap(const plutovg_canvas_t* canvas)
{
    return canvas->state->stroke.style.cap;
}

void plutovg_canvas_set_line_join(plutovg_canvas_t* canvas, plutovg_line_join_t line_join)
{
    canvas->state->stroke.style.join = line_join;
}

plutovg_line_join_t plutovg_canvas_get_line_join(const plutovg_canvas_t* canvas)
{
    return canvas->state->stroke.style.join;
}

void plutovg_canvas_set_miter_limit(plutovg_canvas_t* canvas, float miter_limit)
{
    canvas->state->stroke.style.miter_limit = miter_limit;
}

float plutovg_canvas_get_miter_limit(const plutovg_canvas_t* canvas)
{
    return canvas->state->stroke.style.miter_limit;
}

void plutovg_canvas_set_dash(plutovg_canvas_t* canvas, float offset, const float* dashes, int ndashes)
{
    plutovg_canvas_set_dash_offset(canvas, offset);
    plutovg_canvas_set_dash_array(canvas, dashes, ndashes);
}

void plutovg_canvas_set_dash_offset(plutovg_canvas_t* canvas, float offset)
{
    canvas->state->stroke.dash.offset = offset;
}

float plutovg_canvas_get_dash_offset(const plutovg_canvas_t* canvas)
{
    return canvas->state->stroke.dash.offset;
}

bool plutovg_canvas_set_dash_array(plutovg_canvas_t* canvas, const float* dashes, int ndashes)
{
    plutovg_array_clear(canvas->state->stroke.dash.array);
    if(dashes && ndashes > 0) {
        return plutovg_array_append_data(canvas->state->stroke.dash.array, dashes, ndashes);
    }
    return true;
}

int plutovg_canvas_get_dash_array(const plutovg_canvas_t* canvas, const float** dashes)
{
    if(dashes)
        *dashes = canvas->state->stroke.dash.array.data;
    return canvas->state->stroke.dash.array.size;
}

void plutovg_canvas_translate(plutovg_canvas_t* canvas, float tx, float ty)
{
    canvas->state->matrix = canvas->state->matrix * canvas->state->matrix.create_translate( tx, ty);
}

void plutovg_canvas_scale(plutovg_canvas_t* canvas, float sx, float sy)
{
    canvas->state->matrix = canvas->state->matrix * canvas->state->matrix.create_scale( sx, sy);
}

void plutovg_canvas_shear(plutovg_canvas_t* canvas, float shx, float shy)
{
    canvas->state->matrix = canvas->state->matrix * canvas->state->matrix.create_skew( shx, shy);
}

void plutovg_canvas_rotate(plutovg_canvas_t* canvas, float angle)
{
    canvas->state->matrix = canvas->state->matrix * canvas->state->matrix.create_rotate(angle);
}

void plutovg_canvas_transform(plutovg_canvas_t* canvas, const ::gfx::matrix* matrix)
{
    canvas->state->matrix = canvas->state->matrix * *matrix;
}

void plutovg_canvas_reset_matrix(plutovg_canvas_t* canvas)
{
    canvas->state->matrix = ::gfx::matrix::create_identity();
}

void plutovg_canvas_set_matrix(plutovg_canvas_t* canvas, const ::gfx::matrix* matrix)
{
    canvas->state->matrix = matrix ? *matrix : ::gfx::matrix::create_identity();
}

void plutovg_canvas_get_matrix(const plutovg_canvas_t* canvas, ::gfx::matrix* matrix)
{
    *matrix = canvas->state->matrix;
}

void plutovg_canvas_map(const plutovg_canvas_t* canvas, float x, float y, float* xx, float* yy)
{
    canvas->state->matrix.map(x, y, xx, yy);
}

void plutovg_canvas_map_point(const plutovg_canvas_t* canvas, const ::gfx::pointf* src, ::gfx::pointf* dst)
{
    canvas->state->matrix.map(src->x, src->y, &dst->x,&dst->y);
}

void plutovg_canvas_map_rect(const plutovg_canvas_t* canvas, const plutovg_rect_t* src, plutovg_rect_t* dst)
{
    ::gfx::rectf r(src->x,src->y,src->w+src->x-1,src->h+src->y-1);
    canvas->state->matrix.map_rect(r,&r);
    dst->x = r.x1;
    dst->y = r.y2;
    dst->w = r.width();
    dst->h = r.height();
}

bool plutovg_canvas_move_to(plutovg_canvas_t* canvas, float x, float y)
{
    return plutovg_path_move_to(canvas->path, x, y);
}

bool plutovg_canvas_line_to(plutovg_canvas_t* canvas, float x, float y)
{
    return plutovg_path_line_to(canvas->path, x, y);
}

bool plutovg_canvas_quad_to(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2)
{
    return plutovg_path_quad_to(canvas->path, x1, y1, x2, y2);
}

bool plutovg_canvas_cubic_to(plutovg_canvas_t* canvas, float x1, float y1, float x2, float y2, float x3, float y3)
{
    return plutovg_path_cubic_to(canvas->path, x1, y1, x2, y2, x3, y3);
}

bool plutovg_canvas_arc_to(plutovg_canvas_t* canvas, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y)
{
    return plutovg_path_arc_to(canvas->path, rx, ry, angle, large_arc_flag, sweep_flag, x, y);
}

bool plutovg_canvas_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h)
{
    return plutovg_path_add_rect(canvas->path, x, y, w, h);
}

bool plutovg_canvas_round_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h, float rx, float ry)
{
    return plutovg_path_add_round_rect(canvas->path, x, y, w, h, rx, ry);
}

bool plutovg_canvas_ellipse(plutovg_canvas_t* canvas, float cx, float cy, float rx, float ry)
{
    return plutovg_path_add_ellipse(canvas->path, cx, cy, rx, ry);
}

bool plutovg_canvas_circle(plutovg_canvas_t* canvas, float cx, float cy, float r)
{
    return plutovg_path_add_circle(canvas->path, cx, cy, r);
}

bool plutovg_canvas_arc(plutovg_canvas_t* canvas, float cx, float cy, float r, float a0, float a1, bool ccw)
{
    return plutovg_path_add_arc(canvas->path, cx, cy, r, a0, a1, ccw);
}

bool plutovg_canvas_add_path(plutovg_canvas_t* canvas, const plutovg_path_t* path)
{
    return plutovg_path_add_path(canvas->path, path, NULL);
}

void plutovg_canvas_new_path(plutovg_canvas_t* canvas)
{
    plutovg_path_reset(canvas->path);
}

bool plutovg_canvas_close_path(plutovg_canvas_t* canvas)
{
    return plutovg_path_close(canvas->path);
}

void plutovg_canvas_get_current_point(const plutovg_canvas_t* canvas, float* x, float* y)
{
    plutovg_path_get_current_point(canvas->path, x, y);
}

plutovg_path_t* plutovg_canvas_get_path(const plutovg_canvas_t* canvas)
{
    return canvas->path;
}

bool plutovg_canvas_fill_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents)
{
    float f= plutovg_path_extents(canvas->path, extents, true);
    if(f!=f) {
        return false;
    }
    plutovg_canvas_map_rect(canvas, extents, extents);
    return true;
}

bool plutovg_canvas_stroke_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents)
{
    plutovg_stroke_data_t* stroke = &canvas->state->stroke;
    float cap_limit = stroke->style.width / 2.f;
    if(stroke->style.cap == PLUTOVG_LINE_CAP_SQUARE)
        cap_limit *= PLUTOVG_SQRT2;
    float join_limit = stroke->style.width / 2.f;
    if(stroke->style.join == PLUTOVG_LINE_JOIN_MITER) {
        join_limit *= stroke->style.miter_limit;
    }

    float delta = plutovg_max(cap_limit, join_limit);
    float f = plutovg_path_extents(canvas->path, extents, true);
    if(f!=f) {
        return false;
    }
    extents->x -= delta;
    extents->y -= delta;
    extents->w += delta * 2.f;
    extents->h += delta * 2.f;
    plutovg_canvas_map_rect(canvas, extents, extents);
    return true;
}

void plutovg_canvas_clip_extents(const plutovg_canvas_t* canvas, plutovg_rect_t* extents)
{
    if(canvas->state->clipping) {
        plutovg_span_buffer_extents(&canvas->state->clip_spans, extents);
        
    } else {
        extents->x = canvas->clip_rect.x;
        extents->y = canvas->clip_rect.y;
        extents->w = canvas->clip_rect.w;
        extents->h = canvas->clip_rect.h;
    }
    
}

bool plutovg_canvas_fill(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    if(!plutovg_canvas_fill_preserve(canvas,allocator,reallocator,deallocator)) {
        return false;
    }
    plutovg_canvas_new_path(canvas);
    return true;
}

bool plutovg_canvas_stroke(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    if(!plutovg_canvas_stroke_preserve(canvas,allocator,reallocator,deallocator)) {
        return false;
    }
    plutovg_canvas_new_path(canvas);
    return true;
}

bool plutovg_canvas_clip(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    if(!plutovg_canvas_clip_preserve(canvas,allocator,reallocator,deallocator)) {
        return false;
    }
    plutovg_canvas_new_path(canvas);
    return true;
}

bool plutovg_canvas_paint(plutovg_canvas_t* canvas)
{
    plutovg_state_t* state = canvas->state;
    if(state->clipping) {
        return plutovg_blend(canvas, &state->clip_spans);
    } else {
        if(!plutovg_span_buffer_init_rect(&canvas->clip_spans, 0, 0, canvas->width, canvas->height)) {
            return false;
        }
        return plutovg_blend(canvas, &canvas->clip_spans);
    }
}

bool plutovg_canvas_fill_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_state_t* state = canvas->state;
    if(!plutovg_rasterize(&canvas->fill_spans, canvas->path, &state->matrix, &canvas->clip_rect, NULL, state->winding,allocator,reallocator,deallocator )) {
        return false;
    }
    if(state->clipping) {
        
        if(!plutovg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &state->clip_spans)) {
            return false;
        }
        return plutovg_blend(canvas, &canvas->clip_spans);

    } else {
        return plutovg_blend(canvas, &canvas->fill_spans);
    }
}

bool plutovg_canvas_stroke_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_state_t* state = canvas->state;
    if(!plutovg_rasterize(&canvas->fill_spans, canvas->path, &state->matrix, &canvas->clip_rect, &state->stroke, PLUTOVG_FILL_RULE_NON_ZERO,allocator,reallocator,deallocator)) {
        return false;
    }
    if(state->clipping) {
        if(!plutovg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &state->clip_spans)) {
            return false;
        }
        return plutovg_blend(canvas, &canvas->clip_spans);
    } else {
        return plutovg_blend(canvas, &canvas->fill_spans);
    }
}

bool plutovg_canvas_clip_preserve(plutovg_canvas_t* canvas,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_state_t* state = canvas->state;
    if(state->clipping) {
        if(!plutovg_rasterize(&canvas->fill_spans, canvas->path, &state->matrix, &canvas->clip_rect, NULL, state->winding,allocator,reallocator,deallocator)) {
            return false;
        }
        if(!plutovg_span_buffer_intersect(&canvas->clip_spans, &canvas->fill_spans, &state->clip_spans)) {
            return false;
        }
        if(!plutovg_span_buffer_copy(&state->clip_spans, &canvas->clip_spans)) {
            return false;
        }
    } else {
        if(!plutovg_rasterize(&state->clip_spans, canvas->path, &state->matrix, &canvas->clip_rect, NULL, state->winding,allocator,reallocator,deallocator)) {
            return false;
        }
        state->clipping = true;
    }
    return true;
}

bool plutovg_canvas_fill_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    plutovg_canvas_rect(canvas, x, y, w, h);
    if(!plutovg_canvas_fill(canvas,allocator,reallocator,deallocator)) {
        return false;
    }
    return true;
}

bool plutovg_canvas_fill_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    if(!plutovg_canvas_add_path(canvas, path)) {
        return false;
    }
    return plutovg_canvas_fill(canvas,allocator,reallocator,deallocator);
}

bool plutovg_canvas_stroke_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    if(!plutovg_canvas_rect(canvas, x, y, w, h)) {
        return false;
    }
    return plutovg_canvas_stroke(canvas,allocator,reallocator,deallocator);
}

bool plutovg_canvas_stroke_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    if(!plutovg_canvas_add_path(canvas, path)) {
        return false;
    }
    return plutovg_canvas_stroke(canvas,allocator,reallocator,deallocator);
}

bool plutovg_canvas_clip_rect(plutovg_canvas_t* canvas, float x, float y, float w, float h,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    if(!plutovg_canvas_rect(canvas, x, y, w, h)) {
        return false;
    }
    return plutovg_canvas_clip(canvas,allocator,reallocator,deallocator);
}

bool plutovg_canvas_clip_path(plutovg_canvas_t* canvas, const plutovg_path_t* path,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    if(!plutovg_canvas_add_path(canvas, path)) {
        return false;
    }
    return plutovg_canvas_clip(canvas,allocator,reallocator,deallocator);
}

float plutovg_canvas_add_glyph(plutovg_canvas_t* canvas, plutovg_codepoint_t codepoint, float x, float y)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f)
        return plutovg_font_face_get_glyph_path(state->font_face, state->font_size, x, y, codepoint, canvas->path);
    return 0.f;
}

float plutovg_canvas_add_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face == NULL || state->font_size <= 0.f)
        return 0.f;
    float advance_width = 0.f;
    int32_t cp;
    const uint8_t* data = (const uint8_t*)text;
    while(length) {
        size_t l = length;
        if(::gfx::gfx_result::success!=encoding->to_utf32((::gfx::text_handle)data,&cp,&l)) {
            return NAN;
        }
        data+=l;
        length-=l;
        advance_width += plutovg_font_face_get_glyph_path(state->font_face, state->font_size, x + advance_width, y,(plutovg_codepoint_t)cp, canvas->path);
    }
    return advance_width;
}

float plutovg_canvas_fill_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    float advance_width = plutovg_canvas_add_text(canvas, text, length, encoding, x, y);
    plutovg_canvas_fill(canvas,allocator,reallocator,deallocator);
    return advance_width;
}

float plutovg_canvas_stroke_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    float advance_width = plutovg_canvas_add_text(canvas, text, length, encoding, x, y);
    plutovg_canvas_stroke(canvas,allocator,reallocator,deallocator);
    return advance_width;
}

float plutovg_canvas_clip_text(plutovg_canvas_t* canvas, const ::gfx::text_handle text, size_t length, const ::gfx::text_encoder* encoding, float x, float y,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*))
{
    plutovg_canvas_new_path(canvas);
    float advance_width = plutovg_canvas_add_text(canvas, text, length, encoding, x, y);
    plutovg_canvas_clip(canvas,allocator,reallocator,deallocator);
    return advance_width;
}

void plutovg_canvas_font_metrics(plutovg_canvas_t* canvas, float* ascent, float* descent, float* line_gap, plutovg_rect_t* extents)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        plutovg_font_face_get_metrics(state->font_face, state->font_size, ascent, descent, line_gap, extents);
        return;
    }

    if(ascent) *ascent = 0.f;
    if(descent) *descent = 0.f;
    if(line_gap) *line_gap = 0.f;
    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }
}

void plutovg_canvas_glyph_metrics(plutovg_canvas_t* canvas, plutovg_codepoint_t codepoint, float* advance_width, float* left_side_bearing, plutovg_rect_t* extents)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        plutovg_font_face_get_glyph_metrics(state->font_face, state->font_size, codepoint, advance_width, left_side_bearing, extents);
        return;
    }

    if(advance_width) *advance_width = 0.f;
    if(left_side_bearing) *left_side_bearing = 0.f;
    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }
}

float plutovg_canvas_text_extents(plutovg_canvas_t* canvas, const void* text, int length, plutovg_text_encoding_t encoding, plutovg_rect_t* extents)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face && state->font_size > 0.f) {
        return plutovg_font_face_text_extents(state->font_face, state->font_size, text, length, encoding, extents);
    }

    if(extents) {
        extents->x = 0.f;
        extents->y = 0.f;
        extents->w = 0.f;
        extents->h = 0.f;
    }

    return 0.f;
}
