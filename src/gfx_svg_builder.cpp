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
        if(m_begin==nullptr) {
            m_begin = (float*)m_allocator(m_capacity*sizeof(float));        
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
gfx_result svg_path_builder::to_path(svg_path* out_path, bool closed, const svg_transform* xform) {
    if (out_path == nullptr) {
        return gfx_result::invalid_argument;
    }
    out_path->closed = closed;
    out_path->next = nullptr;
    out_path->point_count = m_size/2;
    out_path->points = (float*)m_allocator(m_size * sizeof(float));
    if (out_path->points == nullptr) {
        return gfx_result::out_of_memory;
    }
    for (size_t i = 0; i < m_size; i += 2) {
        float px = m_begin[i];
        float py = m_begin[i + 1];
        if (xform != nullptr) {
            float dx, dy;
            svg_xform_point(&dx, &dy, px, py, xform->data);
            out_path->points[i] = dx;
            out_path->points[i + 1] = dy;
        } else {
            out_path->points[i] = px;
            out_path->points[i + 1] = py;
        }
    }
    // Find bounds
    for (size_t i = 0; i < out_path->point_count - 1; i += 3) {
        float* curve = &out_path->points[i * 2];
        float bounds[4];
        svg_curve_bounds(bounds, curve);
        if (i == 0) {
            out_path->bounds.x1 = bounds[0];
            out_path->bounds.y1 = bounds[1];
            out_path->bounds.x2 = bounds[2];
            out_path->bounds.y2 = bounds[3];
        } else {
            out_path->bounds.x1 = svg_minf(out_path->bounds.x1, bounds[0]);
            out_path->bounds.y1 = svg_minf(out_path->bounds.y1, bounds[1]);
            out_path->bounds.x2 = svg_maxf(out_path->bounds.x2, bounds[2]);
            out_path->bounds.y2 = svg_maxf(out_path->bounds.y2, bounds[3]);
        }
    }
    clear(true);
    return gfx_result::success;
}
svg_shape* svg_doc_builder::create_shape(const svg_shape_info& info) {
    svg_shape* result = (svg_shape*)m_allocator(sizeof(svg_shape));
    if(result==nullptr) {
        return nullptr;
    }
    result->fill = info.fill;
    result->fill_gradient_id[0]=0;
    result->fill_rule = info.fill_rule;
    result->flags = !info.hidden;
    result->id[0]=0;
    result->miter_limit = info.miter_limit;
    result->next = nullptr;
    result->opacity = 1.0f;
    result->paths = nullptr;
    result->stroke = info.stroke;
    result->stroke_dash_count = info.stroke_dash_count;
    memcpy(result->stroke_dash_array,info.stroke_dash_array,sizeof(float)*info.stroke_dash_count);
    result->stroke_dash_offset = info.stroke_dash_offset;
    result->stroke_line_cap = info.stroke_line_cap;
    result->stroke_line_join = info.stroke_line_join;
    result->stroke_width = info.stroke_width;
    memcpy(result->xform.data, info.xform.data,sizeof(float)*6);
    return result;
}
void svg_doc_builder::do_free() {
    svg_delete_shapes(m_shape,m_deallocator);
    m_shape = nullptr;
    m_shape_tail = nullptr;
}
void svg_doc_builder::do_move(svg_doc_builder& rhs) {
    do_free();
    m_allocator = rhs.m_allocator;
    m_reallocator = rhs.m_reallocator;
    m_deallocator = rhs.m_deallocator;
    m_shape = rhs.m_shape;
    rhs.m_shape = nullptr;
    m_shape_tail = rhs.m_shape_tail;
    rhs.m_shape_tail = nullptr;
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
    if(c<2) {
        return gfx_result::success;
    }
    m_builder.clear();
    const pointf* pt = path.begin();
    m_builder.move_to(*pt++);
    --c;
    while(c--) {
        m_builder.line_to(*pt++);
    }
    svg_path* spath = (svg_path*)m_allocator(sizeof(svg_path));
    if (spath == nullptr) {
        return gfx_result::out_of_memory;
    }
    gfx_result res = m_builder.to_path(spath, closed, &shape_info.xform);
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
svg_doc_builder::svg_doc_builder(void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) : m_allocator(allocator),m_reallocator(reallocator),m_deallocator(deallocator),m_builder(allocator,reallocator,deallocator), m_shape(nullptr),m_shape_tail(nullptr) {

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
    if(path==nullptr) {
        return gfx_result::invalid_argument;
    }
    svg_shape* shape = create_shape(shape_info);
    if(shape==nullptr) {
        return gfx_result::out_of_memory;
    }
    shape->paths = path;
    add_shape(shape);
    return gfx_result::success;
}
gfx_result svg_doc_builder::add_polygon(const pathf& path, svg_shape_info& shape_info) {
    return add_poly_impl(path,true,shape_info);
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
        res = m_builder.line_to({x+w, y});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x+w,y+h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x,y+h});
        if (res != gfx_result::success) {
            return res;
        }
        svg_path* path = (svg_path*)m_allocator(sizeof(svg_path));
        if (path == nullptr) {
            return gfx_result::out_of_memory;
        }
        res = m_builder.to_path(path, true, &shape_info.xform);
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
        res = m_builder.cubic_bezier_to({x + w, y + ry},{x + w - rx * (1 - SVG_KAPPA90), y, x + w, y + ry * (1 - SVG_KAPPA90)});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + w, y + h - ry});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x + w - rx, y + h} ,{x + w, y + h - ry * (1 - SVG_KAPPA90), x + w - rx * (1 - SVG_KAPPA90), y + h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x + rx, y + h});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x, y + h - ry} ,{x + rx * (1 - SVG_KAPPA90), y + h, x, y + h - ry * (1 - SVG_KAPPA90)});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.line_to({x, y + ry});
        if (res != gfx_result::success) {
            return res;
        }
        res = m_builder.cubic_bezier_to({x + rx, y} ,{x, y + ry * (1 - SVG_KAPPA90), x + rx * (1 - SVG_KAPPA90), y});
        if (res != gfx_result::success) {
            return res;
        }
        svg_path* path = (svg_path*)m_allocator(sizeof(svg_path));
        if (path == nullptr) {
            return gfx_result::out_of_memory;
        }
        res = m_builder.to_path(path, true, &shape_info.xform);
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
        svg_path* path = (svg_path*)m_allocator(sizeof(svg_path));
        if(path==nullptr) {
            return gfx_result::out_of_memory;
        }
        res = m_builder.to_path(path, true, &shape_info.xform);
        if (res != gfx_result::success) {
            return res;
        }
        svg_shape* shape = create_shape(shape_info);
        if(shape==nullptr) {
            if(path->points!=nullptr) {
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
gfx_result svg_doc_builder::to_doc(sizef dimensions, svg_doc* out_doc) {
    if(out_doc==nullptr) {
        return gfx_result::invalid_argument;
    }
    m_builder.clear(false);
    svg_image* img = (svg_image*)m_allocator(sizeof(svg_image));
    if(img==nullptr) {
        return gfx_result::out_of_memory;
    }
    img->shapes = m_shape;
    img->dimensions = dimensions;
    out_doc->m_doc_data = img;
    out_doc->m_allocator = m_allocator;
    out_doc->m_reallocator = m_reallocator;
    out_doc->m_deallocator = m_deallocator;
    m_shape = nullptr;
    m_shape_tail = nullptr;
    return gfx_result::success;
    
}
}