#include <stdlib.h>
#include <string.h>
#include "svg_private.hpp"

#define NSVG__SUBSAMPLES 5
#define NSVG__FIXSHIFT 10
#define NSVG__FIX (1 << NSVG__FIXSHIFT)
#define NSVG__FIXMASK (NSVG__FIX - 1)

#ifndef NSVG_PI
#define NSVG_PI (3.14159265358979323846264338327f)
#endif
namespace gfx {
enum NSVGpointFlags {
    NSVG_PT_CORNER = 0x01,
    NSVG_PT_BEVEL = 0x02,
    NSVG_PT_LEFT = 0x04
};

static svg_mem_page* svg_next_page(NSVGrasterizer* r, svg_mem_page* cur) {
    svg_mem_page* newp;

    // If using existing chain, return the next page in chain
    if (cur != NULL && cur->next != NULL) {
        return cur->next;
    }

    // Alloc new page
    newp = (svg_mem_page*)r->allocator(sizeof(svg_mem_page));
    if (newp == NULL) return NULL;
    memset(newp, 0, sizeof(svg_mem_page));

    // Add to linked list
    if (cur != NULL)
        cur->next = newp;
    else
        r->pages = newp;

    return newp;
}

static void svg_reset_pool(NSVGrasterizer* r) {
    svg_mem_page* p = r->pages;
    while (p != NULL) {
        p->size = 0;
        p = p->next;
    }
    r->curpage = r->pages;
}

static unsigned char* svg_alloc(NSVGrasterizer* r, int size) {
    unsigned char* buf;
    if (((size_t)size) > svg_mem_page::mem_page_size) return NULL;
    if (r->curpage == NULL || r->curpage->size + ((size_t)size) > svg_mem_page::mem_page_size) {
        r->curpage = svg_next_page(r, r->curpage);
    }
    buf = &r->curpage->mem[r->curpage->size];
    r->curpage->size += size;
    return buf;
}

static int svg_pt_equals(float x1, float y1, float x2, float y2, float tol) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy < tol * tol;
}

static void svg_add_path_point(NSVGrasterizer* r, float x, float y, int flags) {
    svg_point* pt;

    if (r->npoints > 0) {
        pt = &r->points[r->npoints - 1];
        if (svg_pt_equals(pt->x, pt->y, x, y, r->distTol)) {
            pt->flags = (unsigned char)(pt->flags | flags);
            return;
        }
    }

    if (r->npoints + 1 > r->cpoints) {
        r->cpoints = r->cpoints > 0 ? r->cpoints * 2 : 64;
        r->points = (svg_point*)r->reallocator(r->points, sizeof(svg_point) * r->cpoints);
        if (r->points == NULL) return;
    }

    pt = &r->points[r->npoints];
    pt->x = x;
    pt->y = y;
    pt->flags = (unsigned char)flags;
    r->npoints++;
}

static void svg_duplicate_points(NSVGrasterizer* r) {
    if (r->npoints > r->cpoints2) {
        r->cpoints2 = r->npoints;
        r->points2 = (svg_point*)r->reallocator(r->points2, sizeof(svg_point) * r->cpoints2);
        if (r->points2 == NULL) return;
    }

    memcpy(r->points2, r->points, sizeof(svg_point) * r->npoints);
    r->npoints2 = r->npoints;
}
static void svg_add_edge(NSVGrasterizer* r, float x0, float y0, float x1, float y1) {
    svg_edge* e;

    // Skip horizontal edges
    if (y0 == y1)
        return;

    if (r->nedges + 1 > r->cedges) {
        r->cedges = r->cedges > 0 ? r->cedges * 2 : 64;
        r->edges = (svg_edge*)r->reallocator(r->edges, sizeof(svg_edge) * r->cedges);
        if (r->edges == NULL) return;
    }

    e = &r->edges[r->nedges];
    r->nedges++;

    if (y0 < y1) {
        e->x0 = x0;
        e->y0 = y0;
        e->x1 = x1;
        e->y1 = y1;
        e->direction = 1;
    } else {
        e->x0 = x1;
        e->y0 = y1;
        e->x1 = x0;
        e->y1 = y0;
        e->direction = -1;
    }
}

static void svg_append_path_point(NSVGrasterizer* r, svg_point pt) {
    if (r->npoints + 1 > r->cpoints) {
        r->cpoints = r->cpoints > 0 ? r->cpoints * 2 : 64;
        r->points = (svg_point*)r->reallocator(r->points, sizeof(svg_point) * r->cpoints);
        if (r->points == NULL) return;
    }
    r->points[r->npoints] = pt;
    r->npoints++;
}
static float svg_normalize(float* x, float* y) {
    float d = sqrtf((*x) * (*x) + (*y) * (*y));
    if (d > 1e-6f) {
        float id = 1.0f / d;
        *x *= id;
        *y *= id;
    }
    return d;
}

static float svg_absf(float x) { return x < 0 ? -x : x; }

