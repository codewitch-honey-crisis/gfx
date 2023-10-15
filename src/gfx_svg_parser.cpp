#include <gfx_svg.hpp>
#include "svg_private.hpp"
#include <ml_reader.hpp>
using namespace ml;

#define NSVG_MAX_ATTR 128
#define NSVG_PI (3.14159265358979323846264338327f)
#define NSVG_KAPPA90 (0.5522847493f)  // Length proportional to radius of a cubic bezier handle for 90deg arcs.
#define NSVG_RGB(r, g, b) (gfx::rgba_pixel<32>(r,g,b,255))
#define NSVG_ALIGN_MIN 0
#define NSVG_ALIGN_MID 1
#define NSVG_ALIGN_MAX 2
#define NSVG_ALIGN_NONE 0
#define NSVG_ALIGN_MEET 1
#define NSVG_ALIGN_SLICE 2

#define NSVG_MAX_DASHES 8

namespace gfx {
enum NSVGgradientUnits {
    NSVG_USER_SPACE = 0,
    NSVG_OBJECT_SPACE = 1
};

enum struct NSVGunits : uint8_t {
    NSVG_UNITS_USER,
    NSVG_UNITS_PX,
    NSVG_UNITS_PT,
    NSVG_UNITS_PC,
    NSVG_UNITS_MM,
    NSVG_UNITS_CM,
    NSVG_UNITS_IN,
    NSVG_UNITS_PERCENT,
    NSVG_UNITS_EM,
    NSVG_UNITS_EX
};

typedef struct NSVGcoordinate {
    float value;
    NSVGunits units;
} NSVGcoordinate;

typedef struct NSVGlinearData {
    NSVGcoordinate x1, y1, x2, y2;
} NSVGlinearData;

typedef struct NSVGradialData {
    NSVGcoordinate cx, cy, r, fx, fy;
} NSVGradialData;

typedef struct NSVGgradientData {
    char id[64];
    char ref[64];
    svg_paint_type type;
    union {
        NSVGlinearData linear;
        NSVGradialData radial;
    };
    svg_spread_type spread;
    char units;
    float xform[6];
    int nstops;
    svg_gradient_stop* stops;
    struct NSVGgradientData* next;
} NSVGgradientData;

typedef struct NSVGattrib {
    char id[64];
    float xform[6];
    gfx::rgba_pixel<32> fillColor;
    gfx::rgba_pixel<32> strokeColor;
    float opacity;
    float fillOpacity;
    float strokeOpacity;
    char fillGradient[64];
    char strokeGradient[64];
    float strokeWidth;
    float strokeDashOffset;
    float strokeDashArray[NSVG_MAX_DASHES];
    int strokeDashCount;
    svg_line_join strokeLineJoin;
    svg_line_cap strokeLineCap;
    float miterLimit;
    svg_fill_rule fillRule;
    float fontSize;
    gfx::rgba_pixel<32> stopColor;
    float stopOpacity;
    float stopOffset;
    char hasFill;
    char hasStroke;
    char visible;
} NSVGattrib;
using reader_t = raw_ml_reader<2048>;
struct svg_css_class {
    char selector[512];
    char* value;
    svg_css_class* next;
};
struct svg_parse_result {
    reader_t* reader;
    svg_css_class* css_classes;
    svg_css_class* css_class_tail;
    void* (*allocator)(size_t);
    void* (*reallocator)(void*, size_t);
    void (*deallocator)(void*);
    char lname[32];
    char aname[512];
    char avalue[512];
    char style_val[256];
    char class_val[128];
    char* d;
    size_t d_size;
    NSVGattrib attr[NSVG_MAX_ATTR];
    int attrHead;
    float* pts;
    int npts;
    int cpts;
    svg_path* plist;
    size_t image_size;
    svg_image* image;
    NSVGgradientData* gradients;
    svg_shape* shapesTail;
    float viewMinx, viewMiny, viewWidth, viewHeight;
    int alignX, alignY, alignType;
    float dpi;
    char pathFlag;
    char defsFlag;
};
static int svg_parse_name_value(svg_parse_result& p, const char* start, const char* end);
static void svg_parse_style(svg_parse_result& p, const char* str);
static int svg_parse_stroke_dash_array(svg_parse_result& p, const char* str, float* strokeDashArray);
static float svg_parse_coordinate(svg_parse_result& p, const char* str, float orig, float length);
static float svg_actual_orig_x(svg_parse_result& p);
static float svg_actual_orig_y(svg_parse_result& p);
static float svg_actual_width(svg_parse_result& p);
static float svg_actual_height(svg_parse_result& p);
static float svg_actual_length(svg_parse_result& p);
static int svg_isspace(char c) {
    return strchr(" \t\n\v\f\r", c) != 0;
}

static int svg_isdigit(char c) {
    return c >= '0' && c <= '9';
}

// We roll our own string to float because the std library one uses locale and messes things up.
static double svg_atof(const char* s) {
    char* cur = (char*)s;
    char* end = NULL;
    double res = 0.0, sign = 1.0;
    long long intPart = 0, fracPart = 0;
    char hasIntPart = 0, hasFracPart = 0;

    // Parse optional sign
    if (*cur == '+') {
        cur++;
    } else if (*cur == '-') {
        sign = -1;
        cur++;
    }

    // Parse integer part
    if (svg_isdigit(*cur)) {
        // Parse digit sequence
        intPart = strtoll(cur, &end, 10);
        if (cur != end) {
            res = (double)intPart;
            hasIntPart = 1;
            cur = end;
        }
    }

    // Parse fractional part.
    if (*cur == '.') {
        cur++;  // Skip '.'
        if (svg_isdigit(*cur)) {
            // Parse digit sequence
            fracPart = strtoll(cur, &end, 10);
            if (cur != end) {
                res += (double)fracPart / pow(10.0, (double)(end - cur));
                hasFracPart = 1;
                cur = end;
            }
        }
    }

    // A valid number should have integer or fractional part.
    if (!hasIntPart && !hasFracPart)
        return 0.0;

    // Parse optional exponent
    if (*cur == 'e' || *cur == 'E') {
        long expPart = 0;
        cur++;                            // skip 'E'
        expPart = strtol(cur, &end, 10);  // Parse digit sequence with sign
        if (cur != end) {
            res *= pow(10.0, (double)expPart);
        }
    }

    return res * sign;
}
static const char* svg_parse_number(const char* s, char* it, const int size) {
    const int last = size - 1;
    int i = 0;

    // sign
    if (*s == '-' || *s == '+') {
        if (i < last) it[i++] = *s;
        s++;
    }
    // integer part
    while (*s && svg_isdigit(*s)) {
        if (i < last) it[i++] = *s;
        s++;
    }
    if (*s == '.') {
        // decimal point
        if (i < last) it[i++] = *s;
        s++;
        // fraction part
        while (*s && svg_isdigit(*s)) {
            if (i < last) it[i++] = *s;
            s++;
        }
    }
    // exponent
    if ((*s == 'e' || *s == 'E') && (s[1] != 'm' && s[1] != 'x')) {
        if (i < last) it[i++] = *s;
        s++;
        if (*s == '-' || *s == '+') {
            if (i < last) it[i++] = *s;
            s++;
        }
        while (*s && svg_isdigit(*s)) {
            if (i < last) it[i++] = *s;
            s++;
        }
    }
    it[i] = '\0';

    return s;
}
static const char* svg_get_next_path_item_when_arc_flag(const char* s, char* it) {
    it[0] = '\0';
    while (*s && (svg_isspace(*s) || *s == ',')) s++;
    if (!*s) return s;
    if (*s == '0' || *s == '1') {
        it[0] = *s++;
        it[1] = '\0';
        return s;
    }
    return s;
}

static const char* svg_get_next_path_item(const char* s, char* it) {
    it[0] = '\0';
    // Skip white spaces and commas
    while (*s && (svg_isspace(*s) || *s == ',')) s++;
    if (!*s) return s;
    if (*s == '-' || *s == '+' || *s == '.' || svg_isdigit(*s)) {
        s = svg_parse_number(s, it, 64);
    } else {
        // Parse command
        it[0] = *s++;
        it[1] = '\0';
        return s;
    }

    return s;
}

static int svg_parse_transform_args(const char* str, float* args, int maxNa, int* na) {
    const char* end;
    const char* ptr;
    char it[64];

    *na = 0;
    ptr = str;
    while (*ptr && *ptr != '(') ++ptr;
    if (*ptr == 0)
        return 1;
    end = ptr;
    while (*end && *end != ')') ++end;
    if (*end == 0)
        return 1;

    while (ptr < end) {
        if (*ptr == '-' || *ptr == '+' || *ptr == '.' || svg_isdigit(*ptr)) {
            if (*na >= maxNa) return 0;
            ptr = svg_parse_number(ptr, it, 64);
            args[(*na)++] = (float)svg_atof(it);
        } else {
            ++ptr;
        }
    }
    return (int)(end - str);
}

static int svg_parse_matrix(float* xform, const char* str) {
    float t[6];
    int na = 0;
    int len = svg_parse_transform_args(str, t, 6, &na);
    if (na != 6) return len;
    memcpy(xform, t, sizeof(float) * 6);
    return len;
}

static int svg_parse_translate(float* xform, const char* str) {
    float args[2];
    float t[6];
    int na = 0;
    int len = svg_parse_transform_args(str, args, 2, &na);
    if (na == 1) args[1] = 0.0;

    svg_xform_set_translation(t, args[0], args[1]);
    memcpy(xform, t, sizeof(float) * 6);
    return len;
}

static int svg_parse_scale(float* xform, const char* str) {
    float args[2];
    int na = 0;
    float t[6];
    int len = svg_parse_transform_args(str, args, 2, &na);
    if (na == 1) args[1] = args[0];
    svg_xform_set_scale(t, args[0], args[1]);
    memcpy(xform, t, sizeof(float) * 6);
    return len;
}

static int svg_parse_skew_x(float* xform, const char* str) {
    float args[1];
    int na = 0;
    float t[6];
    int len = svg_parse_transform_args(str, args, 1, &na);
    svg_xform_set_skew_x(t, args[0] / 180.0f * NSVG_PI);
    memcpy(xform, t, sizeof(float) * 6);
    return len;
}

static int svg_parse_skew_y(float* xform, const char* str) {
    float args[1];
    int na = 0;
    float t[6];
    int len = svg_parse_transform_args(str, args, 1, &na);
    svg_xform_set_skew_y(t, args[0] / 180.0f * NSVG_PI);
    memcpy(xform, t, sizeof(float) * 6);
    return len;
}

static int svg_parse_rotate(float* xform, const char* str) {
    float args[3];
    int na = 0;
    float m[6];
    float t[6];
    int len = svg_parse_transform_args(str, args, 3, &na);
    if (na == 1)
        args[1] = args[2] = 0.0f;
    svg_xform_identity(m);

    if (na > 1) {
        svg_xform_set_translation(t, -args[1], -args[2]);
        svg_xform_multiply(m, t);
    }

    svg_xform_set_rotation(t, args[0] / 180.0f * NSVG_PI);
    svg_xform_multiply(m, t);

    if (na > 1) {
        svg_xform_set_translation(t, args[1], args[2]);
        svg_xform_multiply(m, t);
    }

    memcpy(xform, m, sizeof(float) * 6);

    return len;
}

static void svg_parse_transform(float* xform, const char* str) {
    float t[6];
    int len;
    svg_xform_identity(xform);
    while (*str) {
        if (strncmp(str, "matrix", 6) == 0)
            len = svg_parse_matrix(t, str);
        else if (strncmp(str, "translate", 9) == 0)
            len = svg_parse_translate(t, str);
        else if (strncmp(str, "scale", 5) == 0)
            len = svg_parse_scale(t, str);
        else if (strncmp(str, "rotate", 6) == 0)
            len = svg_parse_rotate(t, str);
        else if (strncmp(str, "skewX", 5) == 0)
            len = svg_parse_skew_x(t, str);
        else if (strncmp(str, "skewY", 5) == 0)
            len = svg_parse_skew_y(t, str);
        else {
            ++str;
            continue;
        }
        if (len != 0) {
            str += len;
        } else {
            ++str;
            continue;
        }

        svg_xform_premultiply(xform, t);
    }
}

static void svg_parse_url(char* id, const char* str) {
    int i = 0;
    str += 4;  // "url(";
    if (*str && *str == '#')
        str++;
    while (i < 63 && *str && *str != ')') {
        id[i] = *str++;
        i++;
    }
    id[i] = '\0';
}

static gfx_result svg_push_attr(svg_parse_result& p) {
    if (p.attrHead < NSVG_MAX_ATTR - 1) {
        // printf("svg_push_attr\n");
        p.attrHead++;
        memcpy(&p.attr[p.attrHead], &p.attr[p.attrHead - 1], sizeof(NSVGattrib));
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}

static void svg_pop_attr(svg_parse_result& p) {
    // printf("svg_pop_attr\n");
    if (p.attrHead > 0)
        p.attrHead--;
}
static NSVGattrib* svg_get_attr(svg_parse_result& p) {
    return &p.attr[p.attrHead];
}
static void svg_delete_gradient_data(NSVGgradientData* grad, void(deallocator)(void*)) {
    NSVGgradientData* next;
    while (grad != NULL) {
        next = grad->next;
        deallocator(grad->stops);
        deallocator(grad);
        grad = next;
    }
}

static void svg_delete_parse_result(svg_parse_result& p) {
    svg_css_class* cls = p.css_classes;
    while (cls != nullptr) {
        if (cls->value != nullptr) {
            p.deallocator(cls->value);
        }
        svg_css_class* prev = cls;
        cls = cls->next;
        p.deallocator(prev);
    }
    p.css_classes = nullptr;
    svg_delete_paths(p.plist, p.deallocator);
    svg_delete_gradient_data(p.gradients, p.deallocator);
    if (p.d != nullptr) {
        p.deallocator(p.d);
    }
    svg_delete_image(p.image, p.deallocator);
    p.deallocator(p.pts);
    p.deallocator(&p);
}
static svg_css_class* svg_find_next_css_class(svg_css_class* start, const char* name) {
    size_t nl = strlen(name);
    while (start != nullptr) {
        const char* sz = strstr(start->selector, name);
        if (sz > start->selector) {
            if ((*(sz + nl) == '\0' || *(sz + nl) == ',') && *(sz - 1) == '.') {
                return start;
            }
        }
        start = start->next;
    }
    return nullptr;
}
static gfx_result svg_apply_classes(svg_parse_result& p, const char* classes) {
    const char* ss = classes;
    char cn[64];
    char* sz;
    while (*ss) {
        sz = cn;
        *sz = '\0';
        while (*ss && svg_isspace(*ss)) {
            ++ss;
        }
        if (!*ss) return gfx_result::invalid_format;
        int i = sizeof(cn) - 1;
        while (i && *ss && !svg_isspace(*ss)) {
            *(sz++) = *(ss++);
            --i;
        }
        *sz = '\0';
        if (p.css_classes != nullptr && *cn) {
            svg_css_class* cls = svg_find_next_css_class(p.css_classes, cn);
            while (cls != nullptr) {
                svg_parse_style(p, cls->value);
                cls = svg_find_next_css_class(cls->next, cn);
            }
        }
    }
    return gfx_result::success;
}
static gfx::rgba_pixel<32> svg_parse_color_hex(const char* str) {
    unsigned int r = 0, g = 0, b = 0;
    if (sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3)  // 2 digit hex
        return NSVG_RGB(r, g, b);
    if (sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3)  // 1 digit hex, e.g. #abc -> 0xccbbaa
        return NSVG_RGB(r * 17, g * 17, b * 17);     // same effect as (r<<4|r), (g<<4|g), ..
    return NSVG_RGB(128, 128, 128);
}

// Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
// This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
// for backwards compatibility. Note: other image viewers return black instead.

static gfx::rgba_pixel<32> svg_parse_color_rgb(const char* str) {
    int i;
    unsigned int rgbi[3];
    float rgbf[3];
    // try decimal integers first
    if (sscanf(str, "rgb(%u, %u, %u)", &rgbi[0], &rgbi[1], &rgbi[2]) != 3) {
        // integers failed, try percent values (float, locale independent)
        const char delimiter[3] = {',', ',', ')'};
        str += 4;  // skip "rgb("
        for (i = 0; i < 3; i++) {
            while (*str && (svg_isspace(*str))) str++;  // skip leading spaces
            if (*str == '+') str++;                       // skip '+' (don't allow '-')
            if (!*str) break;
            rgbf[i] = svg_atof(str);

            // Note 1: it would be great if svg_atof() returned how many
            // bytes it consumed but it doesn't. We need to skip the number,
            // the '%' character, spaces, and the delimiter ',' or ')'.

            // Note 2: The following code does not allow values like "33.%",
            // i.e. a decimal point w/o fractional part, but this is consistent
            // with other image viewers, e.g. firefox, chrome, eog, gimp.

            while (*str && svg_isdigit(*str)) str++;  // skip integer part
            if (*str == '.') {
                str++;
                if (!svg_isdigit(*str)) break;            // error: no digit after '.'
                while (*str && svg_isdigit(*str)) str++;  // skip fractional part
            }
            if (*str == '%')
                str++;
            else
                break;
            while (svg_isspace(*str)) str++;
            if (*str == delimiter[i])
                str++;
            else
                break;
        }
        if (i == 3) {
            rgbi[0] = roundf(rgbf[0] * 2.55f);
            rgbi[1] = roundf(rgbf[1] * 2.55f);
            rgbi[2] = roundf(rgbf[2] * 2.55f);
        } else {
            rgbi[0] = rgbi[1] = rgbi[2] = 128;
        }
    }
    // clip values as the CSS spec requires
    for (i = 0; i < 3; i++) {
        if (rgbi[i] > 255) rgbi[i] = 255;
    }
    return NSVG_RGB(rgbi[0], rgbi[1], rgbi[2]);
}

typedef struct NSVGNamedColor {
    const char* name;
    gfx::rgba_pixel<32> color;
} NSVGNamedColor;

static NSVGNamedColor svg_colors[] = {

    {"red", NSVG_RGB(255, 0, 0)},
    {"green", NSVG_RGB(0, 128, 0)},
    {"blue", NSVG_RGB(0, 0, 255)},
    {"yellow", NSVG_RGB(255, 255, 0)},
    {"cyan", NSVG_RGB(0, 255, 255)},
    {"magenta", NSVG_RGB(255, 0, 255)},
    {"black", NSVG_RGB(0, 0, 0)},
    {"grey", NSVG_RGB(128, 128, 128)},
    {"gray", NSVG_RGB(128, 128, 128)},
    {"white", NSVG_RGB(255, 255, 255)},

#ifdef GFX_SVG_ALL_NAMED_COLORS
    {"aliceblue", NSVG_RGB(240, 248, 255)},
    {"antiquewhite", NSVG_RGB(250, 235, 215)},
    {"aqua", NSVG_RGB(0, 255, 255)},
    {"aquamarine", NSVG_RGB(127, 255, 212)},
    {"azure", NSVG_RGB(240, 255, 255)},
    {"beige", NSVG_RGB(245, 245, 220)},
    {"bisque", NSVG_RGB(255, 228, 196)},
    {"blanchedalmond", NSVG_RGB(255, 235, 205)},
    {"blueviolet", NSVG_RGB(138, 43, 226)},
    {"brown", NSVG_RGB(165, 42, 42)},
    {"burlywood", NSVG_RGB(222, 184, 135)},
    {"cadetblue", NSVG_RGB(95, 158, 160)},
    {"chartreuse", NSVG_RGB(127, 255, 0)},
    {"chocolate", NSVG_RGB(210, 105, 30)},
    {"coral", NSVG_RGB(255, 127, 80)},
    {"cornflowerblue", NSVG_RGB(100, 149, 237)},
    {"cornsilk", NSVG_RGB(255, 248, 220)},
    {"crimson", NSVG_RGB(220, 20, 60)},
    {"darkblue", NSVG_RGB(0, 0, 139)},
    {"darkcyan", NSVG_RGB(0, 139, 139)},
    {"darkgoldenrod", NSVG_RGB(184, 134, 11)},
    {"darkgray", NSVG_RGB(169, 169, 169)},
    {"darkgreen", NSVG_RGB(0, 100, 0)},
    {"darkgrey", NSVG_RGB(169, 169, 169)},
    {"darkkhaki", NSVG_RGB(189, 183, 107)},
    {"darkmagenta", NSVG_RGB(139, 0, 139)},
    {"darkolivegreen", NSVG_RGB(85, 107, 47)},
    {"darkorange", NSVG_RGB(255, 140, 0)},
    {"darkorchid", NSVG_RGB(153, 50, 204)},
    {"darkred", NSVG_RGB(139, 0, 0)},
    {"darksalmon", NSVG_RGB(233, 150, 122)},
    {"darkseagreen", NSVG_RGB(143, 188, 143)},
    {"darkslateblue", NSVG_RGB(72, 61, 139)},
    {"darkslategray", NSVG_RGB(47, 79, 79)},
    {"darkslategrey", NSVG_RGB(47, 79, 79)},
    {"darkturquoise", NSVG_RGB(0, 206, 209)},
    {"darkviolet", NSVG_RGB(148, 0, 211)},
    {"deeppink", NSVG_RGB(255, 20, 147)},
    {"deepskyblue", NSVG_RGB(0, 191, 255)},
    {"dimgray", NSVG_RGB(105, 105, 105)},
    {"dimgrey", NSVG_RGB(105, 105, 105)},
    {"dodgerblue", NSVG_RGB(30, 144, 255)},
    {"firebrick", NSVG_RGB(178, 34, 34)},
    {"floralwhite", NSVG_RGB(255, 250, 240)},
    {"forestgreen", NSVG_RGB(34, 139, 34)},
    {"fuchsia", NSVG_RGB(255, 0, 255)},
    {"gainsboro", NSVG_RGB(220, 220, 220)},
    {"ghostwhite", NSVG_RGB(248, 248, 255)},
    {"gold", NSVG_RGB(255, 215, 0)},
    {"goldenrod", NSVG_RGB(218, 165, 32)},
    {"greenyellow", NSVG_RGB(173, 255, 47)},
    {"honeydew", NSVG_RGB(240, 255, 240)},
    {"hotpink", NSVG_RGB(255, 105, 180)},
    {"indianred", NSVG_RGB(205, 92, 92)},
    {"indigo", NSVG_RGB(75, 0, 130)},
    {"ivory", NSVG_RGB(255, 255, 240)},
    {"khaki", NSVG_RGB(240, 230, 140)},
    {"lavender", NSVG_RGB(230, 230, 250)},
    {"lavenderblush", NSVG_RGB(255, 240, 245)},
    {"lawngreen", NSVG_RGB(124, 252, 0)},
    {"lemonchiffon", NSVG_RGB(255, 250, 205)},
    {"lightblue", NSVG_RGB(173, 216, 230)},
    {"lightcoral", NSVG_RGB(240, 128, 128)},
    {"lightcyan", NSVG_RGB(224, 255, 255)},
    {"lightgoldenrodyellow", NSVG_RGB(250, 250, 210)},
    {"lightgray", NSVG_RGB(211, 211, 211)},
    {"lightgreen", NSVG_RGB(144, 238, 144)},
    {"lightgrey", NSVG_RGB(211, 211, 211)},
    {"lightpink", NSVG_RGB(255, 182, 193)},
    {"lightsalmon", NSVG_RGB(255, 160, 122)},
    {"lightseagreen", NSVG_RGB(32, 178, 170)},
    {"lightskyblue", NSVG_RGB(135, 206, 250)},
    {"lightslategray", NSVG_RGB(119, 136, 153)},
    {"lightslategrey", NSVG_RGB(119, 136, 153)},
    {"lightsteelblue", NSVG_RGB(176, 196, 222)},
    {"lightyellow", NSVG_RGB(255, 255, 224)},
    {"lime", NSVG_RGB(0, 255, 0)},
    {"limegreen", NSVG_RGB(50, 205, 50)},
    {"linen", NSVG_RGB(250, 240, 230)},
    {"maroon", NSVG_RGB(128, 0, 0)},
    {"mediumaquamarine", NSVG_RGB(102, 205, 170)},
    {"mediumblue", NSVG_RGB(0, 0, 205)},
    {"mediumorchid", NSVG_RGB(186, 85, 211)},
    {"mediumpurple", NSVG_RGB(147, 112, 219)},
    {"mediumseagreen", NSVG_RGB(60, 179, 113)},
    {"mediumslateblue", NSVG_RGB(123, 104, 238)},
    {"mediumspringgreen", NSVG_RGB(0, 250, 154)},
    {"mediumturquoise", NSVG_RGB(72, 209, 204)},
    {"mediumvioletred", NSVG_RGB(199, 21, 133)},
    {"midnightblue", NSVG_RGB(25, 25, 112)},
    {"mintcream", NSVG_RGB(245, 255, 250)},
    {"mistyrose", NSVG_RGB(255, 228, 225)},
    {"moccasin", NSVG_RGB(255, 228, 181)},
    {"navajowhite", NSVG_RGB(255, 222, 173)},
    {"navy", NSVG_RGB(0, 0, 128)},
    {"oldlace", NSVG_RGB(253, 245, 230)},
    {"olive", NSVG_RGB(128, 128, 0)},
    {"olivedrab", NSVG_RGB(107, 142, 35)},
    {"orange", NSVG_RGB(255, 165, 0)},
    {"orangered", NSVG_RGB(255, 69, 0)},
    {"orchid", NSVG_RGB(218, 112, 214)},
    {"palegoldenrod", NSVG_RGB(238, 232, 170)},
    {"palegreen", NSVG_RGB(152, 251, 152)},
    {"paleturquoise", NSVG_RGB(175, 238, 238)},
    {"palevioletred", NSVG_RGB(219, 112, 147)},
    {"papayawhip", NSVG_RGB(255, 239, 213)},
    {"peachpuff", NSVG_RGB(255, 218, 185)},
    {"peru", NSVG_RGB(205, 133, 63)},
    {"pink", NSVG_RGB(255, 192, 203)},
    {"plum", NSVG_RGB(221, 160, 221)},
    {"powderblue", NSVG_RGB(176, 224, 230)},
    {"purple", NSVG_RGB(128, 0, 128)},
    {"rosybrown", NSVG_RGB(188, 143, 143)},
    {"royalblue", NSVG_RGB(65, 105, 225)},
    {"saddlebrown", NSVG_RGB(139, 69, 19)},
    {"salmon", NSVG_RGB(250, 128, 114)},
    {"sandybrown", NSVG_RGB(244, 164, 96)},
    {"seagreen", NSVG_RGB(46, 139, 87)},
    {"seashell", NSVG_RGB(255, 245, 238)},
    {"sienna", NSVG_RGB(160, 82, 45)},
    {"silver", NSVG_RGB(192, 192, 192)},
    {"skyblue", NSVG_RGB(135, 206, 235)},
    {"slateblue", NSVG_RGB(106, 90, 205)},
    {"slategray", NSVG_RGB(112, 128, 144)},
    {"slategrey", NSVG_RGB(112, 128, 144)},
    {"snow", NSVG_RGB(255, 250, 250)},
    {"springgreen", NSVG_RGB(0, 255, 127)},
    {"steelblue", NSVG_RGB(70, 130, 180)},
    {"tan", NSVG_RGB(210, 180, 140)},
    {"teal", NSVG_RGB(0, 128, 128)},
    {"thistle", NSVG_RGB(216, 191, 216)},
    {"tomato", NSVG_RGB(255, 99, 71)},
    {"turquoise", NSVG_RGB(64, 224, 208)},
    {"violet", NSVG_RGB(238, 130, 238)},
    {"wheat", NSVG_RGB(245, 222, 179)},
    {"whitesmoke", NSVG_RGB(245, 245, 245)},
    {"yellowgreen", NSVG_RGB(154, 205, 50)},
#endif
};

static gfx::rgba_pixel<32> svg_parse_color_name(const char* str) {
    int i, ncolors = sizeof(svg_colors) / sizeof(NSVGNamedColor);

    for (i = 0; i < ncolors; i++) {
        if (strcmp(svg_colors[i].name, str) == 0) {
            return svg_colors[i].color;
        }
    }

    return NSVG_RGB(128, 128, 128);
}

static gfx::rgba_pixel<32> svg_parse_color(const char* str) {
    size_t len = 0;
    while (*str == ' ') ++str;
    len = strlen(str);
    if (len >= 1 && *str == '#')
        return svg_parse_color_hex(str);
    else if (len >= 4 && str[0] == 'r' && str[1] == 'g' && str[2] == 'b' && str[3] == '(')
        return svg_parse_color_rgb(str);
    return svg_parse_color_name(str);
}

static float svg_parse_opacity(const char* str) {
    float val = svg_atof(str);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    return val;
}

static float svg_parse_miter_limit(const char* str) {
    float val = svg_atof(str);
    if (val < 0.0f) val = 0.0f;
    return val;
}

static NSVGunits svg_parse_units(const char* units) {
    if (units[0] == 'p' && units[1] == 'x')
        return NSVGunits::NSVG_UNITS_PX;
    else if (units[0] == 'p' && units[1] == 't')
        return NSVGunits::NSVG_UNITS_PT;
    else if (units[0] == 'p' && units[1] == 'c')
        return NSVGunits::NSVG_UNITS_PC;
    else if (units[0] == 'm' && units[1] == 'm')
        return NSVGunits::NSVG_UNITS_MM;
    else if (units[0] == 'c' && units[1] == 'm')
        return NSVGunits::NSVG_UNITS_CM;
    else if (units[0] == 'i' && units[1] == 'n')
        return NSVGunits::NSVG_UNITS_IN;
    else if (units[0] == '%')
        return NSVGunits::NSVG_UNITS_PERCENT;
    else if (units[0] == 'e' && units[1] == 'm')
        return NSVGunits::NSVG_UNITS_EM;
    else if (units[0] == 'e' && units[1] == 'x')
        return NSVGunits::NSVG_UNITS_EX;
    return NSVGunits::NSVG_UNITS_USER;
}

static svg_line_cap svg_parse_line_cap(const char* str) {
    if (strcmp(str, "butt") == 0)
        return svg_line_cap::butt;
    else if (strcmp(str, "round") == 0)
        return svg_line_cap::round;
    else if (strcmp(str, "square") == 0)
        return svg_line_cap::square;
    // TODO: handle inherit.
    return svg_line_cap::butt;
}

static svg_line_join svg_parse_line_join(const char* str) {
    if (strcmp(str, "miter") == 0)
        return svg_line_join::miter;
    else if (strcmp(str, "round") == 0)
        return svg_line_join::round;
    else if (strcmp(str, "bevel") == 0)
        return svg_line_join::bevel;
    // TODO: handle inherit.
    return svg_line_join::miter;
}

static svg_fill_rule svg_parse_fill_rule(const char* str) {
    if (strcmp(str, "nonzero") == 0)
        return svg_fill_rule::non_zero;
    else if (strcmp(str, "evenodd") == 0)
        return svg_fill_rule::even_odd;
    // TODO: handle inherit.
    return svg_fill_rule::non_zero;
}

static const char* svg_get_next_dash_item(const char* s, char* it) {
    int n = 0;
    it[0] = '\0';
    // Skip white spaces and commas
    while (*s && (svg_isspace(*s) || *s == ',')) s++;
    // Advance until whitespace, comma or end.
    while (*s && (!svg_isspace(*s) && *s != ',')) {
        if (n < 63)
            it[n++] = *s;
        s++;
    }
    it[n++] = '\0';
    return s;
}

static int svg_parse_attr(svg_parse_result& p, const char* name, const char* value) {
    float xform[6];
    NSVGattrib* attr = svg_get_attr(p);
    if (!attr) return 0;
    if (strcmp(name, "class") == 0) {
        svg_apply_classes(p, value);
    }
    if (strcmp(name, "style") == 0) {
        svg_parse_style(p, value);
    } else if (strcmp(name, "display") == 0) {
        if (strcmp(value, "none") == 0)
            attr->visible = 0;
        // Don't reset ->visible on display:inline, one display:none hides the whole subtree

    } else if (strcmp(name, "fill") == 0) {
        if (strcmp(value, "none") == 0) {
            attr->hasFill = 0;
        } else if (strncmp(value, "url(", 4) == 0) {
            attr->hasFill = 2;
            svg_parse_url(attr->fillGradient, value);
        } else {
            attr->hasFill = 1;
            attr->fillColor = svg_parse_color(value);
        }
    } else if (strcmp(name, "opacity") == 0) {
        attr->opacity = svg_parse_opacity(value);
    } else if (strcmp(name, "fill-opacity") == 0) {
        attr->fillOpacity = svg_parse_opacity(value);
    } else if (strcmp(name, "stroke") == 0) {
        if (strcmp(value, "none") == 0) {
            attr->hasStroke = 0;
        } else if (strncmp(value, "url(", 4) == 0) {
            attr->hasStroke = 2;
            svg_parse_url(attr->strokeGradient, value);
        } else {
            attr->hasStroke = 1;
            attr->strokeColor = svg_parse_color(value);
        }
    } else if (strcmp(name, "stroke-width") == 0) {
        attr->strokeWidth = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "stroke-dasharray") == 0) {
        attr->strokeDashCount = svg_parse_stroke_dash_array(p, value, attr->strokeDashArray);
    } else if (strcmp(name, "stroke-dashoffset") == 0) {
        attr->strokeDashOffset = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "stroke-opacity") == 0) {
        attr->strokeOpacity = svg_parse_opacity(value);
    } else if (strcmp(name, "stroke-linecap") == 0) {
        attr->strokeLineCap = svg_parse_line_cap(value);
    } else if (strcmp(name, "stroke-linejoin") == 0) {
        attr->strokeLineJoin = svg_parse_line_join(value);
    } else if (strcmp(name, "stroke-miterlimit") == 0) {
        attr->miterLimit = svg_parse_miter_limit(value);
    } else if (strcmp(name, "fill-rule") == 0) {
        attr->fillRule = svg_parse_fill_rule(value);
    } else if (strcmp(name, "font-size") == 0) {
        attr->fontSize = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "transform") == 0) {
        svg_parse_transform(xform, value);
        svg_xform_premultiply(attr->xform, xform);
    } else if (strcmp(name, "stop-color") == 0) {
        attr->stopColor = svg_parse_color(value);
    } else if (strcmp(name, "stop-opacity") == 0) {
        attr->stopOpacity = svg_parse_opacity(value);
    } else if (strcmp(name, "offset") == 0) {
        attr->stopOffset = svg_parse_coordinate(p, value, 0.0f, 1.0f);
    } else if (strcmp(name, "id") == 0) {
        strncpy(attr->id, value, 63);
        attr->id[63] = '\0';
        // printf("ASSIGN ID: %s\n",attr->id);
    } else {
        return 0;
    }
    return 1;
}
static int svg_parse_attr(svg_parse_result& p) {
    reader_t& s = *p.reader;
    char name[32];
    char value[reader_t::capture_size];
    memcpy(name, s.value(), sizeof(name));
    if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
        return 0;
    }
    memcpy(value, s.value(), sizeof(value));
    if (!s.read() || s.node_type() == ml_node_type::attribute_content) {
        // printf("attribute is too long\n");
        return 0;
    }
    if (s.node_type() == ml_node_type::attribute_end) {
        if (!s.read()) {
            return 0;
        }
    }
    return svg_parse_attr(p, name, value);
}
static float svg_convert_to_pixels(svg_parse_result& p, NSVGcoordinate c, float orig, float length) {
    NSVGattrib* attr = svg_get_attr(p);
    switch (c.units) {
        case NSVGunits::NSVG_UNITS_USER:
            return c.value;
        case NSVGunits::NSVG_UNITS_PX:
            return c.value;
        case NSVGunits::NSVG_UNITS_PT:
            return c.value / 72.0f * p.dpi;
        case NSVGunits::NSVG_UNITS_PC:
            return c.value / 6.0f * p.dpi;
        case NSVGunits::NSVG_UNITS_MM:
            return c.value / 25.4f * p.dpi;
        case NSVGunits::NSVG_UNITS_CM:
            return c.value / 2.54f * p.dpi;
        case NSVGunits::NSVG_UNITS_IN:
            return c.value * p.dpi;
        case NSVGunits::NSVG_UNITS_EM:
            return c.value * attr->fontSize;
        case NSVGunits::NSVG_UNITS_EX:
            return c.value * attr->fontSize * 0.52f;  // x-height of Helvetica.
        case NSVGunits::NSVG_UNITS_PERCENT:
            return orig + c.value / 100.0f * length;
        default:
            return c.value;
    }
    return c.value;
}
static int svg_is_coordinate(const char* s) {
    // optional sign
    if (*s == '-' || *s == '+')
        s++;
    // must have at least one digit, or start by a dot
    return (svg_isdigit(*s) || *s == '.');
}

