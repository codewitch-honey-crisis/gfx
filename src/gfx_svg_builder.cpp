#include <gfx_svg.hpp>

#include "svg_private.hpp"
#define SVG_PI (3.14159265358979323846264338327f)
#define SVG_KAPPA90 (0.5522847493f)  // Length proportional to radius of a cubic bezier handle for 90deg arcs.
namespace gfx {
void svg_path_builder::do_free() {
    if (m_deallocator != nullptr) {
        if (m_begin != nullptr) {
            m_deallocator(m_begin);
            m_begin = nullptr;
            m_size = 0;
            m_capacity = 0;
        }
    }
}
void svg_path_builder::do_copy(const svg_path_builder& rhs) {
    do_free();
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
    m_begin = (float*)m_allocator(rhs.m_size * sizeof(float));
    if (m_begin != nullptr) {
        m_size = rhs.m_size;
        m_capacity = rhs.m_size;
        memcpy(m_begin, rhs.m_begin, m_size * sizeof(float));
    }
}
void svg_path_builder::do_move(svg_path_builder& rhs) {
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
    m_begin = rhs.m_begin;
    rhs.m_begin = nullptr;
    m_capacity = rhs.m_capacity;
    rhs.m_capacity = 0;
    m_size = rhs.m_size;
    rhs.m_size = 0;
}

svg_path_builder::svg_path_builder(void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*))
    : m_allocator(allocator), m_reallocator(reallocator), m_deallocator(deallocator), m_begin(nullptr), m_size(0), m_capacity(0), m_cp{0, 0, 0, 0} {
}
svg_path_builder::svg_path_builder(const svg_path_builder& rhs) {
    do_copy(rhs);
}
svg_path_builder::~svg_path_builder() {
    do_free();
}