static void svg_flatten_cubic_bez(NSVGrasterizer* r,
                                  float x1, float y1, float x2, float y2,
                                  float x3, float y3, float x4, float y4,
                                  int level, int type) {
    float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
    float dx, dy, d2, d3;

    if (level > 10) return;

    x12 = (x1 + x2) * 0.5f;
    y12 = (y1 + y2) * 0.5f;
    x23 = (x2 + x3) * 0.5f;
    y23 = (y2 + y3) * 0.5f;
    x34 = (x3 + x4) * 0.5f;
    y34 = (y3 + y4) * 0.5f;
    x123 = (x12 + x23) * 0.5f;
    y123 = (y12 + y23) * 0.5f;

    dx = x4 - x1;
    dy = y4 - y1;
    d2 = svg_absf(((x2 - x4) * dy - (y2 - y4) * dx));
    d3 = svg_absf(((x3 - x4) * dy - (y3 - y4) * dx));

    if ((d2 + d3) * (d2 + d3) < r->tessTol * (dx * dx + dy * dy)) {
        svg_add_path_point(r, x4, y4, type);
        return;
    }

    x234 = (x23 + x34) * 0.5f;
    y234 = (y23 + y34) * 0.5f;
    x1234 = (x123 + x234) * 0.5f;
    y1234 = (y123 + y234) * 0.5f;

    svg_flatten_cubic_bez(r, x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0);
    svg_flatten_cubic_bez(r, x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type);
}
static void svg_flatten_shape(NSVGrasterizer* r, svg_shape* shape, float scale) {
    int i, j;
    svg_path* path;

    for (path = shape->paths; path != NULL; path = path->next) {
        r->npoints = 0;
        // Flatten path
        svg_add_path_point(r, path->points[0] * scale, path->points[1] * scale, 0);
        for (i = 0; i < path->point_count - 1; i += 3) {
            float* p = &path->points[i * 2];
            svg_flatten_cubic_bez(r, p[0] * scale, p[1] * scale, p[2] * scale, p[3] * scale, p[4] * scale, p[5] * scale, p[6] * scale, p[7] * scale, 0, 0);
        }
        // Close path
        svg_add_path_point(r, path->points[0] * scale, path->points[1] * scale, 0);
        // Build edges
        for (i = 0, j = r->npoints - 1; i < r->npoints; j = i++)
            svg_add_edge(r, r->points[j].x, r->points[j].y, r->points[i].x, r->points[i].y);
    }
}
static void svg_init_closed(svg_point* left, svg_point* right, svg_point* p0, svg_point* p1, float lineWidth) {
    float w = lineWidth * 0.5f;
    float dx = p1->x - p0->x;
    float dy = p1->y - p0->y;
    float len = svg_normalize(&dx, &dy);
    float px = p0->x + dx * len * 0.5f, py = p0->y + dy * len * 0.5f;
    float dlx = dy, dly = -dx;
    float lx = px - dlx * w, ly = py - dly * w;
    float rx = px + dlx * w, ry = py + dly * w;
    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}

static void svg_butt_cap(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p, float dx, float dy, float lineWidth, int connect) {
    float w = lineWidth * 0.5f;
    float px = p->x, py = p->y;
    float dlx = dy, dly = -dx;
    float lx = px - dlx * w, ly = py - dly * w;
    float rx = px + dlx * w, ry = py + dly * w;

    svg_add_edge(r, lx, ly, rx, ry);

    if (connect) {
        svg_add_edge(r, left->x, left->y, lx, ly);
        svg_add_edge(r, rx, ry, right->x, right->y);
    }
    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}
static void svg_square_cap(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p, float dx, float dy, float lineWidth, int connect) {
    float w = lineWidth * 0.5f;
    float px = p->x - dx * w, py = p->y - dy * w;
    float dlx = dy, dly = -dx;
    float lx = px - dlx * w, ly = py - dly * w;
    float rx = px + dlx * w, ry = py + dly * w;

    svg_add_edge(r, lx, ly, rx, ry);

    if (connect) {
        svg_add_edge(r, left->x, left->y, lx, ly);
        svg_add_edge(r, rx, ry, right->x, right->y);
    }
    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}

static void svg_round_cap(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p, float dx, float dy, float lineWidth, int ncap, int connect) {
    int i;
    float w = lineWidth * 0.5f;
    float px = p->x, py = p->y;
    float dlx = dy, dly = -dx;
    float lx = 0, ly = 0, rx = 0, ry = 0, prevx = 0, prevy = 0;

    for (i = 0; i < ncap; i++) {
        float a = (float)i / (float)(ncap - 1) * NSVG_PI;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        float x = px - dlx * ax - dx * ay;
        float y = py - dly * ax - dy * ay;

        if (i > 0)
            svg_add_edge(r, prevx, prevy, x, y);

        prevx = x;
        prevy = y;

        if (i == 0) {
            lx = x;
            ly = y;
        } else if (i == ncap - 1) {
            rx = x;
            ry = y;
        }
    }

    if (connect) {
        svg_add_edge(r, left->x, left->y, lx, ly);
        svg_add_edge(r, rx, ry, right->x, right->y);
    }

    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}
static void svg_bevel_join(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p0, svg_point* p1, float lineWidth) {
    float w = lineWidth * 0.5f;
    float dlx0 = p0->dy, dly0 = -p0->dx;
    float dlx1 = p1->dy, dly1 = -p1->dx;
    float lx0 = p1->x - (dlx0 * w), ly0 = p1->y - (dly0 * w);
    float rx0 = p1->x + (dlx0 * w), ry0 = p1->y + (dly0 * w);
    float lx1 = p1->x - (dlx1 * w), ly1 = p1->y - (dly1 * w);
    float rx1 = p1->x + (dlx1 * w), ry1 = p1->y + (dly1 * w);

    svg_add_edge(r, lx0, ly0, left->x, left->y);
    svg_add_edge(r, lx1, ly1, lx0, ly0);

    svg_add_edge(r, right->x, right->y, rx0, ry0);
    svg_add_edge(r, rx0, ry0, rx1, ry1);

    left->x = lx1;
    left->y = ly1;
    right->x = rx1;
    right->y = ry1;
}

static void svg_miter_join(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p0, svg_point* p1, float lineWidth) {
    float w = lineWidth * 0.5f;
    float dlx0 = p0->dy, dly0 = -p0->dx;
    float dlx1 = p1->dy, dly1 = -p1->dx;
    float lx0, rx0, lx1, rx1;
    float ly0, ry0, ly1, ry1;

    if (p1->flags & NSVG_PT_LEFT) {
        lx0 = lx1 = p1->x - p1->dmx * w;
        ly0 = ly1 = p1->y - p1->dmy * w;
        svg_add_edge(r, lx1, ly1, left->x, left->y);

        rx0 = p1->x + (dlx0 * w);
        ry0 = p1->y + (dly0 * w);
        rx1 = p1->x + (dlx1 * w);
        ry1 = p1->y + (dly1 * w);
        svg_add_edge(r, right->x, right->y, rx0, ry0);
        svg_add_edge(r, rx0, ry0, rx1, ry1);
    } else {
        lx0 = p1->x - (dlx0 * w);
        ly0 = p1->y - (dly0 * w);
        lx1 = p1->x - (dlx1 * w);
        ly1 = p1->y - (dly1 * w);
        svg_add_edge(r, lx0, ly0, left->x, left->y);
        svg_add_edge(r, lx1, ly1, lx0, ly0);

        rx0 = rx1 = p1->x + p1->dmx * w;
        ry0 = ry1 = p1->y + p1->dmy * w;
        svg_add_edge(r, right->x, right->y, rx1, ry1);
    }

    left->x = lx1;
    left->y = ly1;
    right->x = rx1;
    right->y = ry1;
}