static NSVGcoordinate svg_parse_coordinate_raw(const char* str) {
    NSVGcoordinate coord = {0, NSVGunits::NSVG_UNITS_USER};
    char buf[64];
    coord.units = svg_parse_units(svg_parse_number(str, buf, 64));
    coord.value = svg_atof(buf);
    return coord;
}

static NSVGcoordinate svg_coord(float v, NSVGunits units) {
    NSVGcoordinate coord = {v, units};
    return coord;
}

static float svg_parse_coordinate(svg_parse_result& p, const char* str, float orig, float length) {
    NSVGcoordinate coord = svg_parse_coordinate_raw(str);
    return svg_convert_to_pixels(p, coord, orig, length);
}
static float svg_actual_orig_x(svg_parse_result& p) {
    return p.viewMinx;
}
static float svg_actual_orig_y(svg_parse_result& p) {
    return p.viewMiny;
}

static float svg_actual_width(svg_parse_result& p) {
    return p.viewWidth;
}

static float svg_actual_height(svg_parse_result& p) {
    return p.viewHeight;
}

static float svg_actual_length(svg_parse_result& p) {
    float w = svg_actual_width(p), h = svg_actual_height(p);
    return sqrtf(w * w + h * h) / sqrtf(2.0f);
}
static int svg_parse_stroke_dash_array(svg_parse_result& p, const char* str, float* strokeDashArray) {
    char item[64];
    int count = 0, i;
    float sum = 0.0f;

    // Handle "none"
    if (str[0] == 'n')
        return 0;

    // Parse dashes
    while (*str) {
        str = svg_get_next_dash_item(str, item);
        if (!*item) break;
        if (count < NSVG_MAX_DASHES)
            strokeDashArray[count++] = fabsf(svg_parse_coordinate(p, item, 0.0f, svg_actual_length(p)));
    }

    for (i = 0; i < count; i++)
        sum += strokeDashArray[i];
    if (sum <= 1e-6f)
        count = 0;

    return count;
}
static void svg_parse_style(svg_parse_result& p, const char* str) {
    const char* start;
    const char* end;

    while (*str) {
        // Left Trim
        while (*str && svg_isspace(*str)) ++str;
        start = str;
        while (*str && *str != ';') ++str;
        end = str;

        // Right Trim
        while (end > start && (*end == ';' || svg_isspace(*end))) --end;
        ++end;

        svg_parse_name_value(p, start, end);
        if (*str) ++str;
    }
}
static int svg_parse_name_value(svg_parse_result& p, const char* start, const char* end) {
    const char* str;
    const char* val;
    int n;

    str = start;
    while (str < end && *str != ':') ++str;

    val = str;

    // Right Trim
    while (str > start && (*str == ':' || svg_isspace(*str))) --str;
    ++str;

    n = (int)(str - start);
    if (n > 511) n = 511;
    if (n) memcpy(p.aname, start, n);
    p.aname[n] = 0;

    while (val < end && (*val == ':' || svg_isspace(*val))) ++val;

    n = (int)(end - val);
    if (n > 511) n = 511;
    if (n) memcpy(p.avalue, val, n);
    p.avalue[n] = 0;

    return svg_parse_attr(p, p.aname, p.avalue);
}
static gfx_result svg_parse_attribs(svg_parse_result& p) {
    reader_t& s = *p.reader;
    if (ml_node_type::element != s.node_type()) {
        return gfx_result::invalid_state;
    }
    if (!s.read()) {
        return gfx_result::invalid_format;
    }
    p.style_val[0] = '\0';
    p.class_val[0] = '\0';

    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp("style", s.value()) == 0) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
            strcpy(p.style_val, s.value());
            while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
                ;
        } else if (strcmp("class", s.value()) == 0) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
            strcpy(p.class_val, s.value());
            while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
                ;
        } else {
            svg_parse_attr(p);
        }
    }
    gfx_result res;
    if (*p.class_val) {
        res = svg_apply_classes(p, p.class_val);
        if (res != gfx_result::success) {
            return res;
        }
    }
    if (*p.style_val) {
        svg_parse_style(p, p.style_val);
    }
    return gfx_result::success;
}
static NSVGgradientData* svg_find_gradient_data(svg_parse_result& p, const char* id) {
    NSVGgradientData* grad = p.gradients;
    if (id == NULL || *id == '\0')
        return NULL;
    while (grad != NULL) {
        if (strcmp(grad->id, id) == 0)
            return grad;
        grad = grad->next;
    }
    return NULL;
}
static svg_gradient* svg_create_gradient(svg_parse_result& p, const char* id, const float* localBounds, float* xform, svg_paint_type* paintType) {
    NSVGgradientData* data = NULL;
    NSVGgradientData* ref = NULL;
    svg_gradient_stop* stops = NULL;
    svg_gradient* grad;
    float ox, oy, sw, sh, sl;
    int nstops = 0;
    int refIter;

    data = svg_find_gradient_data(p, id);
    if (data == NULL) return NULL;

    // TODO: use ref to fill in all unset values too.
    ref = data;
    refIter = 0;
    while (ref != NULL) {
        NSVGgradientData* nextRef = NULL;
        if (stops == NULL && ref->stops != NULL) {
            stops = ref->stops;
            nstops = ref->nstops;
            break;
        }
        nextRef = svg_find_gradient_data(p, ref->ref);
        if (nextRef == ref) break;  // prevent infite loops on malformed data
        ref = nextRef;
        refIter++;
        if (refIter > 32) break;  // prevent infite loops on malformed data
    }
    if (stops == NULL) return NULL;
    size_t grad_sz = sizeof(svg_gradient) + sizeof(svg_gradient_stop) * (nstops - 1);
    grad = (svg_gradient*)p.allocator(grad_sz);
    if (grad == NULL) return NULL;
    p.image_size += grad_sz;
    // The shape width and height.
    if (data->units == NSVG_OBJECT_SPACE) {
        ox = localBounds[0];
        oy = localBounds[1];
        sw = localBounds[2] - localBounds[0];
        sh = localBounds[3] - localBounds[1];
    } else {
        ox = svg_actual_orig_x(p);
        oy = svg_actual_orig_y(p);
        sw = svg_actual_width(p);
        sh = svg_actual_height(p);
    }
    sl = sqrtf(sw * sw + sh * sh) / sqrtf(2.0f);

    if (data->type == svg_paint_type::linear_gradient) {
        float x1, y1, x2, y2, dx, dy;
        x1 = svg_convert_to_pixels(p, data->linear.x1, ox, sw);
        y1 = svg_convert_to_pixels(p, data->linear.y1, oy, sh);
        x2 = svg_convert_to_pixels(p, data->linear.x2, ox, sw);
        y2 = svg_convert_to_pixels(p, data->linear.y2, oy, sh);
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
        cx = svg_convert_to_pixels(p, data->radial.cx, ox, sw);
        cy = svg_convert_to_pixels(p, data->radial.cy, oy, sh);
        fx = svg_convert_to_pixels(p, data->radial.fx, ox, sw);
        fy = svg_convert_to_pixels(p, data->radial.fy, oy, sh);
        r = svg_convert_to_pixels(p, data->radial.r, 0, sl);
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

    svg_xform_multiply(grad->xform.data, data->xform);
    svg_xform_multiply(grad->xform.data, xform);

    grad->spread = data->spread;
    memcpy(grad->stops, stops, nstops * sizeof(svg_gradient_stop));
    grad->stop_count = nstops;

    *paintType = data->type;

    return grad;
}
static void svg_image_bounds(svg_image* p, float* bounds) {
    svg_shape* shape;
    shape = p->shapes;
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

static float svg_view_align(float content, float container, int type) {
    if (type == NSVG_ALIGN_MIN)
        return 0;
    else if (type == NSVG_ALIGN_MAX)
        return container - content;
    // mid
    return (container - content) * 0.5f;
}

static void svg_scale_gradient(svg_gradient* grad, float tx, float ty, float sx, float sy) {
    float t[6];
    svg_xform_set_translation(t, tx, ty);
    svg_xform_multiply(grad->xform.data, t);

    svg_xform_set_scale(t, sx, sy);
    svg_xform_multiply(grad->xform.data, t);
}

static void svg_scale_to_viewbox(svg_parse_result& p, const char* units) {
    svg_shape* shape;
    svg_path* path;
    float tx, ty, sx, sy, us, bounds[4], t[6], avgs;
    int i;
    float* pt;

    // Guess image size if not set completely.
    svg_image_bounds(p.image, bounds);

    if (p.viewWidth == 0) {
        if (p.image->dimensions.width > 0) {
            p.viewWidth = p.image->dimensions.width;
        } else {
            p.viewMinx = bounds[0];
            p.viewWidth = bounds[2] - bounds[0];
        }
    }
    if (p.viewHeight == 0) {
        if (p.image->dimensions.height > 0) {
            p.viewHeight = p.image->dimensions.height;
        } else {
            p.viewMiny = bounds[1];
            p.viewHeight = bounds[3] - bounds[1];
        }
    }
    if (p.image->dimensions.width == 0)
        p.image->dimensions.width = p.viewWidth;
    if (p.image->dimensions.height == 0)
        p.image->dimensions.height = p.viewHeight;

    tx = -p.viewMinx;
    ty = -p.viewMiny;
    sx = p.viewWidth > 0 ? p.image->dimensions.width / p.viewWidth : 0;
    sy = p.viewHeight > 0 ? p.image->dimensions.height / p.viewHeight : 0;
    // Unit scaling
    us = 1.0f / svg_convert_to_pixels(p, svg_coord(1.0f, svg_parse_units(units)), 0.0f, 1.0f);

    // Fix aspect ratio
    if (p.alignType == NSVG_ALIGN_MEET) {
        // fit whole image into viewbox
        sx = sy = svg_minf(sx, sy);
        tx += svg_view_align(p.viewWidth * sx, p.image->dimensions.width, p.alignX) / sx;
        ty += svg_view_align(p.viewHeight * sy, p.image->dimensions.height, p.alignY) / sy;
    } else if (p.alignType == NSVG_ALIGN_SLICE) {
        // fill whole viewbox with image
        sx = sy = svg_maxf(sx, sy);
        tx += svg_view_align(p.viewWidth * sx, p.image->dimensions.width, p.alignX) / sx;
        ty += svg_view_align(p.viewHeight * sy, p.image->dimensions.height, p.alignY) / sy;
    }

    // Transform
    sx *= us;
    sy *= us;
    avgs = (sx + sy) / 2.0f;
    for (shape = p.image->shapes; shape != NULL; shape = shape->next) {
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
            svg_scale_gradient(shape->fill.gradient, tx, ty, sx, sy);
            memcpy(t, shape->fill.gradient->xform.data, sizeof(float) * 6);
            svg_xform_inverse(shape->fill.gradient->xform.data, t);
        }
        if (shape->stroke.type == svg_paint_type::linear_gradient || shape->stroke.type == svg_paint_type::radial_gradient) {
            svg_scale_gradient(shape->stroke.gradient, tx, ty, sx, sy);
            memcpy(t, shape->stroke.gradient->xform.data, sizeof(float) * 6);
            svg_xform_inverse(shape->stroke.gradient->xform.data, t);
        }

        shape->stroke_width *= avgs;
        shape->stroke_dash_offset *= avgs;
        for (i = 0; i < shape->stroke_dash_count; i++)
            shape->stroke_dash_array[i] *= avgs;
    }
}
static gfx_result svg_parse_gradient_elem(svg_parse_result& ctx, svg_paint_type type) {
    reader_t& s = *ctx.reader;

    NSVGgradientData* grad = (NSVGgradientData*)ctx.allocator(sizeof(NSVGgradientData));
    if (grad == NULL) return gfx_result::out_of_memory;
    ctx.image_size += sizeof(NSVGgradientData);
    memset(grad, 0, sizeof(NSVGgradientData));
    grad->units = NSVG_OBJECT_SPACE;
    grad->type = type;
    if (grad->type == svg_paint_type::linear_gradient) {
        grad->linear.x1 = svg_coord(0.0f, NSVGunits::NSVG_UNITS_PERCENT);
        grad->linear.y1 = svg_coord(0.0f, NSVGunits::NSVG_UNITS_PERCENT);
        grad->linear.x2 = svg_coord(100.0f, NSVGunits::NSVG_UNITS_PERCENT);
        grad->linear.y2 = svg_coord(0.0f, NSVGunits::NSVG_UNITS_PERCENT);
    } else if (grad->type == svg_paint_type::radial_gradient) {
        grad->radial.cx = svg_coord(50.0f, NSVGunits::NSVG_UNITS_PERCENT);
        grad->radial.cy = svg_coord(50.0f, NSVGunits::NSVG_UNITS_PERCENT);
        grad->radial.r = svg_coord(50.0f, NSVGunits::NSVG_UNITS_PERCENT);
    }

    svg_xform_identity(grad->xform);
    if (!s.read()) {
        return gfx_result::invalid_format;
    }
    while (s.node_type() == ml_node_type::attribute) {
        memcpy(ctx.lname, s.value(), sizeof(ctx.lname));
        if (strcmp(ctx.lname, "id") == 0) {
            char* sz = grad->id;
            size_t ssz = sizeof(grad->id);
            bool sf = false;
            while (ssz > 0 && s.read() && s.node_type() != ml_node_type::attribute_end) {
                sf = true;
                strncpy(sz, s.value(), ssz);
                sz += ssz;
                ssz -= strlen(s.value());
            }
            if (!sf) {
                return gfx_result::invalid_format;
            }
            if (s.node_type() != ml_node_type::attribute_end || !s.read()) {
                return gfx_result::invalid_format;
            }

        } else {
            char name[32];
            char value[reader_t::capture_size];
            memcpy(name, s.value(), sizeof(name));
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            memcpy(value, s.value(), sizeof(value));
            if (!s.read() || s.node_type() != ml_node_type::attribute_end) {
                return gfx_result::not_supported;
            }
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
            if (!svg_parse_attr(ctx, name, value)) {
                if (strcmp(name, "gradientUnits") == 0) {
                    if (strcmp(value, "objectBoundingBox") == 0)
                        grad->units = NSVG_OBJECT_SPACE;
                    else
                        grad->units = NSVG_USER_SPACE;
                } else if (strcmp(name, "gradientTransform") == 0) {
                    svg_parse_transform(grad->xform, value);
                } else if (strcmp(name, "cx") == 0) {
                    grad->radial.cx = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "cy") == 0) {
                    grad->radial.cy = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "r") == 0) {
                    grad->radial.r = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "fx") == 0) {
                    grad->radial.fx = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "fy") == 0) {
                    grad->radial.fy = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "x1") == 0) {
                    grad->linear.x1 = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "y1") == 0) {
                    grad->linear.y1 = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "x2") == 0) {
                    grad->linear.x2 = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "y2") == 0) {
                    grad->linear.y2 = svg_parse_coordinate_raw(value);
                } else if (strcmp(name, "spreadMethod") == 0) {
                    if (strcmp(value, "pad") == 0)
                        grad->spread = svg_spread_type::pad;
                    else if (strcmp(value, "reflect") == 0)
                        grad->spread = svg_spread_type::reflect;
                    else if (strcmp(value, "repeat") == 0)
                        grad->spread = svg_spread_type::repeat;
                } else if (strcmp(name, "xlink:href") == 0) {
                    const char* href = value;
                    strncpy(grad->ref, href + 1, 62);
                    grad->ref[62] = '\0';
                }
            }
        }
    }

    grad->next = ctx.gradients;
    ctx.gradients = grad;
    return gfx_result::invalid_format;
}
static gfx_result svg_parse_gradient_stop_elem(svg_parse_result& p) {
    reader_t& s = *p.reader;
    NSVGattrib* curAttr = svg_get_attr(p);
    NSVGgradientData* grad;
    svg_gradient_stop* stop;
    int i, idx;

    curAttr->stopOffset = 0;
    curAttr->stopColor.native_value = 0;
    curAttr->stopOpacity = 1.0f;

    if (!s.read()) {
        return gfx_result::invalid_format;
    }
    while (s.node_type() == ml_node_type::attribute) {
        svg_parse_attr(p);
    }

    // Add stop to the last gradient.
    grad = p.gradients;
    if (grad == NULL) return gfx_result::out_of_memory;

    grad->nstops++;
    grad->stops = (svg_gradient_stop*)p.reallocator(grad->stops, sizeof(svg_gradient_stop) * grad->nstops);
    if (grad->stops == NULL) return gfx_result::out_of_memory;
    p.image_size += sizeof(svg_gradient_stop);

    // Insert
    idx = grad->nstops - 1;
    for (i = 0; i < grad->nstops - 1; i++) {
        if (curAttr->stopOffset < grad->stops[i].offset) {
            idx = i;
            break;
        }
    }
    if (idx != grad->nstops - 1) {
        for (i = grad->nstops - 1; i > idx; i--)
            grad->stops[i] = grad->stops[i - 1];
    }

    stop = &grad->stops[idx];
    stop->color = curAttr->stopColor;
    float amod = stop->color.template channelr<channel_name::A>();
    amod*=curAttr->stopOpacity;
    stop->color.template channelr<channel_name::A>(amod);
    stop->offset = curAttr->stopOffset;
    return gfx_result::success;
}
static void svg_reset_path(svg_parse_result& p) {
    p.npts = 0;
}

