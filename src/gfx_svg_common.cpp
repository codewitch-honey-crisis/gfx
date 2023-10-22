#include <gfx_svg.hpp>
#include "svg_private.hpp"
#define SVG_EPSILON (1e-12)
#define SVG_PI (3.14159265358979323846264338327f)
namespace gfx {
svg_shape_info::svg_shape_info() : fill({svg_paint_type::none, rgba_pixel<32>(0, 0, 0, 255)}),
                                   stroke({svg_paint_type::color, rgba_pixel<32>(0, 0, 0, 255)}),
                                   stroke_width(1.0f),
                                   stroke_dash_offset(0.0f),
                                   stroke_dash_array(),
                                   stroke_dash_count(0),
                                   stroke_line_join(svg_line_join::miter),
                                   stroke_line_cap(svg_line_cap::butt),
                                   miter_limit(1.0f),
                                   fill_rule(svg_fill_rule::non_zero),
                                   hidden(false) {
    xform.identity();
}
void svg_delete_paths(svg_path* path, void(deallocator)(void*) ) {
    while (path) {
        svg_path* next = path->next;
        if (path->points != NULL)
            deallocator(path->points);
        deallocator(path);
        path = next;
    }
}

void svg_delete_paint(svg_paint* paint, void(deallocator)(void*)) {
    if (paint->type == svg_paint_type::linear_gradient || paint->type == svg_paint_type::radial_gradient) {
        deallocator(paint->gradient);
        paint->gradient = nullptr;
    }
}
void svg_delete_shapes(svg_shape* the_shape, void(deallocator)(void*)) {
    svg_shape *snext, *shape=the_shape;
    while (shape != NULL) {
        snext = shape->next;
        svg_delete_paths(shape->paths, deallocator);
        svg_delete_paint(&shape->fill, deallocator);
        svg_delete_paint(&shape->stroke, deallocator);
        deallocator(shape);
        shape = snext;
    }
}
void svg_delete_image(svg_image* image, void(deallocator)(void*)) {
    if (image == NULL) return;
    svg_delete_shapes(image->shapes,deallocator);
    deallocator(image);
}

int svg_pt_in_bounds(float* pt, float* bounds) {
    return pt[0] >= bounds[0] && pt[0] <= bounds[2] && pt[1] >= bounds[1] && pt[1] <= bounds[3];
}

double svg_eval_bezier(double t, double p0, double p1, double p2, double p3) {
    double it = 1.0 - t;
    return it * it * it * p0 + 3.0 * it * it * t * p1 + 3.0 * it * t * t * p2 + t * t * t * p3;
}

void svg_curve_bounds(float* bounds, float* curve) {
    int i, j, count;
    double roots[2], a, b, c, b2ac, t, v;
    float* v0 = &curve[0];
    float* v1 = &curve[2];
    float* v2 = &curve[4];
    float* v3 = &curve[6];

    // Start the bounding box by end points
    bounds[0] = svg_minf(v0[0], v3[0]);
    bounds[1] = svg_minf(v0[1], v3[1]);
    bounds[2] = svg_maxf(v0[0], v3[0]);
    bounds[3] = svg_maxf(v0[1], v3[1]);

    // Bezier curve fits inside the convex hull of it's control points.
    // If control points are inside the bounds, we're done.
    if (svg_pt_in_bounds(v1, bounds) && svg_pt_in_bounds(v2, bounds))
        return;

    // Add bezier curve inflection points in X and Y.
    for (i = 0; i < 2; i++) {
        a = -3.0 * v0[i] + 9.0 * v1[i] - 9.0 * v2[i] + 3.0 * v3[i];
        b = 6.0 * v0[i] - 12.0 * v1[i] + 6.0 * v2[i];
        c = 3.0 * v1[i] - 3.0 * v0[i];
        count = 0;
        if (fabs(a) < SVG_EPSILON) {
            if (fabs(b) > SVG_EPSILON) {
                t = -c / b;
                if (t > SVG_EPSILON && t < 1.0 - SVG_EPSILON)
                    roots[count++] = t;
            }
        } else {
            b2ac = b * b - 4.0 * c * a;
            if (b2ac > SVG_EPSILON) {
                t = (-b + sqrt(b2ac)) / (2.0 * a);
                if (t > SVG_EPSILON && t < 1.0 - SVG_EPSILON)
                    roots[count++] = t;
                t = (-b - sqrt(b2ac)) / (2.0 * a);
                if (t > SVG_EPSILON && t < 1.0 - SVG_EPSILON)
                    roots[count++] = t;
            }
        }
        for (j = 0; j < count; j++) {
            v = svg_eval_bezier(roots[j], v0[i], v1[i], v2[i], v3[i]);
            bounds[0 + i] = svg_minf(bounds[0 + i], (float)v);
            bounds[2 + i] = svg_maxf(bounds[2 + i], (float)v);
        }
    }
}

float svg_clampf(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

float svg_sqr(float x) { return x * x; }
float svg_vmag(float x, float y) { return sqrtf(x * x + y * y); }

float svg_vecrat(float ux, float uy, float vx, float vy) {
    return (ux * vx + uy * vy) / (svg_vmag(ux, uy) * svg_vmag(vx, vy));
}

float svg_vecang(float ux, float uy, float vx, float vy) {
    float r = svg_vecrat(ux, uy, vx, vy);
    if (r < -1.0f) r = -1.0f;
    if (r > 1.0f) r = 1.0f;
    return ((ux * vy < uy * vx) ? -1.0f : 1.0f) * acosf(r);
}

void svg_xform_identity(float* t) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void svg_xform_set_translation(float* t, float tx, float ty) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = tx;
    t[5] = ty;
}

void svg_xform_set_scale(float* t, float sx, float sy) {
    t[0] = sx;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = sy;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void svg_xform_set_skew_x(float* t, float a) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = tanf(a);
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void svg_xform_set_skew_y(float* t, float a) {
    t[0] = 1.0f;
    t[1] = tanf(a);
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void svg_xform_set_rotation(float* t, float a) {
    float cs = cosf(a), sn = sinf(a);
    t[0] = cs;
    t[1] = sn;
    t[2] = -sn;
    t[3] = cs;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void svg_xform_multiply(float* t, const float* s) {
    float t0 = t[0] * s[0] + t[1] * s[2];
    float t2 = t[2] * s[0] + t[3] * s[2];
    float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
    t[1] = t[0] * s[1] + t[1] * s[3];
    t[3] = t[2] * s[1] + t[3] * s[3];
    t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
    t[0] = t0;
    t[2] = t2;
    t[4] = t4;
}

void svg_xform_inverse(float* inv, float* t) {
    double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
    if (det > -1e-6 && det < 1e-6) {
        svg_xform_identity(t);
        return;
    }
    invdet = 1.0 / det;
    inv[0] = (float)(t[3] * invdet);
    inv[2] = (float)(-t[2] * invdet);
    inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
    inv[1] = (float)(-t[1] * invdet);
    inv[3] = (float)(t[0] * invdet);
    inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
}

void svg_xform_premultiply(float* t, const float* s) {
    float s2[6];
    memcpy(s2, s, sizeof(float) * 6);
    svg_xform_multiply(s2, t);
    memcpy(t, s2, sizeof(float) * 6);
}

void svg_xform_point(float* dx, float* dy, float x, float y, const float* t) {
    *dx = x * t[0] + y * t[2] + t[4];
    *dy = x * t[1] + y * t[3] + t[5];
}

void svg_xform_vec(float* dx, float* dy, float x, float y, const float* t) {
    *dx = x * t[0] + y * t[2];
    *dy = x * t[1] + y * t[3];
}
void svg_get_local_bounds(float* bounds, svg_shape* shape, float* xform) {
    svg_path* path;
    float curve[4 * 2], curveBounds[4];
    int i, first = 1;
    for (path = shape->paths; path != NULL; path = path->next) {
        svg_xform_point(&curve[0], &curve[1], path->points[0], path->points[1], xform);
        for (i = 0; i < path->point_count - 1; i += 3) {
            svg_xform_point(&curve[2], &curve[3], path->points[(i + 1) * 2], path->points[(i + 1) * 2 + 1], xform);
            svg_xform_point(&curve[4], &curve[5], path->points[(i + 2) * 2], path->points[(i + 2) * 2 + 1], xform);
            svg_xform_point(&curve[6], &curve[7], path->points[(i + 3) * 2], path->points[(i + 3) * 2 + 1], xform);
            svg_curve_bounds(curveBounds, curve);
            if (first) {
                bounds[0] = curveBounds[0];
                bounds[1] = curveBounds[1];
                bounds[2] = curveBounds[2];
                bounds[3] = curveBounds[3];
                first = 0;
            } else {
                bounds[0] = svg_minf(bounds[0], curveBounds[0]);
                bounds[1] = svg_minf(bounds[1], curveBounds[1]);
                bounds[2] = svg_maxf(bounds[2], curveBounds[2]);
                bounds[3] = svg_maxf(bounds[3], curveBounds[3]);
            }
            curve[0] = curve[6];
            curve[1] = curve[7];
        }
    }
}
void svg_transform::identity() {
    svg_xform_identity(data);
}
void svg_transform::set_translation(float tx, float ty) {
    svg_xform_set_translation(data,tx,ty);
}
void svg_transform::set_scale(float sx, float sy) {
    svg_xform_set_scale(data,sx,sy);
}
void svg_transform::set_skew_x(float a) {
    svg_xform_set_skew_x(data, svg_clampf(a, -360.0f, 360.0f) * (SVG_PI / 180.0f));
}
void svg_transform::set_skew_y(float a) {
    svg_xform_set_skew_y(data, svg_clampf(a, -360.0f, 360.0f) * (SVG_PI / 180.0f));
}
void svg_transform::set_rotation(float a) {
    svg_xform_set_rotation(data, svg_clampf(a, -360.0f, 360.0f) * (SVG_PI / 180.0f));
}
void svg_transform::multiply(const svg_transform& rhs) {
    svg_xform_multiply(data,rhs.data);
}
void svg_transform::inverse(svg_transform& rhs) {
    svg_xform_inverse(data,rhs.data);
}
void svg_transform::premultiply(const svg_transform& rhs) {
    svg_xform_premultiply(data,rhs.data);
}
void svg_transform::point(gfx::pointf* pt_d, gfx::pointf pt) const {
    float dx,dy;
    svg_xform_point(&dx,&dy,pt.x,pt.y,data);
    *pt_d = {dx,dy};
}
void svg_transform::vector(gfx::pointf* pt_d, gfx::pointf pt) const {
    float dx, dy;
    svg_xform_vec(&dx, &dy, pt.x, pt.y, data);
    *pt_d = {dx, dy};
}

}  // namespace gfx