static void svg_round_join(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p0, svg_point* p1, float lineWidth, int ncap) {
    int i, n;
    float w = lineWidth * 0.5f;
    float dlx0 = p0->dy, dly0 = -p0->dx;
    float dlx1 = p1->dy, dly1 = -p1->dx;
    float a0 = atan2f(dly0, dlx0);
    float a1 = atan2f(dly1, dlx1);
    float da = a1 - a0;
    float lx, ly, rx, ry;

    if (da < NSVG_PI) da += NSVG_PI * 2;
    if (da > NSVG_PI) da -= NSVG_PI * 2;

    n = (int)ceilf((svg_absf(da) / NSVG_PI) * (float)ncap);
    if (n < 2) n = 2;
    if (n > ncap) n = ncap;

    lx = left->x;
    ly = left->y;
    rx = right->x;
    ry = right->y;

    for (i = 0; i < n; i++) {
        float u = (float)i / (float)(n - 1);
        float a = a0 + u * da;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        float lx1 = p1->x - ax, ly1 = p1->y - ay;
        float rx1 = p1->x + ax, ry1 = p1->y + ay;

        svg_add_edge(r, lx1, ly1, lx, ly);
        svg_add_edge(r, rx, ry, rx1, ry1);

        lx = lx1;
        ly = ly1;
        rx = rx1;
        ry = ry1;
    }

    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}

static void svg_straight_join(NSVGrasterizer* r, svg_point* left, svg_point* right, svg_point* p1, float lineWidth) {
    float w = lineWidth * 0.5f;
    float lx = p1->x - (p1->dmx * w), ly = p1->y - (p1->dmy * w);
    float rx = p1->x + (p1->dmx * w), ry = p1->y + (p1->dmy * w);

    svg_add_edge(r, lx, ly, left->x, left->y);
    svg_add_edge(r, right->x, right->y, rx, ry);

    left->x = lx;
    left->y = ly;
    right->x = rx;
    right->y = ry;
}