static int svg_get_args_per_element(char cmd) {
    switch (cmd) {
        case 'v':
        case 'V':
        case 'h':
        case 'H':
            return 1;
        case 'm':
        case 'M':
        case 'l':
        case 'L':
        case 't':
        case 'T':
            return 2;
        case 'q':
        case 'Q':
        case 's':
        case 'S':
            return 4;
        case 'c':
        case 'C':
            return 6;
        case 'a':
        case 'A':
            return 7;
        case 'z':
        case 'Z':
            return 0;
    }
    return -1;
}
static gfx_result svg_add_point(svg_parse_result& p, float x, float y) {
    if (p.npts + 1 > p.cpts) {
        p.cpts = p.cpts ? p.cpts * 2 : 8;
        p.pts = (float*)p.reallocator(p.pts, p.cpts * 2 * sizeof(float));
        if (!p.pts) return gfx_result::out_of_memory;
        p.image_size += (2 * sizeof(float));
    }
    p.pts[p.npts * 2 + 0] = x;
    p.pts[p.npts * 2 + 1] = y;
    ++p.npts;
    return gfx_result::success;
}

static gfx_result svg_move_to(svg_parse_result& p, float x, float y) {
    if (p.npts > 0) {
        p.pts[(p.npts - 1) * 2 + 0] = x;
        p.pts[(p.npts - 1) * 2 + 1] = y;
        return gfx_result::success;
    } else {
        return svg_add_point(p, x, y);
    }
}