svg_path_builder& svg_path_builder::operator=(const svg_path_builder& rhs) {
    do_copy(rhs);
    return *this;
}
svg_path_builder::svg_path_builder(svg_path_builder&& rhs) {
    do_move(rhs);
}
svg_path_builder& svg_path_builder::operator=(svg_path_builder&& rhs) {
    do_move(rhs);
    return *this;
}
gfx_result svg_path_builder::add_point(pointf pt) {
    if (!m_begin || m_size >= m_capacity - 1) {
        m_capacity = m_capacity ? (m_capacity * 2) : 8;
        if (m_begin == nullptr) {
            m_begin = (float*)m_allocator(m_capacity * sizeof(float));
        } else {
            m_begin = (float*)m_reallocator(m_begin, m_capacity * sizeof(float));
        }
        if (!m_begin) return gfx_result::out_of_memory;
    }
    m_begin[m_size++] = pt.x;
    m_begin[m_size++] = pt.y;
    return gfx_result::success;
}
gfx_result svg_path_builder::move_to_impl(pointf pt) {
    if (m_size > 0) {
        m_begin[m_size - 2] = pt.x;
        m_begin[m_size - 1] = pt.y;
        return gfx_result::success;
    } else {
        return add_point(pt);
    }
}
gfx_result svg_path_builder::line_to_impl(pointf pt) {
    float px, py;
    float dx, dy;
    if (m_size > 1) {
        px = m_begin[m_size - 2];
        py = m_begin[m_size - 1];
        dx = pt.x - px;
        dy = pt.y - py;
        gfx_result res = add_point({px + dx / 3.0f, py + dy / 3.0f});
        if (res != gfx_result::success) {
            return res;
        }
        res = add_point({pt.x - dx / 3.0f, pt.y - dy / 3.0f});
        if (res != gfx_result::success) {
            return res;
        }
        res = add_point(pt);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
gfx_result svg_path_builder::cubic_bezier_to_impl(pointf pt, const rectf& cp) {
    if (m_size > 0) {
        gfx_result res = add_point(cp.point1());
        if (res != gfx_result::success) {
            return res;
        }
        res = add_point(cp.point2());
        if (res != gfx_result::success) {
            return res;
        }
        res = add_point(pt);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
float* svg_path_builder::begin() {
    return m_begin;
}
float* svg_path_builder::end() {
    return m_begin + m_size;
}
const float* svg_path_builder::cbegin() const {
    return m_begin;
}
const float* svg_path_builder::cend() const {
    return m_begin + m_size;
}
size_t svg_path_builder::size() const {
    return m_size;
}
void svg_path_builder::clear(bool keep_capacity) {
    m_size = 0;
    if (!keep_capacity) {
        do_free();
    }
}
size_t svg_path_builder::capacity() const {
    return m_capacity;
}
gfx_result svg_path_builder::move_to(pointf location, bool relative) {
    if (relative) {
        m_cp.x1 += location.x;
        m_cp.y1 += location.y;
    } else {
        m_cp.x1 = location.x;
        m_cp.y1 = location.y;
    }
    return move_to_impl(m_cp.point1());
}
gfx_result svg_path_builder::line_to(pointf location, bool relative) {
    if (relative) {
        m_cp.x1 += location.x;
        m_cp.y1 += location.y;
    } else {
        m_cp.x1 = location.x;
        m_cp.y1 = location.y;
    }
    return line_to_impl(m_cp.point1());
}
gfx_result svg_path_builder::cubic_bezier_to(pointf location, const rectf& control_points, bool relative) {
    float x2, y2, cx1, cy1, cx2, cy2;

    if (relative) {
        cx1 = m_cp.x1 + control_points.x1;
        cy1 = m_cp.y1 + control_points.y1;
        cx2 = m_cp.x1 + control_points.x2;
        cy2 = m_cp.y1 + control_points.y2;
        x2 = m_cp.x1 + location.x;
        y2 = m_cp.y1 + location.y;
    } else {
        cx1 = control_points.x1;
        cy1 = control_points.y1;
        cx2 = control_points.x2;
        cy2 = control_points.y2;
        x2 = location.x;
        y2 = location.y;
    }

    gfx_result res = cubic_bezier_to_impl({x2, y2}, {cx1, cy1, cx2, cy2});
    if (res != gfx_result::success) {
        return res;
    }
    m_cp.x2 = cx2;
    m_cp.y2 = cy2;
    m_cp.x1 = x2;
    m_cp.y1 = y2;
    return gfx_result::success;
}
gfx_result svg_path_builder::quad_bezier_to(pointf location, pointf control_point, bool relative) {
    float x1, y1, x2, y2, cx, cy;
    float cx1, cy1, cx2, cy2;

    x1 = m_cp.x1;
    y1 = m_cp.y1;
    if (relative) {
        cx = m_cp.x1 + control_point.x;
        cy = m_cp.y1 + control_point.y;
        x2 = m_cp.x1 + location.x;
        y2 = m_cp.y1 + location.y;
    } else {
        cx = control_point.x;
        cy = control_point.y;
        x2 = location.x;
        y2 = location.y;
    }

    // Convert to cubic bezier
    cx1 = x1 + 2.0f / 3.0f * (cx - x1);
    cy1 = y1 + 2.0f / 3.0f * (cy - y1);
    cx2 = x2 + 2.0f / 3.0f * (cx - x2);
    cy2 = y2 + 2.0f / 3.0f * (cy - y2);

    gfx_result res = cubic_bezier_to_impl({x2, y2}, {cx1, cy1, cx2, cy2});
    if (res != gfx_result::success) {
        return res;
    }

    m_cp.x2 = cx;
    m_cp.y2 = cy;
    m_cp.x1 = x2;
    m_cp.y1 = y2;
    return gfx_result::success;
}
gfx_result svg_path_builder::arc_to(pointf location, sizef radius, float x_angle, bool large_arc, svg_sweep_direction sweep_direction, bool relative) {
    // Ported from canvg (https://code.google.com/p/canvg/)
    float rx, ry, rotx;
    float x1, y1, x2, y2, cx, cy, dx, dy, d;
    float x1p, y1p, cxp, cyp, s, sa, sb;
    float ux, uy, vx, vy, a1, da;
    float x, y, tanx, tany, a, px = 0, py = 0, ptanx = 0, ptany = 0, t[6];
    float sinrx, cosrx;
    int fa, fs;
    int i, ndivs;
    float hda, kappa;

    rx = fabsf(radius.width);          // x radius
    ry = fabsf(radius.height);         // y radius
    rotx = x_angle / 180.0f * SVG_PI;  // x rotation angle
    fa = large_arc;                    // Large arc
    fs = (int)sweep_direction;         // Sweep direction
    x1 = m_cp.x1;                      // start point
    y1 = m_cp.y1;
    if (relative) {  // end point
        x2 = m_cp.x1 + location.x;
        y2 = m_cp.y1 + location.y;
    } else {
        x2 = location.x;
        y2 = location.y;
    }

    dx = x1 - x2;
    dy = y1 - y2;
    d = sqrtf(dx * dx + dy * dy);
    if (d < 1e-6f || rx < 1e-6f || ry < 1e-6f) {
        // The arc degenerates to a line
        gfx_result res = line_to_impl({x2, y2});
        if (res != gfx_result::success) {
            return res;
        }
        m_cp.x1 = x2;
        m_cp.y1 = y2;
        return gfx_result::success;
    }

    sinrx = sinf(rotx);
    cosrx = cosf(rotx);

    // Convert to center point parameterization.
    // http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes
    // 1) Compute x1', y1'
    x1p = cosrx * dx / 2.0f + sinrx * dy / 2.0f;
    y1p = -sinrx * dx / 2.0f + cosrx * dy / 2.0f;
    d = svg_sqr(x1p) / svg_sqr(rx) + svg_sqr(y1p) / svg_sqr(ry);
    if (d > 1) {
        d = sqrtf(d);
        rx *= d;
        ry *= d;
    }
    // 2) Compute cx', cy'
    s = 0.0f;
    sa = svg_sqr(rx) * svg_sqr(ry) - svg_sqr(rx) * svg_sqr(y1p) - svg_sqr(ry) * svg_sqr(x1p);
    sb = svg_sqr(rx) * svg_sqr(y1p) + svg_sqr(ry) * svg_sqr(x1p);
    if (sa < 0.0f) sa = 0.0f;
    if (sb > 0.0f)
        s = sqrtf(sa / sb);
    if (fa == fs)
        s = -s;
    cxp = s * rx * y1p / ry;
    cyp = s * -ry * x1p / rx;

    // 3) Compute cx,cy from cx',cy'
    cx = (x1 + x2) / 2.0f + cosrx * cxp - sinrx * cyp;
    cy = (y1 + y2) / 2.0f + sinrx * cxp + cosrx * cyp;

    // 4) Calculate theta1, and delta theta.
    ux = (x1p - cxp) / rx;
    uy = (y1p - cyp) / ry;
    vx = (-x1p - cxp) / rx;
    vy = (-y1p - cyp) / ry;
    a1 = svg_vecang(1.0f, 0.0f, ux, uy);  // Initial angle
    da = svg_vecang(ux, uy, vx, vy);      // Delta angle

    //	if (vecrat(ux,uy,vx,vy) <= -1.0f) da = NSVG_PI;
    //	if (vecrat(ux,uy,vx,vy) >= 1.0f) da = 0;

    if (fs == 0 && da > 0)
        da -= 2 * SVG_PI;
    else if (fs == 1 && da < 0)
        da += 2 * SVG_PI;

    // Approximate the arc using cubic spline segments.
    t[0] = cosrx;
    t[1] = sinrx;
    t[2] = -sinrx;
    t[3] = cosrx;
    t[4] = cx;
    t[5] = cy;

    // Split arc into max 90 degree segments.
    // The loop assumes an iteration per end point (including start and end), this +1.
    ndivs = (int)(fabsf(da) / (SVG_PI * 0.5f) + 1.0f);
    hda = (da / (float)ndivs) / 2.0f;
    // Fix for ticket #179: division by 0: avoid cotangens around 0 (infinite)
    if ((hda < 1e-3f) && (hda > -1e-3f))
        hda *= 0.5f;
    else
        hda = (1.0f - cosf(hda)) / sinf(hda);
    kappa = fabsf(4.0f / 3.0f * hda);
    if (da < 0.0f)
        kappa = -kappa;

    for (i = 0; i <= ndivs; i++) {
        a = a1 + da * ((float)i / (float)ndivs);
        dx = cosf(a);
        dy = sinf(a);
        svg_xform_point(&x, &y, dx * rx, dy * ry, t);                       // position
        svg_xform_vec(&tanx, &tany, -dy * rx * kappa, dx * ry * kappa, t);  // tangent
        if (i > 0) {
            gfx_result res = cubic_bezier_to_impl({x, y}, {px + ptanx, py + ptany, x - tanx, y - tany});
            if (res != gfx_result::success) {
                return res;
            }
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    m_cp.x1 = x2;
    m_cp.y1 = y2;
    return gfx_result::success;
}
gfx_result svg_path_builder::to_path(svg_path** out_path, bool closed, const svg_transform* xform) {
    if (out_path == nullptr) {
        return gfx_result::invalid_argument;
    }
    *out_path = (svg_path*)m_allocator(sizeof(svg_path));
    if (*out_path == nullptr) {
        return gfx_result::out_of_memory;
    }
    svg_path* p = *out_path;
    p->closed = closed;
    p->next = nullptr;
    p->point_count = m_size / 2;
    p->points = (float*)m_allocator(m_size * sizeof(float));
    if (p->points == nullptr) {
        m_deallocator(p);
        return gfx_result::out_of_memory;
    }
    for (size_t i = 0; i < m_size; i += 2) {
        float px = m_begin[i];
        float py = m_begin[i + 1];
        if (xform != nullptr) {
            float dx, dy;
            svg_xform_point(&dx, &dy, px, py, xform->data);
            p->points[i] = dx;
            p->points[i + 1] = dy;
        } else {
            p->points[i] = px;
            p->points[i + 1] = py;
        }
    }
    // Find bounds
    for (size_t i = 0; i < p->point_count - 1; i += 3) {
        float* curve = &p->points[i * 2];
        float bounds[4];
        svg_curve_bounds(bounds, curve);
        if (i == 0) {
            p->bounds.x1 = bounds[0];
            p->bounds.y1 = bounds[1];
            p->bounds.x2 = bounds[2];
            p->bounds.y2 = bounds[3];
        } else {
            p->bounds.x1 = svg_minf(p->bounds.x1, bounds[0]);
            p->bounds.y1 = svg_minf(p->bounds.y1, bounds[1]);
            p->bounds.x2 = svg_maxf(p->bounds.x2, bounds[2]);
            p->bounds.y2 = svg_maxf(p->bounds.y2, bounds[3]);
        }
    }
    clear(true);
    return gfx_result::success;
}
float svg_doc_builder::to_pixels(svg_coordinate c, float orig, float length) {
    switch (c.units) {
        case svg_units::user:
            return c.value;
        case svg_units::px:
            return c.value;
        case svg_units::pt:
            return c.value / 72.0f * m_dpi;
        case svg_units::pc:
            return c.value / 6.0f * m_dpi;
        case svg_units::mm:
            return c.value / 25.4f * m_dpi;
        case svg_units::cm:
            return c.value / 2.54f * m_dpi;
        case svg_units::in:
            return c.value * m_dpi;
        case svg_units::em:
            return c.value * m_font_size;
        case svg_units::ex:
            return c.value * m_font_size * 0.52f;  // x-height of Helvetica.
        case svg_units::percent:
            return orig + c.value / 100.0f * length;
        default:
            return c.value;
    }
    return c.value;
}
void svg_doc_builder::to_bounds(float* bounds) const {
    svg_shape* shape;
    shape = m_shape;
    if (shape == NULL) {
        bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0;
        return;
    }
    bounds[0] = shape->bounds.x1;
    bounds[1] = shape->bounds.y1;
    bounds[2] = shape->bounds.x2;
    bounds[3] = shape->bounds.y2;
    for (shape = shape->next; shape != NULL; shape = shape->next) {
        bounds[0] = svg_minf(bounds[0], shape->bounds.x1);
        bounds[1] = svg_minf(bounds[1], shape->bounds.y1);
        bounds[2] = svg_maxf(bounds[2], shape->bounds.x2);
        bounds[3] = svg_maxf(bounds[3], shape->bounds.y2);
    }
}
#define SVG_ALIGN_MIN 0
#define SVG_ALIGN_MID 1
#define SVG_ALIGN_MAX 2
#define SVG_ALIGN_NONE 0
#define SVG_ALIGN_MEET 1
#define SVG_ALIGN_SLICE 2

float svg_doc_builder::view_align(float content, float container, int type) {
    if (type == SVG_ALIGN_MIN)
        return 0;
    else if (type == SVG_ALIGN_MAX)
        return container - content;
    // mid
    return (container - content) * 0.5f;
}
void svg_doc_builder::scale_gradient(svg_gradient* grad, float tx, float ty, float sx, float sy) {
    float t[6];
    svg_xform_set_translation(t, tx, ty);
    svg_xform_multiply(grad->xform.data, t);

    svg_xform_set_scale(t, sx, sy);
    svg_xform_multiply(grad->xform.data, t);
}
void svg_doc_builder::scale_to_view_box(svg_units units) {
    svg_shape* shape;
    svg_path* path;
    float tx, ty, sx, sy, us, bounds[4], t[6], avgs;
    int i;
    float* pt;

    // Guess image size if not set completely.
    to_bounds(bounds);
    float view_width = m_dimensions.width;
    float view_height = m_dimensions.height;
    tx = m_view_box.x1;
    ty = m_view_box.y1;
    sx = m_dimensions.width / m_view_box.width();
    sy = m_dimensions.height / m_view_box.height();
    // Unit scaling
    us = 1.0f / to_pixels({1.0f,units}, 0.0f, 1.0f);


    // Transform
    sx *= us;
    sy *= us;
    avgs = (sx + sy) / 2.0f;
    for (shape = m_shape; shape != NULL; shape = shape->next) {
        shape->bounds.x1 = (shape->bounds.x1 + tx) * sx;
        shape->bounds.y1 = (shape->bounds.y1 + ty) * sy;
        shape->bounds.x2 = (shape->bounds.x2 + tx) * sx;
        shape->bounds.y2 = (shape->bounds.y2 + ty) * sy;
        for (path = shape->paths; path != NULL; path = path->next) {
            path->bounds.x1 = (path->bounds.x1 + tx) * sx;
            path->bounds.y1 = (path->bounds.y1 + ty) * sy;
            path->bounds.x2 = (path->bounds.x2 + tx) * sx;
            path->bounds.y2 = (path->bounds.y2 + ty) * sy;
            for (i = 0; i < path->point_count; i++) {
                pt = &path->points[i * 2];
                pt[0] = (pt[0] + tx) * sx;
                pt[1] = (pt[1] + ty) * sy;
            }
        }

        if (shape->fill.type == svg_paint_type::linear_gradient || shape->fill.type == svg_paint_type::radial_gradient) {
            scale_gradient(shape->fill.gradient, tx, ty, sx, sy);
            memcpy(t, shape->fill.gradient->xform.data, sizeof(float) * 6);
            svg_xform_inverse(shape->fill.gradient->xform.data, t);
        }
        if (shape->stroke.type == svg_paint_type::linear_gradient || shape->stroke.type == svg_paint_type::radial_gradient) {
            scale_gradient(shape->stroke.gradient, tx, ty, sx, sy);
            memcpy(t, shape->stroke.gradient->xform.data, sizeof(float) * 6);
            svg_xform_inverse(shape->stroke.gradient->xform.data, t);
        }

        shape->stroke_width *= avgs;
        shape->stroke_dash_offset *= avgs;
        for (i = 0; i < shape->stroke_dash_count; i++)
            shape->stroke_dash_array[i] *= avgs;
    }
}

svg_gradient* svg_doc_builder::create_gradient(const svg_gradient_info& info, const float* local_bounds, float* xform, svg_paint_type* paint_type) {
    const svg_gradient_stop* stops = info.stops;
    svg_gradient* grad;
    float ox, oy, sw, sh, sl;
    int nstops = info.stop_count;
    int refIter;
    *paint_type = svg_paint_type::undefined;
    size_t grad_sz = sizeof(svg_gradient) + sizeof(svg_gradient_stop) * (nstops - 1);
    grad = (svg_gradient*)m_allocator(grad_sz);
    if (grad == NULL) return NULL;
    // The shape width and height.
    grad->f = {0.0f, 0.0f};
    ox = local_bounds[0];
    oy = local_bounds[1];
    sw = local_bounds[2] - local_bounds[0];
    sh = local_bounds[3] - local_bounds[1];

    sl = sqrtf(sw * sw + sh * sh) / sqrtf(2.0f);

    if (info.type == svg_gradient_type::linear) {
        float x1, y1, x2, y2, dx, dy;
        x1 = to_pixels(info.linear.x1, ox, sw);
        y1 = to_pixels(info.linear.y1, oy, sh);
        x2 = to_pixels(info.linear.x2, ox, sw);
        y2 = to_pixels(info.linear.y2, oy, sh);
        // Calculate transform aligned to the line
        dx = x2 - x1;
        dy = y2 - y1;
        grad->xform.data[0] = dy;
        grad->xform.data[1] = -dx;
        grad->xform.data[2] = dx;
        grad->xform.data[3] = dy;
        grad->xform.data[4] = x1;
        grad->xform.data[5] = y1;
    } else {
        float cx, cy, fx, fy, r;
        cx = to_pixels(info.radial.center_x, ox, sw);
        cy = to_pixels(info.radial.center_y, oy, sh);
        fx = to_pixels(info.radial.focus_x, ox, sw);
        fy = to_pixels(info.radial.focus_y, oy, sh);
        r = to_pixels(info.radial.radius, 0, sl);
        // Calculate transform aligned to the circle
        grad->xform.data[0] = r;
        grad->xform.data[1] = 0;
        grad->xform.data[2] = 0;
        grad->xform.data[3] = r;
        grad->xform.data[4] = cx;
        grad->xform.data[5] = cy;
        grad->f.x = fx / r;
        grad->f.y = fy / r;
    }
    
    svg_xform_multiply(grad->xform.data, info.xform.data);
    svg_xform_multiply(grad->xform.data, xform);

    grad->spread = info.spread;
    memcpy(grad->stops, stops, nstops * sizeof(svg_gradient_stop));
    grad->stop_count = nstops;
    switch(info.type) {
        case svg_gradient_type::linear:
            *paint_type = svg_paint_type::linear_gradient;
            break;
            default:  // radial
                *paint_type = svg_paint_type::radial_gradient;
            break;
    }
    return grad;
}
void svg_doc_builder::add_gradient(svg_gradient_info* grad) {
    if(m_gradients==nullptr) {
        m_gradients = grad;
        m_gradients_tail = grad;
    } else {
        m_gradients_tail->next = grad;
        m_gradients_tail = grad;
    }
}
svg_shape* svg_doc_builder::create_shape(svg_shape_info& info) {
    svg_shape* result = (svg_shape*)m_allocator(sizeof(svg_shape));
    if (result == nullptr) {
        return nullptr;
    }
    memcpy(result->xform.data, info.xform.data, sizeof(float) * 6);
    result->fill.type = info.fill.type;
    result->fill_gradient = nullptr;

    switch (info.fill.type) {
        case svg_paint_type::color:
            result->fill.color = info.fill.color;
            break;
        case svg_paint_type::radial_gradient:
        case svg_paint_type::linear_gradient:
            add_gradient(info.fill.gradient);
            result->fill_gradient = info.fill.gradient;
            //result->fill.type = svg_paint_type::undefined;
            break;
        default:  // svg_paint_type::none: svg_paint_type::undefined:
            break;
    }

    result->fill_rule = info.fill_rule;
    result->flags = !info.hidden;
    result->id[0] = 0;
    result->miter_limit = info.miter_limit;
    result->next = nullptr;
    result->opacity = 1.0f;
    result->paths = nullptr;
    result->stroke.type = info.stroke.type;
    result->stroke_gradient = nullptr;
    switch (info.stroke.type) {
        case svg_paint_type::color:
            result->stroke.color = info.stroke.color;
            break;
        case svg_paint_type::radial_gradient:
        case svg_paint_type::linear_gradient:
            add_gradient(info.stroke.gradient);
            result->stroke_gradient = info.stroke.gradient;
            //result->stroke.type = svg_paint_type::undefined;
            break;
        default:  // svg_paint_type::none: svg_paint_type::undefined:
            break;
    }
    result->stroke_dash_count = info.stroke_dash_count;
    memcpy(result->stroke_dash_array, info.stroke_dash_array, sizeof(float) * info.stroke_dash_count);
    result->stroke_dash_offset = info.stroke_dash_offset;
    result->stroke_line_cap = info.stroke_line_cap;
    result->stroke_line_join = info.stroke_line_join;
    result->stroke_width = info.stroke_width;
    
    return result;
}
void svg_doc_builder::do_free_gradient_infos() {
    svg_gradient_info* grad = m_gradients;
    while (grad != nullptr) {
        svg_gradient_info* next = grad->next;
        m_deallocator(grad);
        grad = next;
    }
    m_gradients = nullptr;
    m_gradients_tail = nullptr;
}
void svg_doc_builder::do_free() {
    do_free_gradient_infos();
    svg_delete_shapes(m_shape, m_deallocator);
    m_shape = nullptr;
    m_shape_tail = nullptr;
}
void svg_doc_builder::do_move(svg_doc_builder& rhs) {
    do_free();
    m_dimensions = rhs.m_dimensions;
    m_view_box = rhs.m_view_box;
    m_dpi = rhs.m_dpi;
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
    m_shape = rhs.m_shape;
    rhs.m_shape = nullptr;
    m_shape_tail = rhs.m_shape_tail;
    rhs.m_shape_tail = nullptr;
    m_gradients = rhs.m_gradients;
    rhs.m_gradients = nullptr;
    m_gradients_tail = rhs.m_gradients_tail;
    rhs.m_gradients_tail = nullptr;
}
void svg_doc_builder::add_shape(svg_shape* shape) {
    if (m_shape == nullptr) {
        m_shape = shape;
        m_shape_tail = shape;
    } else {
        m_shape_tail->next = shape;
        m_shape_tail = shape;
    }
}
gfx_result svg_doc_builder::add_poly_impl(const pathf& path, bool closed, svg_shape_info& shape_info) {
    size_t c = path.size();
    if (c < 2) {
        return gfx_result::success;
    }
    m_builder.clear();
    const pointf* pt = path.begin();
    m_builder.move_to(*pt++);
    --c;
    while (c--) {
        m_builder.line_to(*pt++);
    }
    svg_path* spath;
    gfx_result res = m_builder.to_path(&spath, true, &shape_info.xform);
    if (res != gfx_result::success) {
        return res;
    }
    svg_shape* shape = create_shape(shape_info);
    if (shape == nullptr) {
        if (spath->points != nullptr) {
            m_deallocator(spath->points);
        }
        m_deallocator(spath);
        return gfx_result::out_of_memory;
    }
    shape->paths = spath;
    add_shape(shape);
    return gfx_result::success;
}
svg_doc_builder::svg_doc_builder(sizef dimensions, uint16_t dpi, float font_size, const rectf* view_box, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) : m_dimensions(dimensions), m_dpi(dpi), m_font_size(font_size),m_view_box(0,0,dimensions.width-1,dimensions.height-1) , m_allocator(allocator), m_reallocator(reallocator), m_deallocator(deallocator), m_builder(allocator, reallocator, deallocator), m_shape(nullptr), m_shape_tail(nullptr), m_gradients(nullptr), m_gradients_tail(nullptr) {
    if(view_box!=nullptr) {
        m_view_box = *view_box;
    }
}
svg_doc_builder::svg_doc_builder(svg_doc_builder&& rhs) {
    do_move(rhs);
}
svg_doc_builder& svg_doc_builder::operator=(svg_doc_builder&& rhs) {
    do_move(rhs);
    return *this;
}
svg_doc_builder::~svg_doc_builder() {
    do_free();
}
gfx_result svg_doc_builder::add_path(svg_path* path, svg_shape_info& shape_info) {
    if (path == nullptr) {
        return gfx_result::invalid_argument;
    }
    svg_shape* shape = create_shape(shape_info);
    if (shape == nullptr) {
        return gfx_result::out_of_memory;
    }
    shape->paths = path;
    add_shape(shape);
    return gfx_result::success;
}
gfx_result svg_doc_builder::add_polygon(const pathf& path, svg_shape_info& shape_info) {
    return add_poly_impl(path, true, shape_info);
}
gfx_result svg_doc_builder::add_polyline(const pathf& path, svg_shape_info& shape_info) {
    return add_poly_impl(path, false, shape_info);
}
gfx_result svg_doc_builder::add_rectangle(const rectf& bounds, svg_shape_info& shape_info) {
    sizef sz = bounds.dimensions();
    if (sz.width > .00001f && sz.height > .00001f) {
        m_builder.clear();
        float x = bounds.x1, y = bounds.y1, w = bounds.width(), h = bounds.height();
        rectf b = bounds.normalize();
        gfx_result res = m_builder.move_to(b.point1());
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + w, y});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + w, y + h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x, y + h});
        if (res != gfx_result::success) {
            return res;
        }
        svg_path* path;
        res = m_builder.to_path(&path, true, &shape_info.xform);
        if (res != gfx_result::success) {
            return res;
        }
        svg_shape* shape = create_shape(shape_info);
        if (shape == nullptr) {
            if (path->points != nullptr) {
                m_deallocator(path->points);
            }
            m_deallocator(path);
            return gfx_result::out_of_memory;
        }
        shape->paths = path;
        add_shape(shape);
    }
    return gfx_result::success;
}
gfx_result svg_doc_builder::add_rounded_rectangle(const rectf& bounds, sizef radiuses, svg_shape_info& shape_info) {
    sizef sz = bounds.dimensions();
    if (sz.width > .00001f && sz.height > .00001f) {
        m_builder.clear();
        float x = bounds.x1, y = bounds.y1, w = bounds.width(), h = bounds.height(), rx = radiuses.width, ry = radiuses.height;
        // Rounded rectangle
        gfx_result res = m_builder.move_to({x + rx, y});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + w - rx, y});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x + w, y + ry}, {x + w - rx * (1 - SVG_KAPPA90), y, x + w, y + ry * (1 - SVG_KAPPA90)});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + w, y + h - ry});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x + w - rx, y + h}, {x + w, y + h - ry * (1 - SVG_KAPPA90), x + w - rx * (1 - SVG_KAPPA90), y + h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + rx, y + h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x, y + h - ry}, {x + rx * (1 - SVG_KAPPA90), y + h, x, y + h - ry * (1 - SVG_KAPPA90)});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x, y + ry});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x + rx, y}, {x, y + ry * (1 - SVG_KAPPA90), x + rx * (1 - SVG_KAPPA90), y});
        if (res != gfx_result::success) {
            return res;
        }
        svg_path* path;
        res = m_builder.to_path(&path, true, &shape_info.xform);
        if (res != gfx_result::success) {
            return res;
        }
        if (res != gfx_result::success) {
            return res;
        }
        svg_shape* shape = create_shape(shape_info);
        if (shape == nullptr) {
            if (path->points != nullptr) {
                m_deallocator(path->points);
            }
            m_deallocator(path);
            return gfx_result::out_of_memory;
        }
        shape->paths = path;
        add_shape(shape);
    }
    return gfx_result::success;
}
gfx_result svg_doc_builder::add_ellipse(pointf center, sizef radiuses, svg_shape_info& shape_info) {
    if (radiuses.width > 0.0f && radiuses.height > 0.0f) {
        m_builder.clear();
        gfx_result res = m_builder.move_to({center.x + radiuses.width, center.y});
        if (res != gfx_result::success) {
            return res;
        }
        // lower right
        res = m_builder.cubic_bezier_to({center.x, center.y + radiuses.height}, {center.x + radiuses.width, center.y + radiuses.height * SVG_KAPPA90, center.x + radiuses.width * SVG_KAPPA90, center.y + radiuses.height});
        if (res != gfx_result::success) {
            return res;
        }
        // lower left
        res = m_builder.cubic_bezier_to({center.x - radiuses.width, center.y}, {center.x - radiuses.width * SVG_KAPPA90, center.y + radiuses.height, center.x - radiuses.width, center.y + radiuses.height * SVG_KAPPA90});
        if (res != gfx_result::success) {
            return res;
        }
        // upper left
        res = m_builder.cubic_bezier_to({center.x, center.y - radiuses.height}, {center.x - radiuses.width, center.y - radiuses.height * SVG_KAPPA90, center.x - radiuses.width * SVG_KAPPA90, center.y - radiuses.height});
        if (res != gfx_result::success) {
            return res;
        }
        // upper right
        res = m_builder.cubic_bezier_to({center.x + radiuses.width, center.y}, {center.x + radiuses.width * SVG_KAPPA90, center.y - radiuses.height, center.x + radiuses.width, center.y - radiuses.height * SVG_KAPPA90});
        if (res != gfx_result::success) {
            return res;
        }
        svg_path* path;
        res = m_builder.to_path(&path, true, &shape_info.xform);
        if (res != gfx_result::success) {
            return res;
        }
        if (res != gfx_result::success) {
            return res;
        }
        svg_shape* shape = create_shape(shape_info);
        if (shape == nullptr) {
            if (path->points != nullptr) {
                m_deallocator(path->points);
            }
            m_deallocator(path);
            return gfx_result::out_of_memory;
        }
        shape->paths = path;
        add_shape(shape);
    }
    return gfx_result::success;
}
gfx_result svg_doc_builder::to_doc(svg_doc* out_doc) {
    if (out_doc == nullptr) {
        return gfx_result::invalid_argument;
    }
    m_builder.clear(false);
    svg_image* img = (svg_image*)m_allocator(sizeof(svg_image));
    if (img == nullptr) {
        return gfx_result::out_of_memory;
    }
    svg_shape* shape = m_shape;
    while(shape!=nullptr) {
        if(shape->fill_gradient!=nullptr) {
            float inv[6], localBounds[4];
            svg_xform_inverse(inv, shape->xform.data);
            svg_get_local_bounds(localBounds, shape, inv);
            shape->fill.gradient = create_gradient(*shape->fill_gradient, localBounds, shape->xform.data, &shape->fill.type);
        }
        if (shape->fill.type == svg_paint_type::undefined) {
            shape->fill.type = svg_paint_type::none;
        }
        if (shape->stroke_gradient != nullptr) {
            float inv[6], localBounds[4];
            svg_xform_inverse(inv, shape->xform.data);
            svg_get_local_bounds(localBounds, shape, inv);
            shape->stroke.gradient = create_gradient(*shape->stroke_gradient, localBounds, shape->xform.data, &shape->stroke.type);
        }
        if (shape->stroke.type == svg_paint_type::undefined) {
            shape->stroke.type = svg_paint_type::none;
        }
        shape = shape->next;
    }
    scale_to_view_box(svg_units::px);
    img->shapes = m_shape;
    img->dimensions = m_dimensions;
    out_doc->m_doc_data = img;
    out_doc->m_allocator = m_allocator;
    out_doc->m_reallocator = m_reallocator;
    out_doc->m_deallocator = m_deallocator;
    m_shape = nullptr;
    m_shape_tail = nullptr;
    do_free_gradient_infos();
    return gfx_result::success;
}
void svg_gradient_builder::do_free() {
    if (m_deallocator != nullptr) {
        if (m_gradient != nullptr) {
            if(m_gradient->stops!=nullptr) {
                m_deallocator(m_gradient->stops);
            }
            m_deallocator(m_gradient);
            m_gradient = nullptr;
        }
    }
}
void svg_gradient_builder::do_copy(const svg_gradient_builder& rhs) {
    if (rhs.m_gradient == nullptr) {
        m_gradient = nullptr;
        return;
    }
    m_gradient = (svg_gradient_info*)m_allocator(sizeof(svg_gradient_info));
    if(m_gradient==nullptr) {
        return;
    }
    memcpy(m_gradient, rhs.m_gradient, sizeof(svg_gradient_info));
    size_t sz = (rhs.m_gradient->stop_count) * sizeof(svg_gradient_stop);
    m_gradient->stops = (svg_gradient_stop*)m_allocator(sz);
    if(m_gradient->stops=nullptr) {
        m_deallocator(m_gradient);
        m_gradient=nullptr;
        return;
    }
    memcpy(m_gradient->stops,rhs.m_gradient->stops,sizeof(svg_gradient_stop)*m_gradient->stop_count);
}
void svg_gradient_builder::do_move(svg_gradient_builder& rhs) {
    m_gradient = rhs.m_gradient;
    rhs.m_gradient = nullptr;
}
svg_gradient_builder::svg_gradient_builder(void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) : m_allocator(allocator), m_reallocator(reallocator), m_deallocator(deallocator), m_gradient(nullptr) {
}
svg_gradient_builder::svg_gradient_builder(const svg_gradient_builder& rhs) {
    do_copy(rhs);
}
svg_gradient_builder& svg_gradient_builder::operator=(const svg_gradient_builder& rhs) {
    do_free();
    do_copy(rhs);
    return *this;
}
svg_gradient_builder::svg_gradient_builder(svg_gradient_builder&& rhs) {
    do_move(rhs);
}
svg_gradient_builder& svg_gradient_builder::operator=(svg_gradient_builder&& rhs) {
    do_free();
    do_move(rhs);
    return *this;
}
svg_gradient_builder::~svg_gradient_builder() {
    do_free();
}
gfx_result svg_gradient_builder::add_stop(rgba_pixel<32> color, float offset) {
    if (m_gradient == nullptr) {
        m_gradient = (svg_gradient_info*)m_allocator(sizeof(svg_gradient_info));
        if (m_gradient == nullptr) {
            return gfx_result::out_of_memory;
        }
        m_gradient->stop_count = 0;
        m_gradient->next = nullptr;
        m_gradient->stops = (svg_gradient_stop*)m_allocator(sizeof(svg_gradient_stop));
        if(m_gradient->stops == nullptr) {
            m_deallocator(m_gradient);
            m_gradient = nullptr;
            return gfx_result::out_of_memory;
        }
    } else {
        m_gradient->stops = (svg_gradient_stop*)m_reallocator(m_gradient->stops, (m_gradient->stop_count+1) * sizeof(svg_gradient_stop));
        if (m_gradient == nullptr) {
            return gfx_result::out_of_memory;
        }
    }
    svg_gradient_stop& stp= m_gradient->stops[m_gradient->stop_count];
    stp.color = color;
    stp.offset = offset;
    ++m_gradient->stop_count;
    return gfx_result::success;
}
void svg_gradient_builder::clear() {
    do_free();
}
gfx_result svg_gradient_builder::to_linear_gradient(svg_gradient_info** out_gradient, svg_gradient_units units ,svg_coordinate x1, svg_coordinate y1, svg_coordinate x2, svg_coordinate y2, svg_spread_type spread, const svg_transform* xform) {
    if (out_gradient == nullptr) {
        return gfx_result::invalid_argument;
    }
    if (m_gradient == nullptr) {
        m_gradient = (svg_gradient_info*)m_allocator(sizeof(svg_gradient_info));
        if (m_gradient == nullptr) {
            return gfx_result::out_of_memory;
        }
        m_gradient->stop_count = 0;
    }
    m_gradient->type = svg_gradient_type::linear;
    m_gradient->linear.x1 = x1;
    m_gradient->linear.y1 = y1;
    m_gradient->linear.x2 = x2;
    m_gradient->linear.y2 = y2;
    m_gradient->spread = spread;
    m_gradient->units = units;
    m_gradient->next = nullptr;
    if(xform!=nullptr) {
        memcpy(m_gradient->xform.data,xform->data,sizeof(float)*6);
    }
    *out_gradient = m_gradient;
    m_gradient = nullptr;
    return gfx_result::success;
}
gfx_result svg_gradient_builder::to_radial_gradient(svg_gradient_info** out_gradient, svg_gradient_units units ,svg_coordinate center_x, svg_coordinate center_y, svg_coordinate focus_x, svg_coordinate focus_y, svg_coordinate radius, svg_spread_type spread, const svg_transform* xform) {
    if (out_gradient == nullptr) {
        return gfx_result::invalid_argument;
    }
    if (m_gradient == nullptr) {
        m_gradient = (svg_gradient_info*)m_allocator(sizeof(svg_gradient_info));
        if (m_gradient == nullptr) {
            return gfx_result::out_of_memory;
        }
        m_gradient->stop_count = 0;
    }
    m_gradient->type = svg_gradient_type::linear;
    m_gradient->radial.center_x = center_x;
    m_gradient->radial.center_y = center_y;
    m_gradient->radial.focus_x = focus_x;
    m_gradient->radial.focus_y = focus_y;
    m_gradient->radial.radius = radius;
    m_gradient->spread = spread;
    m_gradient->units = units;
    m_gradient->next = nullptr;
    if (xform != nullptr) {
        memcpy(m_gradient->xform.data, xform->data, sizeof(float) * 6);
    }
    *out_gradient = m_gradient;
    m_gradient = nullptr;
    return gfx_result::success;
}
}  // namespace gfx