static int svg_curve_divs(float r, float arc, float tol) {
    float da = acosf(r / (r + tol)) * 2.0f;
    int divs = (int)ceilf(arc / da);
    if (divs < 2) divs = 2;
    return divs;
}
static void svg_expand_stroke(NSVGrasterizer* r, svg_point* points, int npoints, int closed, svg_line_join lineJoin, svg_line_cap lineCap, float lineWidth) {
    int ncap = svg_curve_divs(lineWidth * 0.5f, NSVG_PI, r->tessTol);  // Calculate divisions per half circle.
    svg_point left = {0, 0, 0, 0, 0, 0, 0, 0}, right = {0, 0, 0, 0, 0, 0, 0, 0}, firstLeft = {0, 0, 0, 0, 0, 0, 0, 0}, firstRight = {0, 0, 0, 0, 0, 0, 0, 0};
    svg_point *p0, *p1;
    int j, s, e;

    // Build stroke edges
    if (closed) {
        // Looping
        p0 = &points[npoints - 1];
        p1 = &points[0];
        s = 0;
        e = npoints;
    } else {
        // Add cap
        p0 = &points[0];
        p1 = &points[1];
        s = 1;
        e = npoints - 1;
    }

    if (closed) {
        svg_init_closed(&left, &right, p0, p1, lineWidth);
        firstLeft = left;
        firstRight = right;
    } else {
        // Add cap
        float dx = p1->x - p0->x;
        float dy = p1->y - p0->y;
        svg_normalize(&dx, &dy);
        if (lineCap == svg_line_cap::butt)
            svg_butt_cap(r, &left, &right, p0, dx, dy, lineWidth, 0);
        else if (lineCap == svg_line_cap::square)
            svg_square_cap(r, &left, &right, p0, dx, dy, lineWidth, 0);
        else if (lineCap == svg_line_cap::round)
            svg_round_cap(r, &left, &right, p0, dx, dy, lineWidth, ncap, 0);
    }

    for (j = s; j < e; ++j) {
        if (p1->flags & NSVG_PT_CORNER) {
            if (lineJoin == svg_line_join::round)
                svg_round_join(r, &left, &right, p0, p1, lineWidth, ncap);
            else if (lineJoin == svg_line_join::bevel || (p1->flags & NSVG_PT_BEVEL))
                svg_bevel_join(r, &left, &right, p0, p1, lineWidth);
            else
                svg_miter_join(r, &left, &right, p0, p1, lineWidth);
        } else {
            svg_straight_join(r, &left, &right, p1, lineWidth);
        }
        p0 = p1++;
    }

    if (closed) {
        // Loop it
        svg_add_edge(r, firstLeft.x, firstLeft.y, left.x, left.y);
        svg_add_edge(r, right.x, right.y, firstRight.x, firstRight.y);
    } else {
        // Add cap
        float dx = p1->x - p0->x;
        float dy = p1->y - p0->y;
        svg_normalize(&dx, &dy);
        if (lineCap == svg_line_cap::butt)
            svg_butt_cap(r, &right, &left, p1, -dx, -dy, lineWidth, 1);
        else if (lineCap == svg_line_cap::square)
            svg_square_cap(r, &right, &left, p1, -dx, -dy, lineWidth, 1);
        else if (lineCap == svg_line_cap::round)
            svg_round_cap(r, &right, &left, p1, -dx, -dy, lineWidth, ncap, 1);
    }
}
static void svg_prepare_stroke(NSVGrasterizer* r, float miterLimit, svg_line_join lineJoin) {
    int i, j;
    svg_point *p0, *p1;

    p0 = &r->points[r->npoints - 1];
    p1 = &r->points[0];
    for (i = 0; i < r->npoints; i++) {
        // Calculate segment direction and length
        p0->dx = p1->x - p0->x;
        p0->dy = p1->y - p0->y;
        p0->len = svg_normalize(&p0->dx, &p0->dy);
        // Advance
        p0 = p1++;
    }

    // calculate joins
    p0 = &r->points[r->npoints - 1];
    p1 = &r->points[0];
    for (j = 0; j < r->npoints; j++) {
        float dlx0, dly0, dlx1, dly1, dmr2, cross;
        dlx0 = p0->dy;
        dly0 = -p0->dx;
        dlx1 = p1->dy;
        dly1 = -p1->dx;
        // Calculate extrusions
        p1->dmx = (dlx0 + dlx1) * 0.5f;
        p1->dmy = (dly0 + dly1) * 0.5f;
        dmr2 = p1->dmx * p1->dmx + p1->dmy * p1->dmy;
        if (dmr2 > 0.000001f) {
            float s2 = 1.0f / dmr2;
            if (s2 > 600.0f) {
                s2 = 600.0f;
            }
            p1->dmx *= s2;
            p1->dmy *= s2;
        }

        // Clear flags, but keep the corner.
        p1->flags = (p1->flags & NSVG_PT_CORNER) ? NSVG_PT_CORNER : 0;

        // Keep track of left turns.
        cross = p1->dx * p0->dy - p0->dx * p1->dy;
        if (cross > 0.0f)
            p1->flags |= NSVG_PT_LEFT;

        // Check to see if the corner needs to be beveled.
        if (p1->flags & NSVG_PT_CORNER) {
            if ((dmr2 * miterLimit * miterLimit) < 1.0f || lineJoin == svg_line_join::bevel || lineJoin == svg_line_join::round) {
                p1->flags |= NSVG_PT_BEVEL;
            }
        }

        p0 = p1++;
    }
}
static void svg_flatten_shape_stroke(NSVGrasterizer* r, svg_shape* shape, float scale) {
    int i, j, closed;
    svg_path* path;
    svg_point *p0, *p1;
    float miterLimit = shape->miter_limit;
    svg_line_join lineJoin = shape->stroke_line_join;
    svg_line_cap lineCap = shape->stroke_line_cap;
    float lineWidth = shape->stroke_width * scale;

    for (path = shape->paths; path != NULL; path = path->next) {
        // Flatten path
        r->npoints = 0;
        svg_add_path_point(r, path->points[0] * scale, path->points[1] * scale, NSVG_PT_CORNER);
        for (i = 0; i < path->point_count - 1; i += 3) {
            float* p = &path->points[i * 2];
            svg_flatten_cubic_bez(r, p[0] * scale, p[1] * scale, p[2] * scale, p[3] * scale, p[4] * scale, p[5] * scale, p[6] * scale, p[7] * scale, 0, NSVG_PT_CORNER);
        }
        if (r->npoints < 2)
            continue;

        closed = path->closed;

        // If the first and last points are the same, remove the last, mark as closed path.
        p0 = &r->points[r->npoints - 1];
        p1 = &r->points[0];
        if (svg_pt_equals(p0->x, p0->y, p1->x, p1->y, r->distTol)) {
            r->npoints--;
            p0 = &r->points[r->npoints - 1];
            closed = 1;
        }

        if (shape->stroke_dash_count > 0) {
            int idash = 0, dashState = 1;
            float totalDist = 0, dashLen, allDashLen, dashOffset;
            svg_point cur;

            if (closed)
                svg_append_path_point(r, r->points[0]);

            // Duplicate points -> points2.
            svg_duplicate_points(r);

            r->npoints = 0;
            cur = r->points2[0];
            svg_append_path_point(r, cur);

            // Figure out dash offset.
            allDashLen = 0;
            for (j = 0; j < ((int)shape->stroke_dash_count); j++)
                allDashLen += shape->stroke_dash_array[j];
            if (shape->stroke_dash_count & 1)
                allDashLen *= 2.0f;
            // Find location inside pattern
            dashOffset = fmodf(shape->stroke_dash_offset, allDashLen);
            if (dashOffset < 0.0f)
                dashOffset += allDashLen;

            while (dashOffset > shape->stroke_dash_array[idash]) {
                dashOffset -= shape->stroke_dash_array[idash];
                idash = (idash + 1) % shape->stroke_dash_count;
            }
            dashLen = (shape->stroke_dash_array[idash] - dashOffset) * scale;

            for (j = 1; j < r->npoints2;) {
                float dx = r->points2[j].x - cur.x;
                float dy = r->points2[j].y - cur.y;
                float dist = sqrtf(dx * dx + dy * dy);

                if ((totalDist + dist) > dashLen) {
                    // Calculate intermediate point
                    float d = (dashLen - totalDist) / dist;
                    float x = cur.x + dx * d;
                    float y = cur.y + dy * d;
                    svg_add_path_point(r, x, y, NSVG_PT_CORNER);

                    // Stroke
                    if (r->npoints > 1 && dashState) {
                        svg_prepare_stroke(r, miterLimit, lineJoin);
                        svg_expand_stroke(r, r->points, r->npoints, 0, lineJoin, lineCap, lineWidth);
                    }
                    // Advance dash pattern
                    dashState = !dashState;
                    idash = (idash + 1) % shape->stroke_dash_count;
                    dashLen = shape->stroke_dash_array[idash] * scale;
                    // Restart
                    cur.x = x;
                    cur.y = y;
                    cur.flags = NSVG_PT_CORNER;
                    totalDist = 0.0f;
                    r->npoints = 0;
                    svg_append_path_point(r, cur);
                } else {
                    totalDist += dist;
                    cur = r->points2[j];
                    svg_append_path_point(r, cur);
                    j++;
                }
            }
            // Stroke any leftover path
            if (r->npoints > 1 && dashState)
                svg_expand_stroke(r, r->points, r->npoints, 0, lineJoin, lineCap, lineWidth);
        } else {
            svg_prepare_stroke(r, miterLimit, lineJoin);
            svg_expand_stroke(r, r->points, r->npoints, closed, lineJoin, lineCap, lineWidth);
        }
    }
}