static gfx_result svg_line_to(svg_parse_result& p, float x, float y) {
    float px, py, dx, dy;
    if (p.npts > 0) {
        px = p.pts[(p.npts - 1) * 2 + 0];
        py = p.pts[(p.npts - 1) * 2 + 1];
        dx = x - px;
        dy = y - py;
        gfx_result res = svg_add_point(p, px + dx / 3.0f, py + dy / 3.0f);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_point(p, x - dx / 3.0f, y - dy / 3.0f);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_point(p, x, y);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
static gfx_result svg_add_path(svg_parse_result& p, char closed) {
    NSVGattrib* attr = svg_get_attr(p);
    svg_path* path = NULL;
    size_t pts_sz;
    float bounds[4];
    float* curve;
    int i;

    if (p.npts < 4) {
        return gfx_result::invalid_state;
    }
    if (closed) {
        gfx_result res = svg_line_to(p, p.pts[0], p.pts[1]);
        if (res != gfx_result::success) {
            return res;
        }
    }

    // Expect 1 + N*3 points (N = number of cubic bezier segments).
    if ((p.npts % 3) != 1) {
        return gfx_result::invalid_state;
    }

    path = (svg_path*)p.allocator(sizeof(svg_path));
    p.image_size += sizeof(svg_path);
    if (path == NULL) goto error;
    memset(path, 0, sizeof(svg_path));
    pts_sz = p.npts * 2 * sizeof(float);
    path->points = (float*)p.allocator(pts_sz);
    if (path->points == NULL) goto error;
    p.image_size += pts_sz;
    path->closed = closed;
    path->point_count = p.npts;

    // Transform path.
    for (i = 0; i < p.npts; ++i)
        svg_xform_point(&path->points[i * 2], &path->points[i * 2 + 1], p.pts[i * 2], p.pts[i * 2 + 1], attr->xform);

    // Find bounds
    for (i = 0; i < path->point_count - 1; i += 3) {
        curve = &path->points[i * 2];
        svg_curve_bounds(bounds, curve);
        if (i == 0) {
            path->bounds.x1 = bounds[0];
            path->bounds.y1 = bounds[1];
            path->bounds.x2 = bounds[2];
            path->bounds.y2 = bounds[3];
        } else {
            path->bounds.x1 = svg_minf(path->bounds.x1, bounds[0]);
            path->bounds.y1 = svg_minf(path->bounds.y1, bounds[1]);
            path->bounds.x2 = svg_maxf(path->bounds.x2, bounds[2]);
            path->bounds.y2 = svg_maxf(path->bounds.y2, bounds[3]);
        }
    }

    path->next = p.plist;
    p.plist = path;

    return gfx_result::success;

error:
    if (path != NULL) {
        if (path->points != NULL) p.deallocator(path->points);
        p.deallocator(path);
    }
    return gfx_result::out_of_memory;
}

static gfx_result svg_cubic_bez_to(svg_parse_result& p, float cpx1, float cpy1, float cpx2, float cpy2, float x, float y) {
    if (p.npts > 0) {
        gfx_result res = svg_add_point(p, cpx1, cpy1);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_point(p, cpx2, cpy2);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_point(p, x, y);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
static gfx_result svg_path_move_to(svg_parse_result& p, float* cpx, float* cpy, float* args, int rel) {
    if (rel) {
        *cpx += args[0];
        *cpy += args[1];
    } else {
        *cpx = args[0];
        *cpy = args[1];
    }
    return svg_move_to(p, *cpx, *cpy);
}

static gfx_result svg_path_line_to(svg_parse_result& p, float* cpx, float* cpy, float* args, int rel) {
    if (rel) {
        *cpx += args[0];
        *cpy += args[1];
    } else {
        *cpx = args[0];
        *cpy = args[1];
    }
    return svg_line_to(p, *cpx, *cpy);
}

static gfx_result svg_path_hline_to(svg_parse_result& p, float* cpx, float* cpy, float* args, int rel) {
    if (rel)
        *cpx += args[0];
    else
        *cpx = args[0];
    return svg_line_to(p, *cpx, *cpy);
}

static gfx_result svg_path_vline_to(svg_parse_result& p, float* cpx, float* cpy, float* args, int rel) {
    if (rel)
        *cpy += args[0];
    else
        *cpy = args[0];
    return svg_line_to(p, *cpx, *cpy);
}

static gfx_result svg_path_cubic_bez_to(svg_parse_result& p, float* cpx, float* cpy,
                                        float* cpx2, float* cpy2, float* args, int rel) {
    float x2, y2, cx1, cy1, cx2, cy2;

    if (rel) {
        cx1 = *cpx + args[0];
        cy1 = *cpy + args[1];
        cx2 = *cpx + args[2];
        cy2 = *cpy + args[3];
        x2 = *cpx + args[4];
        y2 = *cpy + args[5];
    } else {
        cx1 = args[0];
        cy1 = args[1];
        cx2 = args[2];
        cy2 = args[3];
        x2 = args[4];
        y2 = args[5];
    }

    gfx_result res = svg_cubic_bez_to(p, cx1, cy1, cx2, cy2, x2, y2);
    if (res != gfx_result::success) {
        return res;
    }
    *cpx2 = cx2;
    *cpy2 = cy2;
    *cpx = x2;
    *cpy = y2;
    return gfx_result::success;
}

static gfx_result svg_path_cubic_bez_short_to(svg_parse_result& p, float* cpx, float* cpy,
                                              float* cpx2, float* cpy2, float* args, int rel) {
    float x1, y1, x2, y2, cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        cx2 = *cpx + args[0];
        cy2 = *cpy + args[1];
        x2 = *cpx + args[2];
        y2 = *cpy + args[3];
    } else {
        cx2 = args[0];
        cy2 = args[1];
        x2 = args[2];
        y2 = args[3];
    }

    cx1 = 2 * x1 - *cpx2;
    cy1 = 2 * y1 - *cpy2;

    gfx_result res = svg_cubic_bez_to(p, cx1, cy1, cx2, cy2, x2, y2);
    if (res != gfx_result::success) {
        return res;
    }
    *cpx2 = cx2;
    *cpy2 = cy2;
    *cpx = x2;
    *cpy = y2;
    return gfx_result::success;
}

static gfx_result svg_path_quad_bez_to(svg_parse_result& p, float* cpx, float* cpy,
                                       float* cpx2, float* cpy2, float* args, int rel) {
    float x1, y1, x2, y2, cx, cy;
    float cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        cx = *cpx + args[0];
        cy = *cpy + args[1];
        x2 = *cpx + args[2];
        y2 = *cpy + args[3];
    } else {
        cx = args[0];
        cy = args[1];
        x2 = args[2];
        y2 = args[3];
    }

    // Convert to cubic bezier
    cx1 = x1 + 2.0f / 3.0f * (cx - x1);
    cy1 = y1 + 2.0f / 3.0f * (cy - y1);
    cx2 = x2 + 2.0f / 3.0f * (cx - x2);
    cy2 = y2 + 2.0f / 3.0f * (cy - y2);

    gfx_result res = svg_cubic_bez_to(p, cx1, cy1, cx2, cy2, x2, y2);
    if (res != gfx_result::success) {
        return res;
    }

    *cpx2 = cx;
    *cpy2 = cy;
    *cpx = x2;
    *cpy = y2;
    return gfx_result::success;
}

static gfx_result svg_path_quad_bez_short_to(svg_parse_result& p, float* cpx, float* cpy,
                                             float* cpx2, float* cpy2, float* args, int rel) {
    float x1, y1, x2, y2, cx, cy;
    float cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        x2 = *cpx + args[0];
        y2 = *cpy + args[1];
    } else {
        x2 = args[0];
        y2 = args[1];
    }

    cx = 2 * x1 - *cpx2;
    cy = 2 * y1 - *cpy2;

    // Convert to cubix bezier
    cx1 = x1 + 2.0f / 3.0f * (cx - x1);
    cy1 = y1 + 2.0f / 3.0f * (cy - y1);
    cx2 = x2 + 2.0f / 3.0f * (cx - x2);
    cy2 = y2 + 2.0f / 3.0f * (cy - y2);

    gfx_result res = svg_cubic_bez_to(p, cx1, cy1, cx2, cy2, x2, y2);
    if (res != gfx_result::success) {
        return res;
    }

    *cpx2 = cx;
    *cpy2 = cy;
    *cpx = x2;
    *cpy = y2;
    return gfx_result::success;
}

static gfx_result svg_path_arc_to(svg_parse_result& p, float* cpx, float* cpy, float* args, int rel) {
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

    rx = fabsf(args[0]);                 // y radius
    ry = fabsf(args[1]);                 // x radius
    rotx = args[2] / 180.0f * NSVG_PI;   // x rotation angle
    fa = fabsf(args[3]) > 1e-6 ? 1 : 0;  // Large arc
    fs = fabsf(args[4]) > 1e-6 ? 1 : 0;  // Sweep direction
    x1 = *cpx;                           // start point
    y1 = *cpy;
    if (rel) {  // end point
        x2 = *cpx + args[5];
        y2 = *cpy + args[6];
    } else {
        x2 = args[5];
        y2 = args[6];
    }

    dx = x1 - x2;
    dy = y1 - y2;
    d = sqrtf(dx * dx + dy * dy);
    if (d < 1e-6f || rx < 1e-6f || ry < 1e-6f) {
        // The arc degenerates to a line
        gfx_result res = svg_line_to(p, x2, y2);
        if (res != gfx_result::success) {
            return res;
        }
        *cpx = x2;
        *cpy = y2;
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
        da -= 2 * NSVG_PI;
    else if (fs == 1 && da < 0)
        da += 2 * NSVG_PI;

    // Approximate the arc using cubic spline segments.
    t[0] = cosrx;
    t[1] = sinrx;
    t[2] = -sinrx;
    t[3] = cosrx;
    t[4] = cx;
    t[5] = cy;

    // Split arc into max 90 degree segments.
    // The loop assumes an iteration per end point (including start and end), this +1.
    ndivs = (int)(fabsf(da) / (NSVG_PI * 0.5f) + 1.0f);
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
            gfx_result res = svg_cubic_bez_to(p, px + ptanx, py + ptany, x - tanx, y - tany, x, y);
            if (res != gfx_result::success) {
                return res;
            }
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    *cpx = x2;
    *cpy = y2;
    return gfx_result::success;
}
static float svg_get_average_scale(float* t) {
    float sx = sqrtf(t[0] * t[0] + t[2] * t[2]);
    float sy = sqrtf(t[1] * t[1] + t[3] * t[3]);
    return (sx + sy) * 0.5f;
}

static void svg_get_local_bounds(float* bounds, svg_shape* shape, float* xform) {
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
static void svg_create_gradients(svg_parse_result& p) {
    svg_shape* shape;

    for (shape = p.image->shapes; shape != NULL; shape = shape->next) {
        if (shape->fill.type == svg_paint_type::undefined) {
            if (shape->fill_gradient_id[0] != '\0') {
                float inv[6], localBounds[4];
                svg_xform_inverse(inv, shape->xform.data);
                svg_get_local_bounds(localBounds, shape, inv);
                shape->fill.gradient = svg_create_gradient(p, shape->fill_gradient_id, localBounds, shape->xform.data, &shape->fill.type);
            }
            if (shape->fill.type == svg_paint_type::undefined) {
                shape->fill.type = svg_paint_type::none;
            }
        }
        if (shape->stroke.type == svg_paint_type::undefined) {
            if (shape->stroke_gradient_id[0] != '\0') {
                float inv[6], localBounds[4];
                svg_xform_inverse(inv, shape->xform.data);
                svg_get_local_bounds(localBounds, shape, inv);
                shape->stroke.gradient = svg_create_gradient(p, shape->stroke_gradient_id, localBounds, shape->xform.data, &shape->stroke.type);
            }
            if (shape->stroke.type == svg_paint_type::undefined) {
                shape->stroke.type = svg_paint_type::none;
            }
        }
    }
}

static gfx_result svg_add_shape(svg_parse_result& p) {
    NSVGattrib* attr = svg_get_attr(p);
    float scale = 1.0f;
    svg_shape* shape;
    svg_path* path;
    int i;

    if (p.plist == NULL)
        return gfx_result::invalid_state;
    shape = (svg_shape*)p.allocator(sizeof(svg_shape));
    if (shape == NULL) goto error;
    p.image_size += sizeof(svg_shape);
    memset(shape, 0, sizeof(svg_shape));

    memcpy(shape->id, attr->id, sizeof shape->id);
    // printf("ADD SHAPE ID %s. attrHead is %d\n",shape->id,p.attrHead);
    memcpy(shape->fill_gradient_id, attr->fillGradient, sizeof shape->fill_gradient_id);
    memcpy(shape->stroke_gradient_id, attr->strokeGradient, sizeof shape->stroke_gradient_id);
    memcpy(shape->xform.data, attr->xform, sizeof shape->xform);
    scale = svg_get_average_scale(attr->xform);
    shape->stroke_width = attr->strokeWidth * scale;
    shape->stroke_dash_offset = attr->strokeDashOffset * scale;
    shape->stroke_dash_count = (char)attr->strokeDashCount;
    for (i = 0; i < attr->strokeDashCount; i++)
        shape->stroke_dash_array[i] = attr->strokeDashArray[i] * scale;
    shape->stroke_line_join = attr->strokeLineJoin;
    shape->stroke_line_cap = attr->strokeLineCap;
    shape->miter_limit = attr->miterLimit;
    shape->fill_rule = attr->fillRule;
    shape->opacity = attr->opacity;

    shape->paths = p.plist;
    p.plist = NULL;

    // Calculate shape bounds
    shape->bounds = shape->paths->bounds;
    for (path = shape->paths->next; path != NULL; path = path->next) {
        shape->bounds.x1 = svg_minf(shape->bounds.x1, path->bounds.x1);
        shape->bounds.y1 = svg_minf(shape->bounds.y1, path->bounds.y1);
        shape->bounds.x2 = svg_maxf(shape->bounds.x2, path->bounds.x2);
        shape->bounds.y2 = svg_maxf(shape->bounds.y2, path->bounds.y2);
    }

    // Set fill
    if (attr->hasFill == 0) {
        shape->fill.type = svg_paint_type::none;
    } else if (attr->hasFill == 1) {
        shape->fill.type = svg_paint_type::color;
        shape->fill.color = attr->fillColor;
        float amod = shape->fill.color.template channelr<channel_name::A>();
        amod*=attr->fillOpacity;
        shape->fill.color.template channelr<channel_name::A>(amod);
    } else if (attr->hasFill == 2) {
        shape->fill.type = svg_paint_type::undefined;
    }

    // Set stroke
    if (attr->hasStroke == 0) {
        shape->stroke.type = svg_paint_type::none;
    } else if (attr->hasStroke == 1) {
        shape->stroke.type = svg_paint_type::color;
        shape->stroke.color = attr->strokeColor;
        float amod = shape->stroke.color.template channelr<channel_name::A>();
        amod *= attr->strokeOpacity;
        shape->stroke.color.template channelr<channel_name::A>(amod);
    } else if (attr->hasStroke == 2) {
        shape->stroke.type = svg_paint_type::undefined;
    }

    // Set flags
    shape->flags = (attr->visible ? SVG_FLAGS_VISIBLE : 0x00);

    // Add to tail
    if (p.image->shapes == NULL)
        p.image->shapes = shape;
    else
        p.shapesTail->next = shape;
    p.shapesTail = shape;

    return gfx_result::success;

error:
    if (shape) p.deallocator(shape);
    // printf("out of memory allocating shape\n");
    return gfx_result::out_of_memory;
}
static gfx_result svg_parse_rect_elem(svg_parse_result& p) {
    // printf("parse <rect>\n");
    reader_t& s = *p.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float rx = -1.0f;  // marks not set
    float ry = -1.0f;
    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp(s.value(), "x") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            x = svg_parse_coordinate(p, s.value(), svg_actual_orig_x(p), svg_actual_width(p));
        } else if (strcmp(s.value(), "y") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            y = svg_parse_coordinate(p, s.value(), svg_actual_orig_y(p), svg_actual_height(p));
        } else if (strcmp(s.value(), "width") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            w = svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_width(p));
        } else if (strcmp(s.value(), "height") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            h = svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_height(p));
        } else if (strcmp(s.value(), "rx") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            rx = fabsf(svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_width(p)));
        } else if (strcmp(s.value(), "ry") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            ry = fabsf(svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_height(p)));
        } else {
            svg_parse_attr(p);
        }
        if (s.node_type() == ml_node_type::attribute_content && s.read()) {
            if (s.node_type() == ml_node_type::attribute_content) {
                return gfx_result::out_of_memory;
            }
        }
        if (s.node_type() == ml_node_type::attribute_end) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }

    if (rx < 0.0f && ry > 0.0f) rx = ry;
    if (ry < 0.0f && rx > 0.0f) ry = rx;
    if (rx < 0.0f) rx = 0.0f;
    if (ry < 0.0f) ry = 0.0f;
    if (rx > w / 2.0f) rx = w / 2.0f;
    if (ry > h / 2.0f) ry = h / 2.0f;

    if (w != 0.0f && h != 0.0f) {
        svg_reset_path(p);
        gfx_result res;
        if (rx < 0.00001f || ry < 0.0001f) {
            res = svg_move_to(p, x, y);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x + w, y);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x + w, y + h);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x, y + h);
            if (res != gfx_result::success) {
                return res;
            }
        } else {
            // Rounded rectangle
            res = svg_move_to(p, x + rx, y);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x + w - rx, y);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_cubic_bez_to(p, x + w - rx * (1 - NSVG_KAPPA90), y, x + w, y + ry * (1 - NSVG_KAPPA90), x + w, y + ry);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x + w, y + h - ry);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_cubic_bez_to(p, x + w, y + h - ry * (1 - NSVG_KAPPA90), x + w - rx * (1 - NSVG_KAPPA90), y + h, x + w - rx, y + h);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x + rx, y + h);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_cubic_bez_to(p, x + rx * (1 - NSVG_KAPPA90), y + h, x, y + h - ry * (1 - NSVG_KAPPA90), x, y + h - ry);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_line_to(p, x, y + ry);
            if (res != gfx_result::success) {
                return res;
            }
            res = svg_cubic_bez_to(p, x, y + ry * (1 - NSVG_KAPPA90), x + rx * (1 - NSVG_KAPPA90), y, x + rx, y);
            if (res != gfx_result::success) {
                return res;
            }
        }

        res = svg_add_path(p, 1);
        if (res != gfx_result::success) {
            return res;
        }

        res = svg_add_shape(p);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}

static gfx_result svg_parse_circle_elem(svg_parse_result& p) {
    // printf("parse <circle>\n");
    reader_t& s = *p.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    float cx = 0.0f;
    float cy = 0.0f;
    float r = 0.0f;
    gfx_result res;
    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp(s.value(), "cx") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            cx = svg_parse_coordinate(p, s.value(), svg_actual_orig_x(p), svg_actual_width(p));
        } else if (strcmp(s.value(), "cy") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            cy = svg_parse_coordinate(p, s.value(), svg_actual_orig_y(p), svg_actual_height(p));
        } else if (strcmp(s.value(), "r") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            r = fabsf(svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_length(p)));
        } else {
            svg_parse_attr(p);
        }
        if (s.node_type() == ml_node_type::attribute_content && s.read()) {
            if (s.node_type() == ml_node_type::attribute_content) {
                return gfx_result::out_of_memory;
            }
        }
        if (s.node_type() == ml_node_type::attribute_end) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }
    if (r > 0.0f) {
        svg_reset_path(p);

        res = svg_move_to(p, cx + r, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx + r, cy + r * NSVG_KAPPA90, cx + r * NSVG_KAPPA90, cy + r, cx, cy + r);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx - r * NSVG_KAPPA90, cy + r, cx - r, cy + r * NSVG_KAPPA90, cx - r, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx - r, cy - r * NSVG_KAPPA90, cx - r * NSVG_KAPPA90, cy - r, cx, cy - r);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx + r * NSVG_KAPPA90, cy - r, cx + r, cy - r * NSVG_KAPPA90, cx + r, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_path(p, 1);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_shape(p);
        if (res != gfx_result::success) {
            return res;
        }
    }

    return gfx_result::success;
}