static int svg_cmp_edge(const void* p, const void* q) {
    const svg_edge* a = (const svg_edge*)p;
    const svg_edge* b = (const svg_edge*)q;

    if (a->y0 < b->y0) return -1;
    if (a->y0 > b->y0) return 1;
    return 0;
}

static svg_active_edge* svg_add_active(NSVGrasterizer* r, svg_edge* e, float startPoint) {
    svg_active_edge* z;

    if (r->freelist != NULL) {
        // Restore from freelist.
        z = r->freelist;
        r->freelist = z->next;
    } else {
        // Alloc new edge.
        z = (svg_active_edge*)svg_alloc(r, sizeof(svg_active_edge));
        if (z == NULL) return NULL;
    }

    float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    //	STBTT_assert(e->y0 <= start_point);
    // round dx down to avoid going too far
    if (dxdy < 0)
        z->dx = (int)(-floorf(NSVG__FIX * -dxdy));
    else
        z->dx = (int)floorf(NSVG__FIX * dxdy);
    z->x = (int)floorf(NSVG__FIX * (e->x0 + dxdy * (startPoint - e->y0)));
    //	z->x -= off_x * FIX;
    z->ey = e->y1;
    z->next = 0;
    z->dir = e->direction;

    return z;
}

static void svg_free_active(NSVGrasterizer* r, svg_active_edge* z) {
    z->next = r->freelist;
    r->freelist = z;
}

static void svg_fill_scanline(unsigned char* scanline, int len, int x0, int x1, int maxWeight, int* xmin, int* xmax) {
    int i = x0 >> NSVG__FIXSHIFT;
    int j = x1 >> NSVG__FIXSHIFT;
    if (i < *xmin) *xmin = i;
    if (j > *xmax) *xmax = j;
    if (i < len && j >= 0) {
        if (i == j) {
            // x0,x1 are the same pixel, so compute combined coverage
            scanline[i] = (unsigned char)(scanline[i] + ((x1 - x0) * maxWeight >> NSVG__FIXSHIFT));
        } else {
            if (i >= 0)  // add antialiasing for x0
                scanline[i] = (unsigned char)(scanline[i] + (((NSVG__FIX - (x0 & NSVG__FIXMASK)) * maxWeight) >> NSVG__FIXSHIFT));
            else
                i = -1;  // clip

            if (j < len)  // add antialiasing for x1
                scanline[j] = (unsigned char)(scanline[j] + (((x1 & NSVG__FIXMASK) * maxWeight) >> NSVG__FIXSHIFT));
            else
                j = len;  // clip

            for (++i; i < j; ++i)  // fill pixels between x0 and x1
                scanline[i] = (unsigned char)(scanline[i] + maxWeight);
        }
    }
}
static void svg_fill_active_edges(unsigned char* scanline, int len, svg_active_edge* e, int maxWeight, int* xmin, int* xmax, svg_fill_rule fillRule) {
    // non-zero winding fill
    int x0 = 0, w = 0;

    if (fillRule == svg_fill_rule::non_zero) {
        // Non-zero
        while (e != NULL) {
            if (w == 0) {
                // if we're currently at zero, we need to record the edge start point
                x0 = e->x;
                w += e->dir;
            } else {
                int x1 = e->x;
                w += e->dir;
                // if we went to zero, we need to draw
                if (w == 0)
                    svg_fill_scanline(scanline, len, x0, x1, maxWeight, xmin, xmax);
            }
            e = e->next;
        }
    } else if (fillRule == svg_fill_rule::even_odd) {
        // Even-odd
        while (e != NULL) {
            if (w == 0) {
                // if we're currently at zero, we need to record the edge start point
                x0 = e->x;
                w = 1;
            } else {
                int x1 = e->x;
                w = 0;
                svg_fill_scanline(scanline, len, x0, x1, maxWeight, xmin, xmax);
            }
            e = e->next;
        }
    }
}
static rgba_pixel<32> svg_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return rgba_pixel<32>(r,g,b,a);
}