static gfx_result svg_parse_ellipse_elem(svg_parse_result& p) {
    // printf("parse <ellipse>\n");
    reader_t& s = *p.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    float cx = 0.0f;
    float cy = 0.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    gfx_result res;

    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp(s.value(), "cx") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            cx = svg_parse_coordinate(p, s.value(), svg_actual_orig_x(p), svg_actual_width(p));
        } else if (strcmp(s.value(), "cy") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            cy = svg_parse_coordinate(p, s.value(), svg_actual_orig_y(p), svg_actual_height(p));
        } else if (strcmp(s.value(), "rx") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            rx = fabsf(svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_width(p)));
        } else if (strcmp(s.value(), "ry") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            rx = fabsf(svg_parse_coordinate(p, s.value(), 0.0f, svg_actual_height(p)));
        } else {
            svg_parse_attr(p);
        }
        if (s.node_type() == ml_node_type::attribute_content && s.read()) {
            if (s.node_type() == ml_node_type::attribute_content) {
                return gfx_result::out_of_memory;
            }
        }
        if (s.node_type() == ml_node_type::attribute_end) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }

    if (rx > 0.0f && ry > 0.0f) {
        svg_reset_path(p);

        res = svg_move_to(p, cx + rx, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx + rx, cy + ry * NSVG_KAPPA90, cx + rx * NSVG_KAPPA90, cy + ry, cx, cy + ry);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx - rx * NSVG_KAPPA90, cy + ry, cx - rx, cy + ry * NSVG_KAPPA90, cx - rx, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx - rx, cy - ry * NSVG_KAPPA90, cx - rx * NSVG_KAPPA90, cy - ry, cx, cy - ry);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_cubic_bez_to(p, cx + rx * NSVG_KAPPA90, cy - ry, cx + rx, cy - ry * NSVG_KAPPA90, cx + rx, cy);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_path(p, 1);
        if (res != gfx_result::success) {
            return res;
        }
        res = svg_add_shape(p);
        if (res != gfx_result::success) {
            return res;
        }
    }

    return gfx_result::success;
}

static gfx_result svg_parse_line_elem(svg_parse_result& p) {
    // printf("parse <line>\n");
    reader_t& s = *p.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;
    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp(s.value(), "x1") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            x1 = svg_parse_coordinate(p, s.value(), svg_actual_orig_x(p), svg_actual_width(p));
        } else if (strcmp(s.value(), "y1") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            y1 = svg_parse_coordinate(p, s.value(), svg_actual_orig_y(p), svg_actual_height(p));
        } else if (strcmp(s.value(), "x2") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            x2 = fabsf(svg_parse_coordinate(p, s.value(), svg_actual_orig_x(p), svg_actual_width(p)));
        } else if (strcmp(s.value(), "y2") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            y2 = fabsf(svg_parse_coordinate(p, s.value(), svg_actual_orig_y(p), svg_actual_height(p)));
        } else {
            svg_parse_attr(p);
        }
        if (s.node_type() == ml_node_type::attribute_content && s.read()) {
            if (s.node_type() == ml_node_type::attribute_content) {
                return gfx_result::out_of_memory;
            }
        }
        if (s.node_type() == ml_node_type::attribute_end) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }
    svg_reset_path(p);

    gfx_result res = svg_move_to(p, x1, y1);
    if (res != gfx_result::success) {
        return res;
    }
    res = svg_line_to(p, x2, y2);
    if (res != gfx_result::success) {
        return res;
    }
    res = svg_add_path(p, 0);
    if (res != gfx_result::success) {
        return res;
    }
    res = svg_add_shape(p);
    if (res != gfx_result::success) {
        return res;
    }
    return gfx_result::success;
}

static gfx_result svg_parse_poly_elem(svg_parse_result& p, int closeFlag) {
    reader_t& s = *p.reader;
    // printf("parse <%s>\n", s.value());
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }

    const char* ss;
    float args[2];
    int nargs, npts = 0;
    char item[64];
    gfx_result res;
    svg_reset_path(p);
    while (s.node_type() == ml_node_type::attribute) {
        if (strcmp(s.value(), "points") == 0 && s.read() && s.node_type() == ml_node_type::attribute_content) {
            ss = s.value();
            nargs = 0;
            while (*ss) {
                ss = svg_get_next_path_item(ss, item);
                args[nargs++] = (float)svg_atof(item);
                if (nargs >= 2) {
                    if (npts == 0) {
                        res = svg_move_to(p, args[0], args[1]);
                        if (res != gfx_result::success) {
                            return res;
                        }
                    } else {
                        res = svg_line_to(p, args[0], args[1]);
                        if (res != gfx_result::success) {
                            return res;
                        }
                    }
                    nargs = 0;
                    ++npts;
                }
            }
        } else {
            svg_parse_attr(p);
        }
        if (s.node_type() == ml_node_type::attribute_content && s.read()) {
            if (s.node_type() == ml_node_type::attribute_content) {
                return gfx_result::out_of_memory;
            }
        }
        if (s.node_type() == ml_node_type::attribute_end) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }

    res = svg_add_path(p, (char)closeFlag);
    if (res != gfx_result::success) {
        return res;
    }
    res = svg_add_shape(p);
    if (res != gfx_result::success) {
        return res;
    }
    return gfx_result::success;
}