static rgba_pixel<32> svg_lerp_rgba(rgba_pixel<32> c_lhs, rgba_pixel<32> c_rhs, float u) {
    int iu = (int)(svg_clampf(u, 0.0f, 1.0f) * 256.0f);
    int r = (c_lhs.template channel<channel_name::R>() * (256 - iu) + (c_rhs.template channel<channel_name::R>() * iu))>>8;
    int g = (c_lhs.template channel<channel_name::G>() * (256 - iu) + (c_rhs.template channel<channel_name::G>() * iu)) >> 8;
    int b = (c_lhs.template channel<channel_name::B>() * (256 - iu) + (c_rhs.template channel<channel_name::B>() * iu)) >> 8;
    int a = (c_lhs.template channel<channel_name::A>() * (256 - iu) + (c_rhs.template channel<channel_name::A>() * iu)) >> 8;
    return svg_rgba((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
}

static inline int svg_div255(int x) {
    return ((x + 1) * 257) >> 16;
}
static rgba_pixel<32> svg_apply_opacity(rgba_pixel<32> c, float u) {
    float iu = svg_clampf(u, 0.0f, 1.0f) ;
    int r = c.template channel<channel_name::R>();
    int g = c.template channel<channel_name::G>();
    int b = c.template channel<channel_name::B>();
    int a = c.template channel<channel_name::A>()*iu;
    return svg_rgba((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
}

static void svg_scanline_solid(unsigned char* dst, NSVGrasterizerReadCallback readCallback, void* readCallbackState, NSVGrasterizerWriteCallback writeCallback, void* writeCallbackState, int count, unsigned char* cover, int x, int y,
                                float tx, float ty, float scale, svg_cached_paint* cache) {
    if (dst != nullptr) {
        if (cache->type == svg_paint_type::color) {
            int i, cr, cg, cb, ca;
            cr = cache->colors[0].template channel<channel_name::R>();
            cg = cache->colors[0].template channel<channel_name::G>();
            cb = cache->colors[0].template channel<channel_name::B>();
            ca = cache->colors[0].template channel<channel_name::A>();
            
            for (i = 0; i < count; i++) {
                int r, g, b;
                int cv = cover[0];
                int a = svg_div255(cv * ca);
                int ia = 255 - a;

                cover++;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);

                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;

                dst += 4;
            }
        } else if (cache->type == svg_paint_type::linear_gradient) {
            // TODO: spread modes.
            // TODO: plenty of opportunities to optimize.
            float fx, fy, dx, gy;
            float* t = cache->xform;
            int i, cr, cg, cb, ca;
            rgba_pixel<32> c;

            fx = ((float)x - tx) / scale;
            fy = ((float)y - ty) / scale;
            dx = 1.0f / scale;

            for (i = 0; i < count; i++) {
                int r, g, b, a, ia;
                gy = fx * t[1] + fy * t[3] + t[5];
                c = cache->colors[(int)svg_clampf(gy * 255.0f, 0, 255.0f)];
                cr = c.template channel<channel_name::R>();
                cg = c.template channel<channel_name::G>();
                cb = c.template channel<channel_name::B>();
                ca = c.template channel<channel_name::A>();

                a = svg_div255((int)cover[0] * ca);
                ia = 255 - a;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);

                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;

                cover++;
                dst += 4;
                fx += dx;
            }
        } else if (cache->type == svg_paint_type::radial_gradient) {
            // TODO: spread modes.
            // TODO: plenty of opportunities to optimize.
            // TODO: focus (fx,fy)
            float fx, fy, dx, gx, gy, gd;
            float* t = cache->xform;
            int i, cr, cg, cb, ca;
            rgba_pixel<32> c;

            fx = ((float)x - tx) / scale;
            fy = ((float)y - ty) / scale;
            dx = 1.0f / scale;

            for (i = 0; i < count; i++) {
                int r, g, b, a, ia;
                gx = fx * t[0] + fy * t[2] + t[4];
                gy = fx * t[1] + fy * t[3] + t[5];
                gd = sqrtf(gx * gx + gy * gy);
                c = cache->colors[(int)svg_clampf(gd * 255.0f, 0, 255.0f)];
                cr = c.template channel<channel_name::R>();
                cg = c.template channel<channel_name::G>();
                cb = c.template channel<channel_name::B>();
                ca = c.template channel<channel_name::A>();

                a = svg_div255((int)cover[0] * ca);
                ia = 255 - a;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);

                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;

                cover++;
                dst += 4;
                fx += dx;
            }
        }
    } else if (readCallback != nullptr && writeCallback != nullptr) {
        unsigned char tmp[4];
        dst = tmp;
        if (cache->type == svg_paint_type::color) {
            int i, cr, cg, cb, ca;
            cr = cache->colors[0].template channel<channel_name::R>();
            cg = cache->colors[0].template channel<channel_name::G>();
            cb = cache->colors[0].template channel<channel_name::B>();
            ca = cache->colors[0].template channel<channel_name::A>();
            for (i = 0; i < count; i++) {
                int r, g, b;
                int a = svg_div255((int)cover[0] * ca);
                int ia = 255 - a;

                cover++;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);
                readCallback(x + i, y, &dst[0], &dst[1], &dst[2], &dst[3], readCallbackState);
                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;
                writeCallback(x + i, y, dst[0], dst[1], dst[2], dst[3], writeCallbackState);
            }
        } else if (cache->type == svg_paint_type::linear_gradient) {
            // TODO: spread modes.
            // TODO: plenty of opportunities to optimize.
            float fx, fy, dx, gy;
            float* t = cache->xform;
            int i, cr, cg, cb, ca;
            rgba_pixel<32> c;

            fx = ((float)x - tx) / scale;
            fy = ((float)y - ty) / scale;
            dx = 1.0f / scale;

            for (i = 0; i < count; i++) {
                int r, g, b, a, ia;
                gy = fx * t[1] + fy * t[3] + t[5];
                c = cache->colors[(int)svg_clampf(gy * 255.0f, 0, 255.0f)];
                cr = c.template channel<channel_name::R>();
                cg = c.template channel<channel_name::G>();
                cb = c.template channel<channel_name::B>();
                ca = c.template channel<channel_name::A>();

                a = svg_div255((int)cover[0] * ca);
                ia = 255 - a;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);
                readCallback(x + i, y, &dst[0], &dst[1], &dst[2], &dst[3], readCallbackState);
                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;

                cover++;
                writeCallback(x + i, y, dst[0], dst[1], dst[2], dst[3], writeCallbackState);
                fx += dx;
            }
        } else if (cache->type == svg_paint_type::radial_gradient) {
            // TODO: spread modes.
            // TODO: plenty of opportunities to optimize.
            // TODO: focus (fx,fy)
            float fx, fy, dx, gx, gy, gd;
            float* t = cache->xform;
            int i, cr, cg, cb, ca;
            rgba_pixel<32> c;

            fx = ((float)x - tx) / scale;
            fy = ((float)y - ty) / scale;
            dx = 1.0f / scale;

            for (i = 0; i < count; i++) {
                int r, g, b, a, ia;
                gx = fx * t[0] + fy * t[2] + t[4];
                gy = fx * t[1] + fy * t[3] + t[5];
                gd = sqrtf(gx * gx + gy * gy);
                c = cache->colors[(int)svg_clampf(gd * 255.0f, 0, 255.0f)];
                cr = c.template channel<channel_name::R>();
                cg = c.template channel<channel_name::G>();
                cb = c.template channel<channel_name::B>();
                ca = c.template channel<channel_name::A>();

                a = svg_div255((int)cover[0] * ca);
                ia = 255 - a;

                // Premultiply
                r = svg_div255(cr * a);
                g = svg_div255(cg * a);
                b = svg_div255(cb * a);
                readCallback(x + i, y, &dst[0], &dst[1], &dst[2], &dst[3], readCallbackState);
                // Blend over
                r += svg_div255(ia * (int)dst[0]);
                g += svg_div255(ia * (int)dst[1]);
                b += svg_div255(ia * (int)dst[2]);
                a += svg_div255(ia * (int)dst[3]);

                dst[0] = (unsigned char)r;
                dst[1] = (unsigned char)g;
                dst[2] = (unsigned char)b;
                dst[3] = (unsigned char)a;

                cover++;
                writeCallback(x + i, y, dst[0], dst[1], dst[2], dst[3], writeCallbackState);
                fx += dx;
            }
        }
    }
}
static void svg_rasterize_sorted_edges(NSVGrasterizer* r, float tx, float ty, float scale, svg_cached_paint* cache, svg_fill_rule fillRule) {
    svg_active_edge* active = NULL;
    int y, s;
    int e = 0;
    int maxWeight = (255 / NSVG__SUBSAMPLES);  // weight per vertical scanline
    int xmin, xmax;

    for (y = 0; y < r->height; y++) {
        memset(r->scanline, 0, r->width);
        xmin = r->width;
        xmax = 0;
        for (s = 0; s < NSVG__SUBSAMPLES; ++s) {
            // find center of pixel for this scanline
            float scany = (float)(y * NSVG__SUBSAMPLES + s) + 0.5f;
            svg_active_edge** step = &active;

            // update all active edges;
            // remove all active edges that terminate before the center of this scanline
            while (*step) {
                svg_active_edge* z = *step;
                if (z->ey <= scany) {
                    *step = z->next;  // delete from list
                                      //					NSVG__assert(z->valid);
                    svg_free_active(r, z);
                } else {
                    z->x += z->dx;            // advance to position for current scanline
                    step = &((*step)->next);  // advance through list
                }
            }

            // resort the list if needed
            for (;;) {
                int changed = 0;
                step = &active;
                while (*step && (*step)->next) {
                    if ((*step)->x > (*step)->next->x) {
                        svg_active_edge* t = *step;
                        svg_active_edge* q = t->next;
                        t->next = q->next;
                        q->next = t;
                        *step = q;
                        changed = 1;
                    }
                    step = &(*step)->next;
                }
                if (!changed) break;
            }

            // insert all edges that start before the center of this scanline -- omit ones that also end on this scanline
            while (e < r->nedges && r->edges[e].y0 <= scany) {
                if (r->edges[e].y1 > scany) {
                    svg_active_edge* z = svg_add_active(r, &r->edges[e], scany);
                    if (z == NULL) break;
                    // find insertion point
                    if (active == NULL) {
                        active = z;
                    } else if (z->x < active->x) {
                        // insert at front
                        z->next = active;
                        active = z;
                    } else {
                        // find thing to insert AFTER
                        svg_active_edge* p = active;
                        while (p->next && p->next->x < z->x)
                            p = p->next;
                        // at this point, p->next->x is NOT < z->x
                        z->next = p->next;
                        p->next = z;
                    }
                }
                e++;
            }

            // now process all active edges in non-zero fashion
            if (active != NULL)
                svg_fill_active_edges(r->scanline, r->width, active, maxWeight, &xmin, &xmax, fillRule);
        }
        // Blit
        if (xmin < 0) xmin = 0;
        if (xmax > r->width - 1) xmax = r->width - 1;
        if (xmin <= xmax) {
            svg_scanline_solid(r->bitmap != nullptr ? &r->bitmap[y * r->stride] + xmin * 4 : nullptr, r->readCallback, r->readCallbackState, r->writeCallback, r->writeCallbackState, xmax - xmin + 1, &r->scanline[xmin], xmin, y, tx, ty, scale, cache);
        }
    }
}
static void svg_unpremultiply_alpha(unsigned char* image, int w, int h, int stride) {
    int x, y;

    // Unpremultiply
    for (y = 0; y < h; y++) {
        unsigned char* row = &image[y * stride];
        for (x = 0; x < w; x++) {
            int r = row[0], g = row[1], b = row[2], a = row[3];
            if (a != 0) {
                row[0] = (unsigned char)(r * 255 / a);
                row[1] = (unsigned char)(g * 255 / a);
                row[2] = (unsigned char)(b * 255 / a);
            }
            row += 4;
        }
    }

    // Defringe
    for (y = 0; y < h; y++) {
        unsigned char* row = &image[y * stride];
        for (x = 0; x < w; x++) {
            int r = 0, g = 0, b = 0, a = row[3], n = 0;
            if (a == 0) {
                if (x - 1 > 0 && row[-1] != 0) {
                    r += row[-4];
                    g += row[-3];
                    b += row[-2];
                    n++;
                }
                if (x + 1 < w && row[7] != 0) {
                    r += row[4];
                    g += row[5];
                    b += row[6];
                    n++;
                }
                if (y - 1 > 0 && row[-stride + 3] != 0) {
                    r += row[-stride];
                    g += row[-stride + 1];
                    b += row[-stride + 2];
                    n++;
                }
                if (y + 1 < h && row[stride + 3] != 0) {
                    r += row[stride];
                    g += row[stride + 1];
                    b += row[stride + 2];
                    n++;
                }
                if (n > 0) {
                    row[0] = (unsigned char)(r / n);
                    row[1] = (unsigned char)(g / n);
                    row[2] = (unsigned char)(b / n);
                }
            }
            row += 4;
        }
    }
}