static gfx_result svg_parse_path(svg_parse_result& p, const char* d) {
    gfx_result res;
    const char* str = d;
    char cmd = '\0';
    float args[10];
    int nargs;
    int rargs = 0;
    char initPoint;
    float cpx, cpy, cpx2, cpy2;
    char closedFlag;
    char item[64];

    svg_reset_path(p);
    cpx = 0;
    cpy = 0;
    cpx2 = 0;
    cpy2 = 0;
    initPoint = 0;
    closedFlag = 0;
    nargs = 0;

    while (*str) {
        item[0] = '\0';
        if ((cmd == 'A' || cmd == 'a') && (nargs == 3 || nargs == 4))
            str = svg_get_next_path_item_when_arc_flag(str, item);
        if (!*item) {
            str = svg_get_next_path_item(str, item);
        }
        if (!*item) break;

        if (cmd != '\0' && svg_is_coordinate(item)) {
            if (nargs < 10) {
                args[nargs] = (float)svg_atof(item);
                ++nargs;
            }
            if (nargs >= rargs) {
                switch (cmd) {
                    case 'm':
                    case 'M':
                        res = svg_path_move_to(p, &cpx, &cpy, args, cmd == 'm' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("move to error\n");
                            return res;
                        }
                        // Moveto can be followed by multiple coordinate pairs,
                        // which should be treated as linetos.
                        cmd = (cmd == 'm') ? 'l' : 'L';
                        rargs = svg_get_args_per_element(cmd);
                        cpx2 = cpx;
                        cpy2 = cpy;
                        initPoint = 1;
                        break;
                    case 'l':
                    case 'L':
                        res = svg_path_line_to(p, &cpx, &cpy, args, cmd == 'l' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("line to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'H':
                    case 'h':
                        res = svg_path_hline_to(p, &cpx, &cpy, args, cmd == 'h' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("hline to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'V':
                    case 'v':
                        res = svg_path_vline_to(p, &cpx, &cpy, args, cmd == 'v' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("vline to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'C':
                    case 'c':
                        res = svg_path_cubic_bez_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'c' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("path cubic bez to error\n");
                            return res;
                        }
                        break;
                    case 'S':
                    case 's':
                        res = svg_path_cubic_bez_short_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 's' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("path cubic bez to short error\n");
                            return res;
                        }
                        break;
                    case 'Q':
                    case 'q':
                        res = svg_path_quad_bez_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'q' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("path quad bez to error\n");
                            return res;
                        }
                        break;
                    case 'T':
                    case 't':
                        res = svg_path_quad_bez_short_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 't' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("path quad bez to short error\n");
                            return res;
                        }
                        break;
                    case 'A':
                    case 'a':
                        res = svg_path_arc_to(p, &cpx, &cpy, args, cmd == 'a' ? 1 : 0);
                        if (gfx_result::success != res) {
                            // printf("path arc to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    default:
                        if (nargs >= 2) {
                            cpx = args[nargs - 2];
                            cpy = args[nargs - 1];
                            cpx2 = cpx;
                            cpy2 = cpy;
                        }
                        break;
                }
                nargs = 0;
            }
        } else {
            cmd = item[0];
            if (cmd == 'M' || cmd == 'm') {
                // Commit path.
                if (p.npts > 3) {
                    res = svg_add_path(p, closedFlag);
                    if (gfx_result::success != res) {
                        // printf("add path error\n");
                        return res;
                    }
                }
                // Start new subpath.
                svg_reset_path(p);
                closedFlag = 0;
                nargs = 0;
            } else if (initPoint == 0) {
                // Do not allow other commands until initial point has been set (moveTo called once).
                cmd = '\0';
            }
            if (cmd == 'Z' || cmd == 'z') {
                closedFlag = 1;
                // Commit path.
                if (p.npts > 0) {
                    // Move current point to first point
                    cpx = p.pts[0];
                    cpy = p.pts[1];
                    cpx2 = cpx;
                    cpy2 = cpy;
                    if (p.npts > 3) {
                        res = svg_add_path(p, closedFlag);
                        if (gfx_result::success != res) {
                            // printf("add path error\n");
                            return res;
                        }
                    }
                }
                // Start new subpath.
                svg_reset_path(p);
                res = svg_move_to(p, cpx, cpy);
                if (gfx_result::success != res) {
                    // printf("move to error\n");
                    return res;
                }
                closedFlag = 0;
                nargs = 0;
            }
            rargs = svg_get_args_per_element(cmd);
            if (rargs == -1) {
                // Command not recognized
                cmd = '\0';
                rargs = 0;
            }
        }
    }
    // Commit path.
    if (p.npts > 3) {
        res = svg_add_path(p, closedFlag);
        if (gfx_result::success != res) {
            // printf("add path error\n");
            return res;
        }
    }
    if (p.plist != nullptr) {
        // printf("path add_shape\n");
        res = svg_add_shape(p);
        if (gfx_result::success != res) {
            // printf("add shape error\n");
            return res;
        }
        return res;
    }
    return gfx_result::success;
}
static gfx_result svg_parse_path_elem(svg_parse_result& ctx) {
    // printf("parse <path>\n");
    gfx_result res;
    reader_t& s = *ctx.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    ctx.style_val[0] = '\0';
    ctx.class_val[0] = '\0';

    bool should_parse_path = false;
    while (s.node_type() == ml_node_type::attribute) {
        if (0 == strcmp("d", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            should_parse_path = true;
            size_t cdsz = strlen(s.value()) + 1;
            if (ctx.d == nullptr) {
                ctx.d = (char*)ctx.allocator(cdsz);
                if (ctx.d == nullptr) {
                    return gfx_result::out_of_memory;
                }
                ctx.d_size = cdsz;
            } else if (ctx.d_size < cdsz) {
                char* tmp = (char*)ctx.reallocator(ctx.d, cdsz);
                if (tmp == nullptr) {
                    return gfx_result::out_of_memory;
                }
                ctx.d = tmp;
                ctx.d_size = cdsz;
            }
            strcpy(ctx.d, s.value());
            while (s.read() && s.node_type() == ml_node_type::attribute_content) {
                cdsz += strlen(s.value());
                if (ctx.d_size < cdsz) {
                    char* tmp = (char*)ctx.reallocator(ctx.d, cdsz);
                    if (tmp == nullptr) {
                        return gfx_result::out_of_memory;
                    }
                    ctx.d = tmp;
                    ctx.d_size = cdsz;
                }
                strcat(ctx.d, s.value());
            }
            while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
                ;
        } else if (0 == strcmp("style", s.value())) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
            memcpy(ctx.style_val, s.value(), sizeof(ctx.style_val));
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        } else if (0 == strcmp("class", s.value())) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
            memcpy(ctx.class_val, s.value(), sizeof(ctx.class_val));
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        } else {
            svg_parse_attr(ctx);
        }
        while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
            ;
    }
    if (*ctx.class_val) {
        res = svg_apply_classes(ctx, ctx.class_val);
        if (res != gfx_result::success) {
            return res;
        }
    }
    if (*ctx.style_val) {
        svg_parse_style(ctx, ctx.style_val);
    }
    if (should_parse_path) {
        res = svg_parse_path(ctx, ctx.d);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
static gfx_result svg_parse_css_selector(svg_parse_result& p, char** cur) {
    gfx_result res;
    char* tmp = nullptr;
    char* ss = *cur;
    svg_css_class* cls = nullptr;
    while (svg_isspace(*ss)) ++ss;
    if (*ss == '.') {
        cls = (svg_css_class*)p.allocator(sizeof(svg_css_class));
        if (cls == nullptr) {
            res = gfx_result::out_of_memory;
            goto error;
        }
        int i = sizeof(cls->selector) - 1;
        char* psz = cls->selector;
        while (i && *ss != '{') {
            if (!svg_isspace(*ss)) {
                *psz++ = *ss;
                --i;
            }
            ++ss;
        }
        *psz = '\0';
        while (*ss && *ss != '{') ++ss;
        if (!*ss) {
            res = gfx_result::invalid_format;
            goto error;
        }
        // skip '{'
        ++ss;
        size_t sz = 1;
        while (*ss && *ss != '}') {
            // skip whitespace
            while (*ss && svg_isspace(*ss)) ++ss;
            if (!*ss) {
                res = gfx_result::invalid_format;
                goto error;
            }
            char* se = ss;
            while (*se && *se != '}' && !svg_isspace(*se)) ++se;
            if (!*se) {
                res = gfx_result::invalid_format;
                goto error;
            }
            size_t new_sz = sz + (se - ss);
            if (tmp == nullptr) {
                tmp = (char*)p.allocator(new_sz + 1);
            } else {
                tmp = (char*)p.reallocator(tmp, new_sz + 1);
            }
            if (tmp == nullptr) {
                res = gfx_result::out_of_memory;
                goto error;
            }
            memcpy(tmp + sz - 1, ss, se - ss);
            tmp[sz + (se - ss) - 1] = '\0';
            sz = new_sz;
            ss += (se - ss + 1);
            if (*se == '}') break;
        }
        cls->value = tmp;
        cls->next = nullptr;
        if (p.css_classes == nullptr) {
            p.css_classes = cls;
        } else {
            p.css_class_tail->next = cls;
        }
        p.css_class_tail = cls;
        *cur = ss;
        return gfx_result::success;
    }
    // not supported. Skip this selector
    while (*ss && *ss != '}') ++ss;
    if (!*ss) {
        res = gfx_result::invalid_format;
        goto error;
    }
    if (!*ss) {
        *cur = ss;
        return gfx_result::canceled;
    }
    *cur = ss;
    return gfx_result::success;
error:
    if (cls != nullptr) {
        p.deallocator(cls);
    }
    if (tmp != nullptr) {
        p.deallocator(tmp);
    }
    return res;
}
static gfx_result svg_parse_style_elem(svg_parse_result& p) {
    // printf("parse <style>\n");
    reader_t& s = *p.reader;
    char* content = nullptr;
    size_t content_size = 0;
    while (s.read() && s.node_type() != ml_node_type::element_end) {
        if (s.node_type() == ml_node_type::content) {
            if (content_size == 0) {
                content_size = 1;
            }
            content_size += strlen(s.value());
            if (content == nullptr) {
                content = (char*)p.allocator(content_size);
                if (content == nullptr) {
                    return gfx_result::out_of_memory;
                }
                *content = '\0';
            } else {
                content = (char*)p.reallocator(content, content_size);
                if (content == nullptr) {
                    return gfx_result::out_of_memory;
                }
            }
            strcat(content, s.value());
        }
    }
    char* ss = content;
    gfx_result res = gfx_result::success;
    while (res == gfx_result::success) {
        res = svg_parse_css_selector(p, &ss);
        if (res != gfx_result::success) {
            p.deallocator(content);
            return res;
        }
    }
    p.deallocator(content);
    return gfx_result::success;
}
static gfx_result svg_parse_svg_elem(svg_parse_result& ctx) {
    // printf("parse <svg>\n");
    reader_t& s = *ctx.reader;
    while (s.read() && s.node_type() == ml_node_type::attribute) {
        if (0 == strcmp("width", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            ctx.image->dimensions.width = svg_parse_coordinate(ctx, s.value(), 0.0f, 0.0f);
        } else if (0 == strcmp("height", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            ctx.image->dimensions.height = svg_parse_coordinate(ctx, s.value(), 0.0f, 0.0f);
        } else if (0 == strcmp("viewBox", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            const char* ss = s.value();
            char buf[64];
            ss = svg_parse_number(ss, buf, 64);
            ctx.viewMinx = svg_atof(buf);
            while (*ss && (svg_isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = svg_parse_number(ss, buf, 64);
            ctx.viewMiny = svg_atof(buf);
            while (*ss && (svg_isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = svg_parse_number(ss, buf, 64);
            ctx.viewWidth = svg_atof(buf);
            while (*ss && (svg_isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = svg_parse_number(ss, buf, 64);
            ctx.viewHeight = svg_atof(buf);
        } else if (strcmp(s.value(), "preserveAspectRatio") == 0) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            if (strstr(s.value(), "none") != 0) {
                // No uniform scaling
                ctx.alignType = NSVG_ALIGN_NONE;
            } else {
                // Parse X align
                if (strstr(s.value(), "xMin") != 0)
                    ctx.alignX = NSVG_ALIGN_MIN;
                else if (strstr(s.value(), "xMid") != 0)
                    ctx.alignX = NSVG_ALIGN_MID;
                else if (strstr(s.value(), "xMax") != 0)
                    ctx.alignX = NSVG_ALIGN_MAX;
                // Parse X align
                if (strstr(s.value(), "yMin") != 0)
                    ctx.alignY = NSVG_ALIGN_MIN;
                else if (strstr(s.value(), "yMid") != 0)
                    ctx.alignY = NSVG_ALIGN_MID;
                else if (strstr(s.value(), "yMax") != 0)
                    ctx.alignY = NSVG_ALIGN_MAX;
                // Parse meet/slice
                ctx.alignType = NSVG_ALIGN_MEET;
                if (strstr(s.value(), "slice") != 0)
                    ctx.alignType = NSVG_ALIGN_SLICE;
            }
        } else {
            svg_parse_attr(ctx);
        }
        while ((s.node_type() == ml_node_type::attribute_content)) {
            if (!s.read()) {
                return gfx_result::invalid_format;
            }
        }
    }
    return gfx_result::success;
}

static void svg_parse_start_element(svg_parse_result& p) {
    reader_t& s = *p.reader;
    memcpy(p.lname, s.value(), sizeof(p.lname));
    // printf("parse <%s>\n",p.lname);
    if (p.defsFlag) {
        // Skip everything but gradients and styles in defs
        if (strcmp(s.value(), "linearGradient") == 0) {
            svg_parse_gradient_elem(p, svg_paint_type::linear_gradient);
        } else if (strcmp(s.value(), "radialGradient") == 0) {
            svg_parse_gradient_elem(p, svg_paint_type::radial_gradient);
        } else if (strcmp(s.value(), "stop") == 0) {
            svg_parse_gradient_stop_elem(p);
        } else if (strcmp(s.value(), "style") == 0) {
            svg_parse_style_elem(p);
        }
        // printf("%s in defs\n",s.value());
        return;
    }

    if (strcmp(s.value(), "g") == 0) {
        // printf("parse <g>\n");
        svg_push_attr(p);
        svg_parse_attribs(p);
    } else if (strcmp(s.value(), "path") == 0) {
        if (p.pathFlag)  // Do not allow nested paths.
            return;
        svg_push_attr(p);
        svg_parse_path_elem(p);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "rect") == 0) {
        svg_push_attr(p);
        svg_parse_rect_elem(p);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "circle") == 0) {
        svg_push_attr(p);
        svg_parse_circle_elem(p);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "ellipse") == 0) {
        svg_push_attr(p);
        svg_parse_ellipse_elem(p);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "line") == 0) {
        svg_push_attr(p);
        svg_parse_line_elem(p);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "polyline") == 0) {
        svg_push_attr(p);
        svg_parse_poly_elem(p, 0);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "polygon") == 0) {
        svg_push_attr(p);
        svg_parse_poly_elem(p, 1);
        svg_pop_attr(p);
    } else if (strcmp(s.value(), "linearGradient") == 0) {
        svg_parse_gradient_elem(p, svg_paint_type::linear_gradient);
    } else if (strcmp(s.value(), "radialGradient") == 0) {
        svg_parse_gradient_elem(p, svg_paint_type::radial_gradient);
    } else if (strcmp(s.value(), "stop") == 0) {
        svg_parse_gradient_stop_elem(p);
    } else if (strcmp(s.value(), "defs") == 0) {
        p.defsFlag = 1;
    } else if (strcmp(s.value(), "svg") == 0) {
        svg_parse_svg_elem(p);
    }
}

static void svg_parse_end_element(svg_parse_result& p) {
    reader_t& s = *p.reader;
    if (strcmp(s.is_empty_element() ? p.lname : s.value(), "g") == 0) {
        svg_pop_attr(p);
    } else if (strcmp(s.is_empty_element() ? p.lname : s.value(), "path") == 0) {
        p.pathFlag = 0;
    } else if (strcmp(s.is_empty_element() ? p.lname : s.value(), "defs") == 0) {
        // printf("end defs %d\n",(int)s.is_empty_element());
        p.defsFlag = 0;
    }
}

gfx_result svg_parse_document(reader_t& reader, svg_parse_result* result) {
    result->reader = &reader;
    while (reader.read()) {
        switch (reader.node_type()) {
            case ml_node_type::element:
                svg_parse_start_element(*result);
                break;
            case ml_node_type::element_end:
                svg_parse_end_element(*result);
                break;
            default:
                break;
        }
    }
    return gfx_result::success;
}

gfx_result svg_parse_to_image(stream* svg_stream, uint16_t dpi, svg_image** out_image, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    if (out_image == nullptr || svg_stream == nullptr || !svg_stream->caps().read) {
        return gfx_result::invalid_argument;
    }
    reader_t reader(svg_stream);
    void* p = allocator(sizeof(svg_parse_result));
    if (p == nullptr) {
        return gfx_result::out_of_memory;
    }
    svg_parse_result& parse_res = *(svg_parse_result*)p;
    memset(&parse_res, 0, sizeof(svg_parse_result));
    parse_res.image_size = 0;
    parse_res.image = (svg_image*)allocator(sizeof(svg_image));
    if (parse_res.image == nullptr) {
        return gfx_result::out_of_memory;
    }
    parse_res.image_size += sizeof(svg_image);
    memset(parse_res.image, 0, sizeof(svg_image));
    // Init style
    svg_xform_identity(parse_res.attr[0].xform);
    memset(parse_res.attr[0].id, 0, sizeof(parse_res.attr[0].id));
    parse_res.attr[0].fillColor = NSVG_RGB(0, 0, 0);
    parse_res.attr[0].strokeColor = NSVG_RGB(0, 0, 0);
    parse_res.attr[0].opacity = 1;
    parse_res.attr[0].fillOpacity = 1;
    parse_res.attr[0].strokeOpacity = 1;
    parse_res.attr[0].stopOpacity = 1;
    parse_res.attr[0].strokeWidth = 1;
    parse_res.attr[0].strokeLineJoin = svg_line_join::miter;
    parse_res.attr[0].strokeLineCap = svg_line_cap::butt;
    parse_res.attr[0].miterLimit = 4;
    parse_res.attr[0].fillRule = svg_fill_rule::non_zero;
    parse_res.attr[0].hasFill = 1;
    parse_res.attr[0].visible = 1;
    parse_res.allocator = allocator;
    parse_res.reallocator = reallocator;
    parse_res.deallocator = deallocator;
    parse_res.dpi = dpi;
    parse_res.d_size = 0;
    parse_res.css_classes = nullptr;
    parse_res.css_class_tail = nullptr;
    parse_res.d = nullptr;
    gfx_result res = svg_parse_document(reader, &parse_res);
    if (res != gfx_result::success) {
        return res;
    }

    // Create gradients after all definitions have been parsed
    svg_create_gradients(parse_res);

    // Scale to viewBox
    svg_scale_to_viewbox(parse_res, "px");
#ifdef SVG_DUMP_PARSE
    dump_parse_res(parse_res);
#endif
    *out_image = parse_res.image;
    parse_res.image = NULL;
    // printf("image size: %d kb\n",parse_res.image_size/1024);
    svg_delete_parse_result(parse_res);

    return gfx_result::success;
}
}