static void svg_init_paint(svg_cached_paint* cache, svg_paint* paint, float opacity) {
    int i, j;
    svg_gradient* grad;

    cache->type = paint->type;

    if (paint->type == svg_paint_type::color) {
        cache->colors[0] = svg_apply_opacity(paint->color, opacity);
        return;
    }

    grad = paint->gradient;

    cache->spread = grad->spread;
    memcpy(cache->xform, grad->xform.data, sizeof(float) * 6);

    if (grad->stop_count == 0) {
        for (i = 0; i < 256; i++)
            cache->colors[i]=rgba_pixel<32>();
    }
    if (grad->stop_count == 1) {
        for (i = 0; i < 256; i++)
            cache->colors[i] = svg_apply_opacity(grad->stops[i].color, opacity);
    } else {
        rgba_pixel<32> ca(0,false);
        rgba_pixel<32> cb(0,false);
        float ua, ub, du, u;
        int ia, ib, count;

        ca = svg_apply_opacity(grad->stops[0].color, opacity);
        ua = svg_clampf(grad->stops[0].offset, 0, 1);
        ub = svg_clampf(grad->stops[grad->stop_count - 1].offset, ua, 1);
        ia = (int)(ua * 255.0f);
        ib = (int)(ub * 255.0f);
        for (i = 0; i < ia; i++) {
            cache->colors[i] = ca;
        }

        for (i = 0; i < ((int)grad->stop_count) - 1; i++) {
            ca = svg_apply_opacity(grad->stops[i].color, opacity);
            cb = svg_apply_opacity(grad->stops[i + 1].color, opacity);
            ua = svg_clampf(grad->stops[i].offset, 0, 1);
            ub = svg_clampf(grad->stops[i + 1].offset, 0, 1);
            ia = (int)(ua * 255.0f);
            ib = (int)(ub * 255.0f);
            count = ib - ia;
            if (count <= 0) continue;
            u = 0;
            du = 1.0f / (float)count;
            for (j = 0; j < count; j++) {
                cache->colors[ia + j] = svg_lerp_rgba(ca, cb, u);
                u += du;
            }
        }

        for (i = ib; i < 256; i++)
            cache->colors[i] = cb;
    }
}
gfx_result svg_rasterize(NSVGrasterizer* r,
                   svg_image* image, float tx, float ty, float scale,
                   unsigned char* dst, NSVGrasterizerReadCallback readCallback, void* readCallbackState, NSVGrasterizerWriteCallback writeCallback, void* writeCallbackState, int w, int h, int stride) {
    svg_shape* shape = NULL;
    svg_edge* e = NULL;
    svg_cached_paint cache;
    int i;
    r->readCallback = readCallback;
    r->readCallbackState = readCallbackState;
    r->writeCallback = writeCallback;
    r->writeCallbackState = writeCallbackState;
    r->bitmap = dst;
    r->width = w;
    r->height = h;
    r->stride = stride;

    if (w > r->cscanline) {
        r->cscanline = w;
        r->scanline = (unsigned char*)r->reallocator(r->scanline, w);
        if (r->scanline == NULL) return gfx_result::out_of_memory;
    }

    if (dst != nullptr) {
        for (i = 0; i < h; i++) memset(&dst[i * stride], 0, w * 4);
    }
    for (shape = image->shapes; shape != NULL; shape = shape->next) {
        if (!(shape->flags & SVG_FLAGS_VISIBLE))
            continue;

        if (shape->fill.type != svg_paint_type::none) {
            svg_reset_pool(r);
            r->freelist = NULL;
            r->nedges = 0;

            svg_flatten_shape(r, shape, scale);

            // Scale and translate edges
            for (i = 0; i < r->nedges; i++) {
                e = &r->edges[i];
                e->x0 = tx + e->x0;
                e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
                e->x1 = tx + e->x1;
                e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
            }

            // Rasterize edges
            if (r->nedges != 0)
                qsort(r->edges, r->nedges, sizeof(svg_edge), svg_cmp_edge);

            // now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
            svg_init_paint(&cache, &shape->fill, shape->opacity);

            svg_rasterize_sorted_edges(r, tx, ty, scale, &cache, shape->fill_rule);
        }
        if (shape->stroke.type != svg_paint_type::none && (shape->stroke_width * scale) > 0.01f) {
            svg_reset_pool(r);
            r->freelist = NULL;
            r->nedges = 0;

            svg_flatten_shape_stroke(r, shape, scale);

            //			dumpEdges(r, "edge.svg");

            // Scale and translate edges
            for (i = 0; i < r->nedges; i++) {
                e = &r->edges[i];
                e->x0 = tx + e->x0;
                e->y0 = (ty + e->y0) * NSVG__SUBSAMPLES;
                e->x1 = tx + e->x1;
                e->y1 = (ty + e->y1) * NSVG__SUBSAMPLES;
            }

            // Rasterize edges
            if (r->nedges != 0)
                qsort(r->edges, r->nedges, sizeof(svg_edge), svg_cmp_edge);

            // now, traverse the scanlines and find the intersections on each scanline, use non-zero rule
            svg_init_paint(&cache, &shape->stroke, shape->opacity);

            svg_rasterize_sorted_edges(r, tx, ty, scale, &cache, svg_fill_rule::non_zero);
        }
    }
    if (dst != nullptr) {
        svg_unpremultiply_alpha(dst, w, h, stride);
    }
    r->bitmap = NULL;
    r->width = 0;
    r->height = 0;
    r->stride = 0;
    return gfx_result::success;
}
gfx_result svg_create_rasterizer(NSVGrasterizer** out_rasterizer, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    NSVGrasterizer* r = (NSVGrasterizer*)allocator(sizeof(NSVGrasterizer));
    if (r == NULL) return gfx_result::out_of_memory;
    memset(r, 0, sizeof(NSVGrasterizer));
    r->allocator = allocator;
    r->reallocator = reallocator;
    r->deallocator = deallocator;

    r->tessTol = 0.25f;
    r->distTol = 0.01f;
    *out_rasterizer = r;
    return gfx_result::success;

}

void svg_delete_rasterizer(NSVGrasterizer* r) {
    svg_mem_page* p;

    if (r == NULL) return;

    p = r->pages;
    while (p != NULL) {
        svg_mem_page* next = p->next;
        r->deallocator(p);
        p = next;
    }

    if (r->edges) r->deallocator(r->edges);
    if (r->points) r->deallocator(r->points);
    if (r->points2) r->deallocator(r->points2);
    if (r->scanline) r->deallocator(r->scanline);

    r->deallocator(r);
}
}