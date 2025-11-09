#include <string.h>
#include <gfx_canvas.hpp>
#include <gfx_math.hpp>
#include <ml_reader.hpp>

using namespace gfx;
using namespace ml;

//#define SVG_DUMP_PATH
#define MAX_STROKE_ARRAY 8
#define MAX_NAME 20
#define MAX_VALUE 128
#define MAX_ID 64
#define MAX_ATTRS 64

enum { BOUNDS_PATH = 0, BOUNDS_FILL = 1, BOUNDS_STROKE = 2 };
enum {
    COLOR_MODE_UNKNOWN = 0,
    COLOR_MODE_RGB,
    COLOR_MODE_HSL
};
enum {
    TAG_UNKNOWN = 0,
    TAG_CIRCLE,
    TAG_CLIP_PATH,  // TODO
    TAG_DEFS,
    TAG_ELLIPSE,
    TAG_G,
    TAG_IMAGE,
    TAG_LINE,
    TAG_LINEAR_GRADIENT,
    TAG_PATH,
    TAG_POLYGON,
    TAG_POLYLINE,
    TAG_RADIAL_GRADIENT,
    TAG_RECT,
    TAG_STOP,
    TAG_STYLE,
    TAG_SVG,
    TAG_SYMBOL,
    TAG_USE
};
enum {
    USER_SPACE = 0,
    OBJECT_SPACE = 1
};
enum {
    ATTR_UNKNOWN = 0,
    ATTR_CLASS,
    ATTR_CLIP_PATH,
    ATTR_CLIP_RULE,
    ATTR_CLIP_PATH_UNITS,
    ATTR_COLOR,
    ATTR_CX,
    ATTR_CY,
    ATTR_D,
    ATTR_DISPLAY,
    ATTR_FILL,
    ATTR_FILL_OPACITY,
    ATTR_FILL_RULE,
    ATTR_FONT_SIZE,
    ATTR_FX,
    ATTR_FY,
    ATTR_GRADIENT_TRANSFORM,
    ATTR_GRADIENT_UNITS,
    ATTR_HEIGHT,
    ATTR_ID,
    ATTR_OFFSET,
    ATTR_OPACITY,
    ATTR_POINTS,
    ATTR_PRESERVE_ASPECT_RATIO,
    ATTR_R,
    ATTR_RX,
    ATTR_RY,
    ATTR_SPREAD_METHOD,
    ATTR_STOP_COLOR,
    ATTR_STOP_OPACITY,
    ATTR_STROKE,
    ATTR_STROKE_DASHARRAY,
    ATTR_STROKE_DASHOFFSET,
    ATTR_STROKE_LINECAP,
    ATTR_STROKE_LINEJOIN,
    ATTR_STROKE_MITERLIMIT,
    ATTR_STROKE_OPACITY,
    ATTR_STROKE_WIDTH,
    ATTR_STYLE,
    ATTR_TRANSFORM,
    ATTR_VIEW_BOX,
    ATTR_VISIBILITY,
    ATTR_WIDTH,
    ATTR_X,
    ATTR_X1,
    ATTR_X2,
    ATTR_XLINK_HREF,
    ATTR_Y,
    ATTR_Y1,
    ATTR_Y2
};
enum {
    TRANS_ERROR = 0,
    TRANS_MATRIX,
    TRANS_ROTATE,
    TRANS_SKEW_X,
    TRANS_SKEW_Y,
    TRANS_SCALE,
    TRANS_TRANSLATE
};
enum {
    UNITS_USER = 0,
    UNITS_PX ,
    UNITS_PT ,
    UNITS_PC ,
    UNITS_MM ,
    UNITS_CM ,
    UNITS_IN ,
    UNITS_PERCENT,
    UNITS_EM ,
    UNITS_EX ,
    UNITS_DPI
};
typedef struct {
    const char name[MAX_NAME];
    int id;
} name_entry_t;

struct svg_coord {
    float value;
    char units;
}; 

static int svg_name_entry_compare(const void* a, const void* b) {
    const char* name = (const char*)a;
    const name_entry_t* entry = (const name_entry_t*)b;
    return strcmp(name, entry->name);
}

static int svg_lookupid(const char* data, const name_entry_t* table,
                        size_t count) {
    name_entry_t* entry =
        (name_entry_t*)bsearch(data, table, count / sizeof(name_entry_t),
                               sizeof(name_entry_t), svg_name_entry_compare);
    if (entry == NULL) return 0;
    return entry->id;
}

static int svg_elementid(const char* data) {
    static const name_entry_t table[] = {
        {"circle", TAG_CIRCLE},
        {"clipPath", TAG_CLIP_PATH},
        {"defs", TAG_DEFS},
        {"ellipse", TAG_ELLIPSE},
        {"g", TAG_G},
        {"image", TAG_IMAGE},
        {"line", TAG_LINE},
        {"linearGradient", TAG_LINEAR_GRADIENT},
        {"path", TAG_PATH},
        {"polygon", TAG_POLYGON},
        {"polyline", TAG_POLYLINE},
        {"radialGradient", TAG_RADIAL_GRADIENT},
        {"rect", TAG_RECT},
        {"stop", TAG_STOP},
        {"style", TAG_STYLE},
        {"svg", TAG_SVG},
        {"symbol", TAG_SYMBOL},
        {"use", TAG_USE}};

    return svg_lookupid(data, table, sizeof(table));
}

static int svg_attributeid(const char* data) {
    static const name_entry_t table[] = {
        {"class", ATTR_CLASS},
        {"clip-path", ATTR_CLIP_PATH},
        {"clip-rule", ATTR_CLIP_RULE},
        {"clipPathUnits", ATTR_CLIP_PATH_UNITS},
        {"color", ATTR_COLOR},
        {"cx", ATTR_CX},
        {"cy", ATTR_CY},
        {"d", ATTR_D},
        {"display", ATTR_DISPLAY},
        {"fill", ATTR_FILL},
        {"fill-opacity", ATTR_FILL_OPACITY},
        {"fill-rule", ATTR_FILL_RULE},
        {"font-size", ATTR_FONT_SIZE},
        {"fx", ATTR_FX},
        {"fy", ATTR_FY},
        {"gradientTransform", ATTR_GRADIENT_TRANSFORM},
        {"gradientUnits", ATTR_GRADIENT_UNITS},
        {"height", ATTR_HEIGHT},
        {"id", ATTR_ID},
        {"offset", ATTR_OFFSET},
        {"opacity", ATTR_OPACITY},
        {"points", ATTR_POINTS},
        {"preserveAspectRatio", ATTR_PRESERVE_ASPECT_RATIO},
        {"r", ATTR_R},
        {"rx", ATTR_RX},
        {"ry", ATTR_RY},
        {"spreadMethod", ATTR_SPREAD_METHOD},
        {"stop-color", ATTR_STOP_COLOR},
        {"stop-opacity", ATTR_STOP_OPACITY},
        {"stroke", ATTR_STROKE},
        {"stroke-dasharray", ATTR_STROKE_DASHARRAY},
        {"stroke-dashoffset", ATTR_STROKE_DASHOFFSET},
        {"stroke-linecap", ATTR_STROKE_LINECAP},
        {"stroke-linejoin", ATTR_STROKE_LINEJOIN},
        {"stroke-miterlimit", ATTR_STROKE_MITERLIMIT},
        {"stroke-opacity", ATTR_STROKE_OPACITY},
        {"stroke-width", ATTR_STROKE_WIDTH},
        {"style", ATTR_STYLE},
        {"transform", ATTR_TRANSFORM},
        {"viewBox", ATTR_VIEW_BOX},
        {"visibility", ATTR_VISIBILITY},
        {"width", ATTR_WIDTH},
        {"x", ATTR_X},
        {"x1", ATTR_X1},
        {"x2", ATTR_X2},
        {"xlink:href", ATTR_XLINK_HREF},
        {"y", ATTR_Y},
        {"y1", ATTR_Y1},
        {"y2", ATTR_Y2}};

    return svg_lookupid(data, table, sizeof(table));
}

/*static int svg_cssattributeid(const char* data) {
    static const name_entry_t table[] = {
        {"clip-path", ATTR_CLIP_PATH},
        {"clip-rule", ATTR_CLIP_RULE},
        {"color", ATTR_COLOR},
        {"display", ATTR_DISPLAY},
        {"fill", ATTR_FILL},
        {"fill-opacity", ATTR_FILL_OPACITY},
        {"fill-rule", ATTR_FILL_RULE},
        {"opacity", ATTR_OPACITY},
        {"stop-color", ATTR_STOP_COLOR},
        {"stop-opacity", ATTR_STOP_OPACITY},
        {"stroke", ATTR_STROKE},
        {"stroke-dasharray", ATTR_STROKE_DASHARRAY},
        {"stroke-dashoffset", ATTR_STROKE_DASHOFFSET},
        {"stroke-linecap", ATTR_STROKE_LINECAP},
        {"stroke-linejoin", ATTR_STROKE_LINEJOIN},
        {"stroke-miterlimit", ATTR_STROKE_MITERLIMIT},
        {"stroke-opacity", ATTR_STROKE_OPACITY},
        {"stroke-width", ATTR_STROKE_WIDTH},
        {"visibility", ATTR_VISIBILITY}};

    return svg_lookupid(data, table, sizeof(table));
}*/

#define SVG_RGB(r, g, b) (::gfx::vector_pixel(255,r,g,b))
#define SVG_RGBA(r, g, b, a) (::gfx::vector_pixel(a,r,g,b))
#define SVG_HSL(h, s, l)                                           \
    (::gfx::convert<::gfx::hsla_pixel<32>, ::gfx::vector_pixel>( \
        ::gfx::hsla_pixel<32>(h, s, l,255)))

#define SVG_ALIGN_MIN 0
#define SVG_ALIGN_MID 1
#define SVG_ALIGN_MAX 2
#define SVG_ALIGN_NONE 0
#define SVG_ALIGN_MEET 1
#define SVG_ALIGN_SLICE 2

#define SVG_PI 3.14159265358979323846f
#define SVG_TWO_PI 6.28318530717958647693f
#define SVG_HALF_PI 1.57079632679489661923f
#define SVG_SQRT2 1.41421356237309504880f
#define SVG_KAPPA 0.55228474983079339840f
#define SVG_DEG2RAD(x) ((x) * (SVG_PI / 180.0f))
#define SVG_RAD2DEG(x) ((x) * (180.0f / SVG_PI))

typedef ::gfx::gfx_result result_t;
#define FMT_ERROR ::gfx::gfx_result::invalid_format
#define IO_ERROR ::gfx::gfx_result::io_error
#define SUCCESS ::gfx::gfx_result::success
#define OUT_OF_MEMORY ::gfx::gfx_result::out_of_memory
#define NOT_SUPPORTED ::gfx::gfx_result::not_supported
#define INVALID_ARG ::gfx::gfx_result::invalid_argument
#define INVALID_STATE ::gfx::gfx_result::invalid_state
#define SUCCEEDED(x) ((x) == SUCCESS)

struct svg_named_color {
    const char* name;
    ::gfx::vector_pixel color;
};
typedef enum {
    view_align_none,
    view_align_x_min_y_min,
    view_align_x_mid_y_min,
    view_align_x_max_y_min,
    view_align_x_min_y_mid,
    view_align_x_mid_y_mid,
    view_align_x_max_y_mid,
    view_align_x_min_y_max,
    view_align_x_mid_y_max,
    view_align_x_max_y_max
} view_align_t;
typedef enum { view_scale_meet, view_scale_slice } view_scale_t;

struct svg_attrib {
    char id[MAX_ID];
    ::gfx::matrix xform;
    ::gfx::vector_pixel fillColor;
    ::gfx::vector_pixel strokeColor;
    ::gfx::vector_pixel color;
    float opacity;
    float fillOpacity;
    float strokeOpacity;
    char fillGradient[MAX_ID];
    char strokeGradient[MAX_ID];
    float strokeWidth;
    float strokeDashOffset;
    float strokeDashArray[MAX_STROKE_ARRAY];
    size_t strokeDashCount;
    ::gfx::line_join strokeLineJoin;
    ::gfx::line_cap strokeLineCap;
    float miterLimit;
    ::gfx::fill_rule fillRule;
    float fontSize;
    ::gfx::vector_pixel stopColor;
    float stopOpacity;
    svg_coord stopOffset;
    char hasFill;
    char hasStroke;
    char visible;
};
struct svg_css_has {
    long id : 1;
    long xform : 1;
    long color : 1;
    long fillColor : 1;
    long strokeColor : 1;
    long opacity : 1;
    long fillOpacity : 1;
    long strokeOpacity : 1;

    long fillGradient : 1;
    long strokeGradient : 1;
    long strokeWidth : 1;
    long strokeDashArray : 1;
    long strokeDashOffset : 1;
    long strokeLineJoin : 1;
    long strokeLineCap : 1;
    long miterLimit : 1;

    long fillRule : 1;
    long fontSize : 1;
    long stopOpacity : 1;
    long stopOffset : 1;
    long stopColor : 1;
    long hasFill : 1;
    long hasStroke : 1;
    long visible : 1;
};
struct svg_css_data {
    bool is_id;
    char selector[MAX_NAME];
    svg_css_has has;
    svg_attrib data;
    svg_css_data* next;
};
struct svg_grad_stop {
    float opacity;
    svg_coord offset;
    vector_pixel color;
};
struct svg_grad_data {
    char id[64];
    char ref[64];
    char units;
    gradient_type type;
    spread_method spread;
    ::gfx::matrix transform;
    union {
        struct {
            svg_coord x1,y1,x2,y2;
        } linear;
        struct {
            svg_coord cx,cy,fx,fy,cr,fr;
        } radial;
    };
    svg_grad_stop* stops;
    size_t stops_size;
    struct svg_grad_data* next;
};
struct svg_context {
    ::ml::ml_reader_ex<64> rdr;
    ::gfx::matrix xform;
    ::gfx::canvas* cvs;
    ::gfx::sizef dimensions;
    ::gfx::rectf bounds;
    float view_box_min_x, view_box_min_y, view_box_width, view_box_height;
    float view_width, view_height;
    int align_type;
    int align_scale;
    float dpi;
    svg_grad_data* grad_head;
    svg_grad_data* grad_tail;
    svg_css_data* css_current;
    union {
        long data;
        svg_css_has attr;
    } css_has;
    svg_css_data* css_head;
    svg_css_data* css_tail;
    svg_attrib attrs[MAX_ATTRS];
    size_t attrs_head;
    char tmp_name[MAX_NAME];
    char tmp_value[MAX_VALUE];
    bool in_path;
    bool in_defs;
    int tag_id;
    void*(*allocator)(size_t);
    void*(*reallocator)(void*,size_t);
    void(*deallocator)(void*);
};
static result_t svg_parse_attribute_id(svg_context& ctx, const char** current,
                                       int id, bool is_css);
static gfx_result svg_draw_path(svg_context& ctx,const canvas_path& path);         
void svg_init_attribute(svg_attrib& a) {
    a.xform = ::gfx::matrix::create_identity();
    a.id[0] = 0;
    a.color = SVG_RGB(0, 0, 0);
    a.fillColor = SVG_RGB(0, 0, 0);
    a.strokeColor = SVG_RGB(0, 0, 0);
    a.fillGradient[0] = 0;
    a.strokeGradient[0] = 0;
    a.opacity = 1.0f;
    a.fillOpacity = 1.0f;
    a.strokeOpacity = 1.0f;
    a.stopOpacity = 1.0f;
    a.strokeDashCount = 0;
    a.strokeWidth = 1.0f;
    a.strokeLineJoin = ::gfx::line_join::miter;
    a.strokeLineCap = ::gfx::line_cap::butt;
    a.miterLimit = 4.0f;
    a.fillRule = ::gfx::fill_rule::non_zero;
    a.hasFill = 1;
    a.hasStroke = 0;
    a.visible = 1;
    a.fontSize = 1.0f;
    a.stopColor = SVG_RGB(0, 0, 0);
    a.stopOffset = {0.0f, UNITS_USER};
    a.stopOpacity = 1.0f;
    a.strokeDashOffset = 0.0f;
}
static void svg_init_context(svg_context& ctx) {
    ctx.attrs_head = 0;
    svg_init_attribute(ctx.attrs[0]);
    ctx.dpi = 96;
    ctx.xform = matrix::create_identity();
    ctx.in_defs = false;
    ctx.in_path = false;
    ctx.tag_id = 0;
    ctx.align_scale = view_scale_meet;
    ctx.align_type = view_align_x_mid_y_mid;
    ctx.grad_head = nullptr;
    ctx.grad_tail = nullptr;
    ctx.css_current = nullptr;
    ctx.css_head = nullptr;
    ctx.css_tail = nullptr;
    ctx.tmp_name[0] = 0;
    ctx.tmp_value[0] = 0;
    ctx.dimensions = {0,0};
    ctx.view_box_min_x = 0;
    ctx.view_box_min_y = 0;
    ctx.view_box_width = 0;
    ctx.view_box_height = 0;
    ctx.view_width = 0;
    ctx.view_height = 0;
}
static void svg_delete_context(svg_context& ctx) {
    svg_grad_data* gd = ctx.grad_head;
    while(gd!=nullptr) {
        if(gd->stops!=nullptr) {
            ctx.deallocator(gd->stops);
        }
        svg_grad_data* next = gd->next;
        ctx.deallocator(gd);
        gd = next;
    }
    ctx.grad_head = nullptr;
    ctx.grad_tail = nullptr;
    svg_css_data* css = ctx.css_head;
    while(css!=nullptr) {
        svg_css_data* next = css->next;
        ctx.deallocator(css);
        css=next;
    }
    ctx.css_head = nullptr;
    ctx.css_tail = nullptr;
}

static result_t svg_push_attr(svg_context& ctx) {
    if (ctx.attrs_head < MAX_ATTRS - 1) {
        ++ctx.attrs_head;
        svg_attrib& lhs = ctx.attrs[ctx.attrs_head];
        svg_attrib& rhs = ctx.attrs[ctx.attrs_head - 1];
        lhs.color = rhs.color;
        lhs.fillColor = rhs.fillColor;
        memcpy(lhs.fillGradient, rhs.fillGradient, MAX_ID);
        lhs.fillOpacity = rhs.fillOpacity;
        lhs.fillRule = rhs.fillRule;
        lhs.fontSize = rhs.fontSize;
        lhs.hasFill = rhs.hasFill;
        lhs.hasStroke = rhs.hasStroke;
        memcpy(lhs.id, rhs.id, MAX_ID);
        lhs.miterLimit = rhs.miterLimit;
        lhs.opacity = rhs.opacity;
        lhs.stopColor = rhs.stopColor;
        lhs.stopOffset = rhs.stopOffset;
        lhs.stopOpacity = rhs.stopOpacity;
        lhs.strokeColor = rhs.strokeColor;
        if (rhs.strokeDashCount) {
            memcpy(lhs.strokeDashArray, rhs.strokeDashArray,
                   sizeof(float) * rhs.strokeDashCount);
        }
        lhs.strokeDashCount = rhs.strokeDashCount;
        lhs.strokeDashOffset = rhs.strokeDashOffset;
        memcpy(lhs.strokeGradient, rhs.strokeGradient, MAX_ID);
        lhs.strokeLineCap = rhs.strokeLineCap;
        lhs.strokeLineJoin = rhs.strokeLineJoin;
        lhs.strokeOpacity = rhs.strokeOpacity;
        lhs.strokeWidth = rhs.strokeWidth;
        lhs.visible = rhs.visible;
        lhs.xform = rhs.xform;
        return SUCCESS;
    }
    return OUT_OF_MEMORY;
}
static void svg_pop_attr(svg_context& ctx) {
    if (ctx.attrs_head > 0) {
        --ctx.attrs_head;
    }
}

static svg_attrib& svg_get_attr(svg_context& ctx) {
    return ctx.attrs[ctx.attrs_head];
}
bool svg_isspace(char ch) { return strchr(" \t\n\v\f\r", ch) != 0; }
bool svg_isalpha(char ch) {
    return ((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z');
}
static int svg_isdigit(char ch) { return ch >= '0' && ch <= '9'; }
bool svg_isalnum(char ch) {
    return ((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z') ||
           ((ch) >= '0' && ch <= '9');
}
static int svg_ishexdigit(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}
static int svg_hexdigit(char ch) {
    int res;
    if (ch >= 'a') {
        res = ch - 'a' + 10;
    } else if (ch >= 'A') {
        res = ch - 'A' + 10;
    } else {
        res = ch - '0';
    }
    return res;
}
result_t svg_ensure_current(svg_context& ctx, const char** current) {
    if (*current == nullptr) return SUCCESS;
    if (!**current) {
        if (!ctx.rdr.read()) return IO_ERROR;
        if (!ctx.rdr.has_value()) {
            *current = nullptr;
            return SUCCESS;
        }
        *current = ctx.rdr.value();
    }
    return SUCCESS;
}

static result_t svg_skip_space(svg_context& ctx, const char** current) {
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        while (**current && svg_isspace(**current)) {
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }

    return SUCCESS;
}
static result_t svg_skip_space_or_comma(svg_context& ctx, const char** current) {
    if (*current == nullptr) {
        return SUCCESS;
    }
    result_t res = svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) {
        return res;
    }
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) {
        return res;
    }
    if(*current!=nullptr && **current==',') {
        ++(*current);
    }
    res = svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) {
        return res;
    }
    
    return SUCCESS;
}

static result_t svg_skip_to(svg_context& ctx, const char** current, char delim,
                            bool consume) {
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        while (**current && **current != delim) {
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }
    if (consume && *current) {
        if (**current == delim) {
            ++(*current);
            if (!**current) {
                if (!ctx.rdr.read()) {
                    return IO_ERROR;
                }
                if (!ctx.rdr.has_value()) {
                    *current = nullptr;
                } else {
                    *current = ctx.rdr.value();
                }
            }
        }
    }
    return SUCCESS;
}
static result_t svg_skip_css_val(svg_context& ctx, const char** current) {
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        while (**current && **current != ';' && **current!='}') {
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }
    return SUCCESS;
}
static svg_css_data* svg_find_css_data(svg_context& ctx,const char* sel, bool is_id) {
    svg_css_data* css = ctx.css_head;
    while(css!=nullptr) {
        if(is_id==css->is_id && 0==strcmp(sel,css->selector)) {
            return css;
        }
        css = css->next;
    }
    return nullptr;
}
static result_t svg_parse_path_flag(svg_context& ctx, const char** current,
                                    bool* value) {
    result_t res = svg_ensure_current(ctx,current);
    if(current==nullptr) {
        return IO_ERROR;
    }
    *value = false;
    while (true) {
        res=svg_skip_space_or_comma(ctx,current); if(!SUCCEEDED(res)) return res;
        res=svg_ensure_current(ctx,current);if(!SUCCEEDED(res)) return res;
        if(current==nullptr) {
            return IO_ERROR;
        }
    
        if (**current == '0') {
            *value = false;
            ++(*current);
            return SUCCESS;
        } else if (**current == '1') {
            *value = true;
            ++(*current);
                return SUCCESS;
        } else {
            return FMT_ERROR;
        }
        break;
    
    }

    return SUCCESS;
}
/*static result_t svg_parse_id(svg_context& ctx, const char** current,
                                    char* out_id,size_t max_len) {
    if(*current==nullptr) {
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        if (!ctx.rdr.has_value()) {
            *current = nullptr;
            return FMT_ERROR;
        }
        *current = ctx.rdr.value();
    }
    *out_id = 0;
    while (*current!=nullptr) {
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                return SUCCESS;
            }
            *current = ctx.rdr.value();
        } else {
            if(!svg_isalnum(**current)) {
                break;
            }
            if(max_len>0 && max_len--) {
                *(out_id++)=**current;
                *out_id=0;
            }
            ++(*current);
        }

    }
    return SUCCESS;
}*/
static result_t svg_parse_ref(svg_context& ctx, const char** current,
                                    char* out_id,size_t max_len) {
    if(*current==nullptr) {
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        if (!ctx.rdr.has_value()) {
            *current = nullptr;
            return FMT_ERROR;
        }
        *current = ctx.rdr.value();
    }
    if(**current!='#') {
        return FMT_ERROR;
    }
    ++(*current);
    *out_id = 0;
    while (true) {
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                return SUCCESS;
            }
            *current = ctx.rdr.value();
        } else {
            if(!svg_isalnum(**current)) {
                break;
            }
            if(max_len>1 && max_len--) {
                *(out_id++)=**current;
                *out_id = 0;
            }
            ++(*current);
        }
    }

    return SUCCESS;
}
/*static result_t svg_parse_int(svg_context& ctx, const char** current,
                              int* result) {
    char* end = NULL;
    int sign = 1.0;
    int res;
    int intPart = 0;
    char hasIntPart = 0;
    int state = 0;
    if(*current==nullptr) {
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        if (!ctx.rdr.has_value()) {
            *current = nullptr;
            return FMT_ERROR;
        }
        *current = ctx.rdr.value();
    }
    // Parse optional sign
    if (**current == '+') {
        (*current)++;
    } else if (**current == '-') {
        sign = -1;
        (*current)++;
    }
    while (state < 2) {
        if (**current) {
            switch (state) {
                case 0:  // int part
                    if (!svg_isdigit(**current)) {
                        state = 1;
                        break;
                    }
                    hasIntPart = 1;
                    intPart = (intPart * 10) + (**current - '0');
                    ++(*current);
                    break;
                case 1:
                    *result = intPart * sign;
                    state = 2;
                    break;
            }

        } else {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        }
    }

    if (!hasIntPart) {
        return FMT_ERROR;
    }
    return SUCCESS;
}*/
static result_t svg_parse_hex(svg_context& ctx, const char** current,
                              unsigned int* result, size_t* digits) {
    //char* end = NULL;
    if (digits) {
        *digits = 0;
    }
    unsigned int intPart = 0;
    char hasIntPart = 0;
    int state = 0;
    while (state < 1) {
        if (**current) {
            switch (state) {
                case 0:  // int part
                    if (!svg_ishexdigit(**current)) {
                        state = 1;
                        break;
                    }
                    if (digits) {
                        ++(*digits);
                    }
                    hasIntPart = 1;
                    intPart = (intPart * 0x10) + (svg_hexdigit(**current));
                    ++(*current);
                    break;
            }

        } else {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        }
    }

    if (!hasIntPart) {
        return FMT_ERROR;
    }
    *result = intPart;
    return SUCCESS;
}
static result_t svg_parse_float(svg_context& ctx, const char** current,
                                bool exp, float* result) {
    //char* end = NULL;
    double sign = 1.0;
    long long intPart = 0, fracPart = 0;
    int fracCount = 0;
    long expPart = 0;
    char expNeg = 0;
    char hasIntPart = 0, hasFracPart = 0, hasExpPart = 0;
    int state = 0;
    if(*current==nullptr) {
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        if (!ctx.rdr.has_value()) {
            *current = nullptr;
            return FMT_ERROR;
        }
        *current = ctx.rdr.value();
    }
    // Parse optional sign
    if (**current == '+') {
        (*current)++;
    } else if (**current == '-') {
        sign = -1;
        (*current)++;
    }

    while (state < 6) {
        if (**current) {
            switch (state) {
                case 0:  // int part
                    if (!svg_isdigit(**current)) {
                        state = 1;
                        break;
                    }
                    hasIntPart = 1;
                    intPart = (intPart * 10) + (**current - '0');
                    //*result = (float)intPart;
                    ++(*current);
                    break;
                case 1:
                    if (**current != '.') {
                        state = 3;
                        break;
                    }
                    ++(*current);
                    state = 2;
                    break;
                case 2:  // frac part
                    if (!svg_isdigit(**current)) {
                        state = 3;
                        break;
                    }
                    ++fracCount;
                    hasFracPart = 1;
                    fracPart = (fracPart * 10) + (**current - '0');
                    ++(*current);
                    //if (!**current) {
                        //*result +=
                        //    (double)fracPart / pow(10.0, (double)fracCount);
                        //hasFracPart=false;
                    //}
                    break;
                case 3:
                    //if (hasFracPart) {
                    //    *result +=
                    //        (double)fracPart / pow(10.0, (double)fracCount);
                    //    hasFracPart=false;
                    //}
                    if (exp && (**current == 'E' || **current == 'e')) {
                        ++(*current);
                        state = 4;
                    } else {
                        state = 6;
                    }
                    break;
                case 4:
                    if (**current == '+') {
                        ++(*current);
                    }
                    if (**current == '-') {
                        expNeg = 1;
                        ++(*current);
                    }
                    state = 5;
                    break;
                case 5:
                    if (!svg_isdigit(**current)) {
                        state = 6;
                        break;
                    }
                    hasExpPart = 1;
                    expPart = (expPart * 10) + (**current - '0');
                    ++(*current);
                    break;
            }

        } else {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        }
    }

    if (!hasIntPart && !hasFracPart) {
        return FMT_ERROR;
    }
    if(hasIntPart) {
        *result = intPart;
    } else {
        *result = 0;
    }
    if(hasFracPart) {
         *result += (double)fracPart / pow(10.0, (double)fracCount);
    }
    if (hasExpPart) {
        if (expNeg) {
            expPart = -expPart;
        }
        *result *= pow(10.0, (double)expPart);
    }
    *result *= sign;

    return SUCCESS;
}
result_t svg_parse_alpha_str(svg_context& ctx, const char** current,
                             char* value, size_t max_len) {
    size_t len = 0;
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        while (**current) {
            if (!svg_isalpha(**current)) {
                *value = 0;
                return SUCCESS;
            }
            if (len < max_len - 1) {
                *(value++) = **current;
                ++len;
            }
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }

    *value = 0;
    return SUCCESS;
}

result_t svg_parse_alphanum_str(svg_context& ctx, const char** current,
                                char* value, size_t max_len) {
    size_t len = 0;
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        while (**current) {
            if (!svg_isalnum(**current)) {
                *value = 0;
                return SUCCESS;
            }
            if (len < max_len - 1) {
                *(value++) = **current;
                ++len;
            }
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }

    *value = 0;
    return SUCCESS;
}
result_t svg_parse_name_str(svg_context& ctx, const char** current, char* value,
                            size_t max_len) {
    size_t len = 0;
    if (*current == nullptr) {
        return FMT_ERROR;
    }
    result_t res = svg_skip_space(ctx, current);
    if (!SUCCEEDED(res)) {
        return res;
    }
    if (!*current) {
        return FMT_ERROR;
    }
    while (true) {
        while (**current) {
            if (!(svg_isalnum(**current) || '-' == **current ||
                  '_' == **current)) {
                *value = 0;
                res = svg_skip_space(ctx, current);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                return SUCCESS;
            }
            if (len < max_len - 1) {
                *(value++) = **current;
                ++len;
            }
            ++(*current);
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            break;
        }
    }

    *value = 0;
    res = svg_skip_space(ctx, current);
    if (!SUCCEEDED(res)) {
        return res;
    }
    return SUCCESS;
}
static svg_named_color svg_colors[] = {

    {"red", SVG_RGB(255, 0, 0)},
    {"green", SVG_RGB(0, 128, 0)},
    {"blue", SVG_RGB(0, 0, 255)},
    {"yellow", SVG_RGB(255, 255, 0)},
    {"cyan", SVG_RGB(0, 255, 255)},
    {"magenta", SVG_RGB(255, 0, 255)},
    {"black", SVG_RGB(0, 0, 0)},
    {"grey", SVG_RGB(128, 128, 128)},
    {"gray", SVG_RGB(128, 128, 128)},
    {"white", SVG_RGB(255, 255, 255)},

#ifdef GFX_SVG_ALL_NAMED_COLORS
    {"aliceblue", SVG_RGB(240, 248, 255)},
    {"antiquewhite", SVG_RGB(250, 235, 215)},
    {"aqua", SVG_RGB(0, 255, 255)},
    {"aquamarine", SVG_RGB(127, 255, 212)},
    {"azure", SVG_RGB(240, 255, 255)},
    {"beige", SVG_RGB(245, 245, 220)},
    {"bisque", SVG_RGB(255, 228, 196)},
    {"blanchedalmond", SVG_RGB(255, 235, 205)},
    {"blueviolet", SVG_RGB(138, 43, 226)},
    {"brown", SVG_RGB(165, 42, 42)},
    {"burlywood", SVG_RGB(222, 184, 135)},
    {"cadetblue", SVG_RGB(95, 158, 160)},
    {"chartreuse", SVG_RGB(127, 255, 0)},
    {"chocolate", SVG_RGB(210, 105, 30)},
    {"coral", SVG_RGB(255, 127, 80)},
    {"cornflowerblue", SVG_RGB(100, 149, 237)},
    {"cornsilk", SVG_RGB(255, 248, 220)},
    {"crimson", SVG_RGB(220, 20, 60)},
    {"darkblue", SVG_RGB(0, 0, 139)},
    {"darkcyan", SVG_RGB(0, 139, 139)},
    {"darkgoldenrod", SVG_RGB(184, 134, 11)},
    {"darkgray", SVG_RGB(169, 169, 169)},
    {"darkgreen", SVG_RGB(0, 100, 0)},
    {"darkgrey", SVG_RGB(169, 169, 169)},
    {"darkkhaki", SVG_RGB(189, 183, 107)},
    {"darkmagenta", SVG_RGB(139, 0, 139)},
    {"darkolivegreen", SVG_RGB(85, 107, 47)},
    {"darkorange", SVG_RGB(255, 140, 0)},
    {"darkorchid", SVG_RGB(153, 50, 204)},
    {"darkred", SVG_RGB(139, 0, 0)},
    {"darksalmon", SVG_RGB(233, 150, 122)},
    {"darkseagreen", SVG_RGB(143, 188, 143)},
    {"darkslateblue", SVG_RGB(72, 61, 139)},
    {"darkslategray", SVG_RGB(47, 79, 79)},
    {"darkslategrey", SVG_RGB(47, 79, 79)},
    {"darkturquoise", SVG_RGB(0, 206, 209)},
    {"darkviolet", SVG_RGB(148, 0, 211)},
    {"deeppink", SVG_RGB(255, 20, 147)},
    {"deepskyblue", SVG_RGB(0, 191, 255)},
    {"dimgray", SVG_RGB(105, 105, 105)},
    {"dimgrey", SVG_RGB(105, 105, 105)},
    {"dodgerblue", SVG_RGB(30, 144, 255)},
    {"firebrick", SVG_RGB(178, 34, 34)},
    {"floralwhite", SVG_RGB(255, 250, 240)},
    {"forestgreen", SVG_RGB(34, 139, 34)},
    {"fuchsia", SVG_RGB(255, 0, 255)},
    {"gainsboro", SVG_RGB(220, 220, 220)},
    {"ghostwhite", SVG_RGB(248, 248, 255)},
    {"gold", SVG_RGB(255, 215, 0)},
    {"goldenrod", SVG_RGB(218, 165, 32)},
    {"greenyellow", SVG_RGB(173, 255, 47)},
    {"honeydew", SVG_RGB(240, 255, 240)},
    {"hotpink", SVG_RGB(255, 105, 180)},
    {"indianred", SVG_RGB(205, 92, 92)},
    {"indigo", SVG_RGB(75, 0, 130)},
    {"ivory", SVG_RGB(255, 255, 240)},
    {"khaki", SVG_RGB(240, 230, 140)},
    {"lavender", SVG_RGB(230, 230, 250)},
    {"lavenderblush", SVG_RGB(255, 240, 245)},
    {"lawngreen", SVG_RGB(124, 252, 0)},
    {"lemonchiffon", SVG_RGB(255, 250, 205)},
    {"lightblue", SVG_RGB(173, 216, 230)},
    {"lightcoral", SVG_RGB(240, 128, 128)},
    {"lightcyan", SVG_RGB(224, 255, 255)},
    {"lightgoldenrodyellow", SVG_RGB(250, 250, 210)},
    {"lightgray", SVG_RGB(211, 211, 211)},
    {"lightgreen", SVG_RGB(144, 238, 144)},
    {"lightgrey", SVG_RGB(211, 211, 211)},
    {"lightpink", SVG_RGB(255, 182, 193)},
    {"lightsalmon", SVG_RGB(255, 160, 122)},
    {"lightseagreen", SVG_RGB(32, 178, 170)},
    {"lightskyblue", SVG_RGB(135, 206, 250)},
    {"lightslategray", SVG_RGB(119, 136, 153)},
    {"lightslategrey", SVG_RGB(119, 136, 153)},
    {"lightsteelblue", SVG_RGB(176, 196, 222)},
    {"lightyellow", SVG_RGB(255, 255, 224)},
    {"lime", SVG_RGB(0, 255, 0)},
    {"limegreen", SVG_RGB(50, 205, 50)},
    {"linen", SVG_RGB(250, 240, 230)},
    {"maroon", SVG_RGB(128, 0, 0)},
    {"mediumaquamarine", SVG_RGB(102, 205, 170)},
    {"mediumblue", SVG_RGB(0, 0, 205)},
    {"mediumorchid", SVG_RGB(186, 85, 211)},
    {"mediumpurple", SVG_RGB(147, 112, 219)},
    {"mediumseagreen", SVG_RGB(60, 179, 113)},
    {"mediumslateblue", SVG_RGB(123, 104, 238)},
    {"mediumspringgreen", SVG_RGB(0, 250, 154)},
    {"mediumturquoise", SVG_RGB(72, 209, 204)},
    {"mediumvioletred", SVG_RGB(199, 21, 133)},
    {"midnightblue", SVG_RGB(25, 25, 112)},
    {"mintcream", SVG_RGB(245, 255, 250)},
    {"mistyrose", SVG_RGB(255, 228, 225)},
    {"moccasin", SVG_RGB(255, 228, 181)},
    {"navajowhite", SVG_RGB(255, 222, 173)},
    {"navy", SVG_RGB(0, 0, 128)},
    {"oldlace", SVG_RGB(253, 245, 230)},
    {"olive", SVG_RGB(128, 128, 0)},
    {"olivedrab", SVG_RGB(107, 142, 35)},
    {"orange", SVG_RGB(255, 165, 0)},
    {"orangered", SVG_RGB(255, 69, 0)},
    {"orchid", SVG_RGB(218, 112, 214)},
    {"palegoldenrod", SVG_RGB(238, 232, 170)},
    {"palegreen", SVG_RGB(152, 251, 152)},
    {"paleturquoise", SVG_RGB(175, 238, 238)},
    {"palevioletred", SVG_RGB(219, 112, 147)},
    {"papayawhip", SVG_RGB(255, 239, 213)},
    {"peachpuff", SVG_RGB(255, 218, 185)},
    {"peru", SVG_RGB(205, 133, 63)},
    {"pink", SVG_RGB(255, 192, 203)},
    {"plum", SVG_RGB(221, 160, 221)},
    {"powderblue", SVG_RGB(176, 224, 230)},
    {"purple", SVG_RGB(128, 0, 128)},
    {"rosybrown", SVG_RGB(188, 143, 143)},
    {"royalblue", SVG_RGB(65, 105, 225)},
    {"saddlebrown", SVG_RGB(139, 69, 19)},
    {"salmon", SVG_RGB(250, 128, 114)},
    {"sandybrown", SVG_RGB(244, 164, 96)},
    {"seagreen", SVG_RGB(46, 139, 87)},
    {"seashell", SVG_RGB(255, 245, 238)},
    {"sienna", SVG_RGB(160, 82, 45)},
    {"silver", SVG_RGB(192, 192, 192)},
    {"skyblue", SVG_RGB(135, 206, 235)},
    {"slateblue", SVG_RGB(106, 90, 205)},
    {"slategray", SVG_RGB(112, 128, 144)},
    {"slategrey", SVG_RGB(112, 128, 144)},
    {"snow", SVG_RGB(255, 250, 250)},
    {"springgreen", SVG_RGB(0, 255, 127)},
    {"steelblue", SVG_RGB(70, 130, 180)},
    {"tan", SVG_RGB(210, 180, 140)},
    {"teal", SVG_RGB(0, 128, 128)},
    {"thistle", SVG_RGB(216, 191, 216)},
    {"tomato", SVG_RGB(255, 99, 71)},
    {"turquoise", SVG_RGB(64, 224, 208)},
    {"violet", SVG_RGB(238, 130, 238)},
    {"wheat", SVG_RGB(245, 222, 179)},
    {"whitesmoke", SVG_RGB(245, 245, 245)},
    {"yellowgreen", SVG_RGB(154, 205, 50)},
#endif
};
static gfx::vector_pixel svg_lookup_color(const char* str) {
    int i, ncolors = sizeof(svg_colors) / sizeof(svg_named_color);

    for (i = 0; i < ncolors; i++) {
        if (strcmp(svg_colors[i].name, str) == 0) {
            return svg_colors[i].color;
        }
    }

    return SVG_RGB(0, 0, 0);
}
result_t svg_parse_gradient_units(svg_context& ctx, const char** current, char* out_units) {
    char val[20];
    result_t res=svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) {
        return res;
    }
    res=svg_parse_alpha_str(ctx,current,val,20);
    if(!SUCCEEDED(res)) {
        return res;
    }
    *out_units = USER_SPACE;
    if(0==strcmp("objectBoundingBox",val)) {
        *out_units = OBJECT_SPACE;
    }
    return SUCCESS;
}
static result_t svg_parse_spread_method(svg_context& ctx,const char** current, spread_method* out_spread_method)
{
    char val[10];
    result_t res=svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) {
        return res;
    }
    res=svg_parse_alpha_str(ctx,current,val,20);
    if(!SUCCEEDED(res)) {
        return res;
    }
    *out_spread_method = spread_method::pad;
    if(0==strcmp("reflect",val)) {
        *out_spread_method = spread_method::reflect;
    } else if(0==strcmp("repeat",val)) {
        *out_spread_method = spread_method::repeat;
    } 
    return SUCCESS;
}

result_t svg_parse_color(svg_context& ctx, const char** current, int mode,
                         vector_pixel* out_color) {
    bool found = false;
    int state = mode==COLOR_MODE_UNKNOWN?0:5;
    unsigned int r, g, b;
    bool hsl = mode==COLOR_MODE_HSL?true:false;
    //char term = 0;
    float rgb[4];
    float* rc = rgb;
    ctx.tmp_value[0] = 0;
    char* name = ctx.tmp_value;
    unsigned int uvalue;
    result_t res;
    size_t len;
    while (true) {
        res = svg_ensure_current(ctx,current);
        if(!SUCCEEDED(res)) {
            return res;
        }
        if (!*current || !**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                if (state == 9) {
                    switch (rc - rgb) {
                        case 3:
                            if (hsl) {
                                *out_color = SVG_HSL(rgb[0], rgb[1], rgb[2]);
                            } else {
                                *out_color = SVG_RGB(rgb[0], rgb[1], rgb[2]);
                            }
                            break;
                        case 4:
                            if (hsl) {
                                *out_color = SVG_HSL(rgb[0], rgb[1], rgb[2]);
                                out_color->template channel<3>(rgb[3]);
                            } else {
                                *out_color =
                                    SVG_RGBA(rgb[0], rgb[1], rgb[2], rgb[3]);
                            }
                            break;
                        default:
                            return FMT_ERROR;
                    }
                    return SUCCESS;
                } 
                if(state==3) {
                    *out_color = svg_lookup_color(ctx.tmp_value);
                    return SUCCESS;
                }
                return IO_ERROR;
            }
            *current = ctx.rdr.value();
        } else {
            switch (state) {
                case 0:
                    found = false;
                    while (svg_isspace(**current)) {
                        found = true;
                        ++(*current);
                    }
                    if (!found && **current) {
                        state = 1;
                    }
                    break;
                case 1:
                    switch (**current) {
                        case '#':
                            ++(*current);
                            state = 2;
                            break;
                        case 'r':
                            *(name++) = **current;
                            *name = 0;
                            hsl = false;
                            ++(*current);
                            state = 3;
                            break;
                        case 'h':
                            *(name++) = **current;
                            *name = 0;
                            hsl = true;
                            ++(*current);
                            state = 3;
                            break;
                        default:
                            // color name
                            while (svg_isalpha(**current)) {
                                if (name - ctx.tmp_value < MAX_VALUE - 1) {
                                    *(name++) = **current;
                                    *name=0;
                                }
                                ++(*current);
                            }
                            res = svg_parse_alpha_str(
                                ctx, current, name,
                                MAX_VALUE - (name - ctx.tmp_value));
                            if (!SUCCEEDED(res)) {
                                return res;
                            }
                            *out_color = svg_lookup_color(ctx.tmp_value);
                            return SUCCESS;
                    }
                    break;
                case 2:
                    res = svg_parse_hex(ctx, current, &uvalue, &len);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    if (len > 3) {
                        r = (uvalue >> 16) & 0xFF;
                        g = (uvalue >> 8) & 0xFF;
                        b = uvalue & 0xFF;
                        *out_color = SVG_RGB(r, g, b);
                    } else {
                        *out_color = SVG_RGB(((uvalue >> 8) & 0x0F) * 17,
                                             ((uvalue >> 4) & 0x0F) * 17,
                                             (uvalue & 0x0F) * 17);
                    }
                    return SUCCESS;
                case 3:
                    if (svg_isalpha(**current)) {
                        if (name - ctx.tmp_value < MAX_VALUE - 1) {
                            *(name++) = **current;
                            *name=0;
                        }
                        ++(*current);
                    } else {
                        state = 4;
                    }
                    break;
                case 4:
                    if (**current == '(') {
                        ++(*current);
                        rc = rgb;
                        state = 5;
                        break;
                    }
                    // color name
                    res = svg_parse_alpha_str(
                        ctx, current, name, MAX_VALUE - (name - ctx.tmp_value));
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    if (0 == strcmp("currentcolor", name)) {
                        *out_color = svg_get_attr(ctx).color;
                    } else {
                        *out_color = svg_lookup_color(ctx.tmp_value);
                    }
                    return SUCCESS;
                case 5:
                    found = false;
                    while (svg_isspace(**current)) {
                        found = true;
                        ++(*current);
                        res=svg_ensure_current(ctx,current);
                        if(!SUCCEEDED(res)) return res;
                    }
                    if (!found && **current) {
                        state = 6;
                    }
                    break;
                case 6:
                    if (((size_t)(rc - rgb)) > (sizeof(rgb) / sizeof(float))) {
                        return FMT_ERROR;
                    }
                    res = svg_parse_float(ctx, current, true, rc);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    state = 7;
                    break;
                case 7:
                    if (**current == '%') {
                        ++(*current);
                        res=svg_ensure_current(ctx,current);
                        if(!SUCCEEDED(res)) return res;
                        *rc *= 2.55;
                    }
                    ++rc;
                    state = 8;
                    break;
                case 8:
                    found = false;
                    while (svg_isspace(**current)) {
                        found = true;
                        ++(*current);
                        res=svg_ensure_current(ctx,current);
                        if(!SUCCEEDED(res)) return res;
                    }
                    if (!found && **current) {
                        state = 9;
                    }
                    break;
                case 9:
                    if (**current != ')' && **current != ',') {
                        return FMT_ERROR;
                    }
                    found = **current == ')';
                    ++(*current);
                    res=svg_ensure_current(ctx,current);
                    if(!SUCCEEDED(res)) return res;
                    state = found ? 10 : 5;
                    break;
                case 10:
                    switch (rc - rgb) {
                        case 3:
                            if (hsl) {
                                *out_color = SVG_HSL(rgb[0], rgb[1], rgb[2]);
                            } else {
                                *out_color = SVG_RGB(rgb[0], rgb[1], rgb[2]);
                            }
                            return SUCCESS;
                        case 4:
                            if (hsl) {
                                *out_color = SVG_HSL(rgb[0], rgb[1], rgb[2]);
                                out_color->template channel<3>(rgb[3]);
                            } else {
                                *out_color =
                                    SVG_RGBA(rgb[0], rgb[1], rgb[2], rgb[3]);
                            }
                            return SUCCESS;

                        default:
                            return FMT_ERROR;
                    }
                    break;
            }
        }
    }

    return SUCCESS;
}

static result_t svg_parse_coordinate(svg_context& ctx, const char** current,
                                     svg_coord* out_coord) {
    float v;
    int state = 0;
    result_t result = svg_parse_float(ctx, current, false, &v);
    if (!SUCCEEDED(result)) {
        return result;
    }
    out_coord->value = v;
    if (*current == nullptr) {
        out_coord->units = UNITS_USER;
        return SUCCESS;
    }
    while (true) {
        if (**current == '\0') {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        } else {
            switch (state) {
                case 0:
                    switch (**current) {
                        case 'p':
                            state = 1;
                            ++(*current);
                            break;
                        case 'm':
                            state = 2;
                            ++(*current);
                            break;
                        case 'c':
                            state = 3;
                            ++(*current);
                            break;
                        case 'i':
                            state = 4;
                            ++(*current);
                            break;
                        case '%':
                            ++(*current);
                            out_coord->units = UNITS_PERCENT;
                            state = 6;
                            break;
                        case 'e':
                            state = 5;
                            ++(*current);
                            break;
                        default:
                            if (svg_isspace(**current) || '}' == **current ||
                                ';' == **current || ',' == **current) {
                                out_coord->units = UNITS_USER;
                                return SUCCESS;
                            }
                            return FMT_ERROR;
                    }
                    break;
                case 1:
                    switch (**current) {
                        case 'x':
                            ++(*current);
                            out_coord->units = UNITS_PX;
                            state = 6;
                            break;
                        case 't':
                            ++(*current);
                            out_coord->units = UNITS_PT;
                            state = 6;
                            break;
                        case 'c':
                            ++(*current);
                            out_coord->units = UNITS_PC;
                            state = 6;
                            break;
                        default:
                            return FMT_ERROR;
                    }
                    break;
                case 2:
                    switch (**current) {
                        case 'm':
                            ++(*current);
                            out_coord->units = UNITS_MM;
                            state = 6;
                            break;
                        default:
                            return FMT_ERROR;
                    }
                    break;
                case 3:
                    switch (**current) {
                        case 'm':
                            ++(*current);
                            out_coord->units = UNITS_CM;
                            state = 6;
                            break;
                        default:
                            return FMT_ERROR;
                    }
                    break;
                case 4:
                    switch (**current) {
                        case 'n':
                            ++(*current);
                            out_coord->units = UNITS_IN;
                            state = 6;
                            break;
                        default:
                            return FMT_ERROR;
                    }
                    break;
                case 5:
                    switch (**current) {
                        case 'm':
                            ++(*current);
                            out_coord->units = UNITS_EM;
                            state = 6;
                            break;
                        case 'x':
                            ++(*current);
                            out_coord->units = UNITS_EX;
                            state = 6;
                            break;
                    }
                    break;
                case 6:
                    if (svg_isspace(**current) || '}' == **current ||
                        ';' == **current || ',' == **current) {
                        return SUCCESS;
                    }
                    return FMT_ERROR;
                default:
                    return FMT_ERROR;
            }
        }
    }
    return SUCCESS;
}
static float svg_actual_orig_x(svg_context& ctx) { return ctx.view_box_min_x; }
static float svg_actual_orig_y(svg_context& ctx) { return ctx.view_box_min_y; }

static float svg_actual_width(svg_context& ctx) { return ctx.view_box_width>0?ctx.view_box_width:ctx.view_width; }

static float svg_actual_height(svg_context& ctx) { return ctx.view_box_height>0?ctx.view_box_height:ctx.view_height; }

static float svg_actual_length(svg_context& ctx) {
    float w = svg_actual_width(ctx), h = svg_actual_height(ctx);
    return sqrtf(w * w + h * h) /  math::sqrt2;
}
float svg_convert_to_pixels(svg_context& ctx, const svg_coord& coord,
                            float orig, float length) {
    svg_attrib& attr = svg_get_attr(ctx);
    switch (coord.units) {
        case UNITS_USER:
            return coord.value;
        case UNITS_PX:
            return coord.value;
        case UNITS_PT:
            return coord.value / 72.0f * ctx.dpi;
        case UNITS_PC:
            return coord.value / 6.0f * ctx.dpi;
        case UNITS_MM:
            return coord.value / 25.4f * ctx.dpi;
        case UNITS_CM:
            return coord.value / 2.54f * ctx.dpi;
        case UNITS_IN:
            return coord.value * ctx.dpi;
        case UNITS_EM:
            return coord.value * attr.fontSize;
        case UNITS_EX:
            return coord.value * attr.fontSize *
                   0.52f;  // x-height of Helvetica.
        case UNITS_PERCENT:
            return orig + coord.value / 100.0f * length;
        default:
            break;
    }
    return coord.value;
}
/*static result_t svg_parse_gradient_width(svg_context& ctx, const char** current, char units, float* out_value)
{
    svg_coord coord;
    result_t res = svg_parse_coordinate(ctx,current,&coord);
    if(!SUCCEEDED(res)) {
        return res;
    }
    if(units == USER_SPACE) {
        *out_value = svg_convert_to_pixels(ctx,coord,0,svg_actual_width(ctx));
    } else {
        if(coord.units == UNITS_PERCENT) {
           *out_value =  coord.value / 100.f;
        }
        *out_value = coord.value;
    }
    return SUCCESS;
}
static result_t svg_parse_gradient_height(svg_context& ctx, const char** current, char units, float* out_value)
{
    svg_coord coord;
    result_t res = svg_parse_coordinate(ctx,current,&coord);
    if(!SUCCEEDED(res)) {
        return res;
    }
    if(units == USER_SPACE) {
        *out_value = svg_convert_to_pixels(ctx,coord,0,svg_actual_height(ctx));
    } else {
        if(coord.units == UNITS_PERCENT) {
           *out_value =  coord.value / 100.f;
        }
        *out_value = coord.value;
    }
    return SUCCESS;
}

static result_t svg_parse_gradient_length(svg_context& ctx, const char** current, char units, float* out_value)
{
    svg_coord coord;
    result_t res = svg_parse_coordinate(ctx,current,&coord);
    if(!SUCCEEDED(res)) {
        return res;
    }
    if(units == USER_SPACE) {
        *out_value = svg_convert_to_pixels(ctx,coord,0,svg_actual_length(ctx));
    } else {
        if(coord.units == UNITS_PERCENT) {
           *out_value =  coord.value / 100.f;
        }
        *out_value = coord.value;
    }
    return SUCCESS;
}*/
static result_t svg_parse_transform(svg_context& ctx, const char** current,
                                    ::gfx::matrix* out_matrix) {
    // matrix
    // rotate
    // scale
    // skewX
    // skewY
    // translate
    result_t result = SUCCESS;
    int state = 0;
    int ttype = TRANS_ERROR;
    float args[6];
    size_t argsc = 0;
    float v;
    *out_matrix=matrix::create_identity();
    result = svg_skip_space(ctx, current);
    if (!SUCCEEDED(result)) {
        return result;
    }
    if (*current == nullptr) {
        return SUCCESS;
    }
    while (true) {
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (ctx.rdr.has_value()) {
                *current = ctx.rdr.value();
            } else {
                return SUCCESS;
            }
        }
        // halt on ;/} for CSS style
        if (**current == ';' || **current == '}' || !**current) {
            if (state == 0) {
                return SUCCESS;
            }
            return FMT_ERROR;
        }
        switch (state) {
            case -7:
                while (*current && **current && svg_isspace(**current))
                    ++(*current);
                if (!**current) {
                    if (!ctx.rdr.read()) {
                        return IO_ERROR;
                    }
                    if (!ctx.rdr.has_value()) {
                        if (ctx.rdr.node_type() ==
                            ml_node_type::attribute_end) {
                            if (!ctx.rdr.read()) {
                                return IO_ERROR;
                            }
                            *current = nullptr;
                            return SUCCESS;
                        }
                    } else {
                        *current = ctx.rdr.value();
                    }
                    break;
                }
                argsc = 0;
                state = 0;
                break;
            case -6:
                if (**current == ')') {
                    ++(*current);
                    switch (ttype) {
                        case TRANS_MATRIX:
                            if (argsc != 6) {
                                return FMT_ERROR;
                            }
                            *out_matrix =
                                ::gfx::matrix(args[0], args[1], args[2],
                                              args[3], args[4], args[5]) *
                                *out_matrix;
                            state = -7;
                            break;
                        case TRANS_ROTATE:
                            if (argsc == 1) {
                                *out_matrix = ::gfx::matrix::create_rotate(
                                                  SVG_DEG2RAD(args[0])) *
                                              *out_matrix;
                                state = -7;
                                break;
                            } else if (argsc == 3) {
                                *out_matrix = (::gfx::matrix::create_translate(
                                                   -args[1], -args[2]) *
                                               ::gfx::matrix::create_rotate(
                                                   SVG_DEG2RAD(args[0]))) *
                                              *out_matrix;
                                state = -7;
                                break;
                            }
                            return FMT_ERROR;
                        case TRANS_SCALE:
                            if (argsc == 1) {
                                *out_matrix =
                                    ::gfx::matrix::create_scale(args[0], args[1]) *
                                    *out_matrix;
                                state = -7;
                                break;
                            } else if (argsc == 2) {
                                *out_matrix =
                                    ::gfx::matrix::create_scale(args[0], args[1]) *
                                    *out_matrix;
                                state = -7;
                                break;
                            }
                            return FMT_ERROR;
                        case TRANS_SKEW_X:
                            if (argsc == 1) {
                                *out_matrix = ::gfx::matrix::create_skew_x(
                                                  SVG_DEG2RAD(args[0])) *
                                              *out_matrix;
                                state = -7;
                                break;
                            }
                            return FMT_ERROR;
                        case TRANS_SKEW_Y:
                            if (argsc == 1) {
                                *out_matrix = ::gfx::matrix::create_skew_y(
                                                  SVG_DEG2RAD(args[0])) *
                                              *out_matrix;
                                state = -7;
                                break;
                            }
                            return FMT_ERROR;
                        case TRANS_TRANSLATE:
                            if (argsc == 1) {
                                *out_matrix =
                                    ::gfx::matrix::create_translate(args[0], 0) *
                                    *out_matrix;
                                state = -7;
                                break;
                            } else if (argsc == 2) {
                                *out_matrix =
                                    ::gfx::matrix::create_translate(args[0], args[1]) *
                                    *out_matrix;
                                state = -7;
                                break;
                            }
                            return FMT_ERROR;
                        default:
                            return FMT_ERROR;
                    }
                } else {
                    if (**current == ',') {
                        ++(*current);
                    }
                    state = -3;
                }
                break;
            case -5:
                while (*current && **current && svg_isspace(**current))
                    ++(*current);
                state = -6;
                break;
            case -4:
                if (argsc > 5) {
                    return FMT_ERROR;
                }
                if (**current == '.' || **current == '+' || **current == '-' ||
                    svg_isdigit(**current)) {
                    result = svg_parse_float(ctx, current, true, &v);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    args[argsc++] = v;
                }
                state = -5;
                break;
                break;
            case -3:
                while (*current && **current && svg_isspace(**current))
                    ++(*current);
                state = -4;
                break;
            case -2:
                ++(*current);
                state = -3;
                break;
            case -1:
                if (**current != '(') {
                    ++(*current);
                } else {
                    state = -2;
                }
                break;
            case 0:
                switch (**current) {
                    case 'm':
                        ttype = TRANS_MATRIX;
                        state = -1;
                        break;
                    case 'r':
                        ttype = TRANS_ROTATE;
                        state = -1;
                        break;
                    case 's':
                        ++(*current);
                        state = 1;
                        break;
                    case 't':
                        ttype = TRANS_TRANSLATE;
                        state = -1;
                        break;
                    default:
                        return FMT_ERROR;
                }
                break;
            case 1:
                switch (**current) {
                    case 'c':
                        ttype = TRANS_SCALE;
                        state = -1;
                        break;
                    case 'k':
                        ++(*current);
                        state = 2;
                        ++(*current);
                        break;
                    default:
                        return FMT_ERROR;
                }
                break;
            case 2:
                switch (**current) {
                    case 'X':
                        ttype = TRANS_SKEW_X;
                        state = -1;
                        break;
                    case 'Y':
                        ttype = TRANS_SKEW_Y;
                        state = -1;
                        break;
                    case 'e':
                    case 'w':
                        ++(*current);
                        break;
                    default:
                        return FMT_ERROR;
                }
                break;
            default:
                return FMT_ERROR;
        }
        if (**current == '\0') {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                break;
            }
            *current = ctx.rdr.value();
        }
    }

    return SUCCESS;
}
result_t svg_parse_view_box_value(svg_context& ctx, const char** current,
                                  float* out_value) {
    result_t res;
    res = svg_skip_space(ctx, current);
    if (!SUCCEEDED(res)) return res;
    res = svg_parse_float(ctx, current, true, out_value);
    if (!SUCCEEDED(res)) return res;
    res = svg_ensure_current(ctx, current);
    if (!SUCCEEDED(res)) return res;
    if (*current == nullptr) return SUCCESS;
    if (**current == '%') {
        ++(*current);
        res = svg_ensure_current(ctx, current);
        if (!SUCCEEDED(res)) return res;
    }
    return SUCCESS;
}
result_t svg_parse_view_box(svg_context& ctx, const char** current) {
    result_t res = svg_parse_view_box_value(ctx, current, &ctx.view_box_min_x);
    if (!SUCCEEDED(res)) return res;
    res = svg_parse_view_box_value(ctx, current, &ctx.view_box_min_y);
    if (!SUCCEEDED(res)) return res;
    res = svg_parse_view_box_value(ctx, current, &ctx.view_box_width);
    if (!SUCCEEDED(res)) return res;
    return svg_parse_view_box_value(ctx, current, &ctx.view_box_height);
}
result_t svg_parse_stroke_dash_array(svg_context& ctx, const char** current,
                                     size_t* out_values_size,
                                     float* out_values) {
    result_t result = SUCCESS;
    *out_values_size = 0;
    bool first = true;
    while (true) {
        result = svg_skip_space(ctx, current);
        if (!SUCCEEDED(result)) {
            return result;
        }
        if (*current == nullptr) {
            return SUCCESS;
        }
        if (!**current) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (ctx.rdr.has_value()) {
                *current = ctx.rdr.value();
            } else {
                return SUCCESS;
            }
        }
        if (first) {
            first = false;
            if (*current && **current == 'n') {  // none
                return svg_parse_alpha_str(ctx, current, ctx.tmp_value,
                                           MAX_VALUE);
            }
        }
        // halt on ;/} for CSS style
        if (**current == ';' || **current == '}' || !**current) {
            return SUCCESS;
        }
        svg_coord coord;
        result = svg_parse_coordinate(ctx, current, &coord);
        if (!SUCCEEDED(result)) {
            return result;
        }
        if (*out_values_size < MAX_STROKE_ARRAY) {
            *(out_values)++ = fabsf(svg_convert_to_pixels(
                ctx, coord, 0.0f, svg_actual_length(ctx)));
            ++(*out_values_size);
        }
        result = svg_skip_space(ctx, current);
        if (!SUCCEEDED(result)) {
            return result;
        }
        if (*current && (**current == ',' || svg_isspace(**current))) {
            ++(*current);
            if (**current == '\0') {
                if (!ctx.rdr.read()) {
                    return IO_ERROR;
                }
                if (!ctx.rdr.has_value()) {
                    *current = nullptr;
                    break;
                }
                *current = ctx.rdr.value();
            }
        }
    }

    return SUCCESS;
}
result_t svg_parse_path_points(svg_context& ctx, const char** current,
                          size_t values_size, float* out_values) {
    result_t result;
    for (size_t i = 0; i < values_size; ++i) {
        result = svg_skip_space_or_comma(ctx, current);
        if (!SUCCEEDED(result)) {
            return result;
        }
        result = svg_parse_float(ctx, current, true, &out_values[i]);
        if (!SUCCEEDED(result)) {
            return result;
        }
        result = svg_skip_space_or_comma(ctx, current);
        if (!SUCCEEDED(result)) {
            return result;
        }
    }
    return SUCCESS;
}
void svg_make_relative_points(float cur_x, float cur_y, float* values,
                              size_t values_size) {
    for (size_t i = 0; i < values_size; i += 2) {
        values[i] += cur_x;
        values[i + 1] += cur_y;
    }
}
void svg_make_relative_point(float rel, float* in_out_value) {
    *in_out_value +=rel;
}

result_t svg_parse_path(svg_context& ctx, canvas_path& path) {
    
    float values[6];
    bool flags[2];
    float start_x = 0;
    float start_y = 0;
    float last_ctrl_x = 0, last_ctrl_y = 0;
    float cur_x = 0, cur_y = 0;
    const char* sz = ctx.rdr.value();
    char last_cmd = 0;
    char cmd = 0;
    result_t result = SUCCESS;
    bool first = true;
    while (true) {
        if (sz && *sz) {
            result = svg_skip_space(ctx, &sz);
            if (!SUCCEEDED(result)) {
                return result;
            }
        }
        if (sz == nullptr) {
            break;
        }
        if (!*sz) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                sz = nullptr;
                break;
            }
            sz = ctx.rdr.value();
        } else {
            if (first) {
                // M must be the first cmd
                first = false;
                if (*sz != 'M' && *sz != 'm') {
                    return FMT_ERROR;
                }
            }
            if(svg_isalpha(*sz)) {
                cmd = *(sz++);
            }
            switch (cmd) {
                case 'M':
                case 'm':
                    result = svg_parse_path_points(ctx, &sz, 2, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_points(cur_x, cur_y, values, 2);
                    }
#ifdef SVG_DUMP_PATH
                    printf("move to (%0.2f, %0.2f)\n", values[0], values[1]);
#endif
                    result = path.move_to({values[0], values[1]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_x = start_x = values[0];
                    cur_y = start_y = values[1];
                    cmd = cmd > 'Z' ? 'l' : 'L';
                    break;
                case 'H':
                case 'h':
                    result = svg_parse_path_points(ctx, &sz, 1, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_point(cur_x,values);
                    }
#ifdef SVG_DUMP_PATH
                    printf("hline to (%0.2f)\n", values[0]);
#endif
                    result = path.line_to({values[0], cur_y});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_x = values[0];
                    break;
                case 'V':
                case 'v':
                    result = svg_parse_path_points(ctx, &sz, 1, values + 1);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_point(cur_y,values+1);
                    }
#ifdef SVG_DUMP_PATH
                    printf("vline to (%0.2f)\n", values[1]);
#endif
                    result = path.line_to({cur_x, values[1]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_y = values[1];
                    break;
                case 'L':
                case 'l':
                    result = svg_parse_path_points(ctx, &sz, 2, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_points(cur_x, cur_y, values, 2);
                    }
#ifdef SVG_DUMP_PATH
                    printf("line to (%0.2f, %0.2f)\n", values[0], values[1]);
#endif
                    result = path.line_to({values[0], values[1]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_x = values[0];
                    cur_y = values[1];
                    break;
                case 'Q':
                case 'q':
                    result = svg_parse_path_points(ctx, &sz, 4, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_points(cur_x, cur_y, values, 4);
                    }
#ifdef SVG_DUMP_PATH
                    printf("quad to (%0.2f, %0.2f) (%0.2f, %0.2f)\n", values[0],
                           values[1], values[2], values[3]);
#endif
                    result = path.quad_to({values[0], values[1]},
                                          {values[2], values[3]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    last_ctrl_x = values[0];
                    last_ctrl_y = values[1];
                    cur_x = values[2];
                    cur_y = values[3];
                    break;
                case 'T':
                case 't':
                    if (last_cmd != 'Q' && last_cmd != 'q' && last_cmd != 'T' &&
                        last_cmd != 't') {
                        values[0] = cur_x;
                        values[1] = cur_y;
                    } else {
                        values[0] = 2 * cur_x - last_ctrl_x;
                        values[1] = 2 * cur_y - last_ctrl_y;
                    }
                    result = svg_parse_path_points(ctx, &sz, 2, values + 2);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {
                        svg_make_relative_points(cur_x, cur_y, values + 2, 2);
                    }
#ifdef SVG_DUMP_PATH
                    printf("T quad to (%0.2f, %0.2f)\n", values[2], values[3]);
#endif
                    result = path.quad_to({values[0], values[1]},
                                          {values[2], values[3]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    last_ctrl_x = values[0];
                    last_ctrl_y = values[1];
                    cur_x = values[2];
                    cur_y = values[3];
                    break;
                case 'S':
                case 's':
                    if (last_cmd != 'C' && last_cmd != 'c' && last_cmd != 'S' &&
                        last_cmd != 's') {
                        values[0] = cur_x;
                        values[1] = cur_y;
                    } else {
                        values[0] = 2 * cur_x - last_ctrl_x;
                        values[1] = 2 * cur_y - last_ctrl_y;
                    }

                    result = svg_parse_path_points(ctx, &sz, 4, values + 2);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {
                        svg_make_relative_points(cur_x, cur_y, values + 2, 4);
                    }
#ifdef SVG_DUMP_PATH
                    printf("S cubic to (%0.2f, %0.2f) (%0.2f, %0.2f)\n",
                           values[2], values[3], values[4], values[5]);
#endif
                    result = path.cubic_to({values[0], values[1]},
                                           {values[2], values[3]},
                                           {values[4], values[5]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    last_ctrl_x = values[2];
                    last_ctrl_y = values[3];
                    cur_x = values[4];
                    cur_y = values[5];
                    break;
                case 'C':
                case 'c':
                    result = svg_parse_path_points(ctx, &sz, 5, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    result = svg_parse_path_points(ctx, &sz, 1, values+5);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {  // if lowercase (copy/pastable)
                        svg_make_relative_points(cur_x, cur_y, values, 6);
                    }
#ifdef SVG_DUMP_PATH
                    printf(
                        "cubic to (%0.2f, %0.2f) (%0.2f, %0.2f) (%0.2f, "
                        "%0.2f)\n",
                        values[0], values[1], values[2], values[3], values[4],
                        values[5]);
#endif
                    result = path.cubic_to({values[0], values[1]},
                                           {values[2], values[3]},
                                           {values[4], values[5]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    last_ctrl_x = values[2];
                    last_ctrl_y = values[3];
                    cur_x = values[4];
                    cur_y = values[5];
                    break;
                case 'A':
                case 'a':
                    result = svg_parse_path_points(ctx, &sz, 3, values);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    result = svg_parse_path_flag(ctx, &sz, &flags[0]);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    result = svg_parse_path_flag(ctx, &sz, &flags[1]);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    result = svg_parse_path_points(ctx, &sz, 2, values + 3);
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    if (cmd > 'Z') {
                        svg_make_relative_point(cur_x,&values[3]);
                        svg_make_relative_point(cur_y,&values[4]);
                    }
#ifdef SVG_DUMP_PATH
                    printf("arc to (%0.2f, %0.2f) (%0.2f)%s%s (%0.2f, %0.2f)\n",
                           values[0], values[1], values[2],
                           flags[0] ? " large_arc" : "",
                           flags[1] ? " sweep" : "", values[3], values[4]);
#endif
                    result = path.arc_to({values[0], values[1]},
                                         SVG_DEG2RAD(values[2]), flags[0],
                                         flags[1], {values[3], values[4]});
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_x = values[3];
                    cur_y = values[4];
                    break;
                case 'Z':
                case 'z':
#ifdef SVG_DUMP_PATH
                    puts("close path");
#endif
                    result = path.close();
                    if (!SUCCEEDED(result)) {
                        return result;
                    }
                    cur_x = start_x;
                    cur_y = start_y;
                    break;
                default:  // unhandled
#ifdef SVG_DUMP_PATH
                    puts("unhandled");
#endif
                    break;
            }
            last_cmd = cmd;
            
        }
    }

    return SUCCESS;
}
static result_t svg_parse_poly_points(svg_context& ctx, const char** current,canvas_path& path) {
    float values[2];
    result_t res = svg_parse_path_points(ctx,current,2,values);
    if(!SUCCEEDED(res)) {
        return res;
    }
    res = path.move_to({values[0],values[1]});
    if(!SUCCEEDED(res)) {
        return res;
    }
    while(*current && **current && **current!='}' && **current!=';') {
        res = svg_parse_path_points(ctx,current,2,values);
        if(!SUCCEEDED(res)) {
            return res;
        }
        res=path.line_to({values[0],values[1]});
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    return SUCCESS;
}
static result_t svg_parse_paint(svg_context& ctx, const char** current,
                                char* out_visibility, vector_pixel* out_color,
                                char* out_id, size_t max_id) {
    *out_visibility = 0;
    *out_id = 0;
    *out_color = SVG_RGB(0, 0, 0);
    *out_id = '\0';
    result_t res = svg_ensure_current(ctx, current);
    if (!SUCCEEDED(res)) return res;
    if ('#' == **current) {
        res = svg_parse_color(ctx, current, COLOR_MODE_UNKNOWN, out_color);
        if (!SUCCEEDED(res)) return res;
        *out_visibility = 1;
        return SUCCESS;
    } 
    
    res = svg_parse_alpha_str(ctx, current, ctx.tmp_value, MAX_VALUE);
    if (!SUCCEEDED(res)) {
        return res;
    }
    char* sz = ctx.tmp_value;
    *out_visibility = 0;
    bool isid = false;
    if (0 == strcmp(ctx.tmp_value, "none")) {
        *out_visibility = 0;
        *sz = 0;
        return SUCCESS;
    } else if (0 == strcmp(ctx.tmp_value, "url") && (**current == '(')) {
        ++(*current);
        res = svg_ensure_current(ctx,current);
        if(!SUCCEEDED(res)) {
            return res;
        }
        if (**current == '#') {
            isid = true;
            ++(*current);
        } else if (**current == 0) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            *current = ctx.rdr.value();
            if (**current == '#') {
                ++(*current);
                isid = true;
            }
        }
    } else if (0 == strcmp(ctx.tmp_value, "rgb") && (**current == '(')) {
        ++(*current);
        res = svg_parse_color(ctx,current,COLOR_MODE_RGB,out_color);
        if(!SUCCEEDED(res)) {
            return res;
        }
    } else if (0 == strcmp(ctx.tmp_value, "hsl") && (**current == '(')) {
        ++(*current);
        res = svg_parse_color(ctx,current,COLOR_MODE_HSL,out_color);
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    if (!isid) {
        *out_visibility = 1;
        if(*ctx.tmp_value) {
            *out_color = svg_lookup_color(ctx.tmp_value);
        }
        return SUCCESS;
        
    } else {
        res = svg_parse_alphanum_str(ctx,current,out_id,MAX_ID-1);
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    if (**current == ')') {
        ++(*current);
        if (**current == 0) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            *current = ctx.rdr.has_value() ? ctx.rdr.value() : nullptr;
        }
    }
    res = svg_ensure_current(ctx, current);
    if (!SUCCEEDED(res)) return res;
    if(*current!=nullptr && **current) {
        res=svg_skip_space(ctx,current);
        if(!SUCCEEDED(res)) {
            return res;
        }
        res = svg_parse_color(ctx, current, COLOR_MODE_UNKNOWN, out_color);
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    *out_visibility = 2;
    return SUCCESS;
}
static result_t svg_parse_display(svg_context& ctx, const char** current,
                                  bool* none) {
    result_t res = svg_parse_alpha_str(ctx, current, ctx.tmp_value, MAX_VALUE);
    if (!SUCCEEDED(res)) {
        return res;
    }
    *none = false;
    if (0 == strcmp("none", ctx.tmp_value)) {
        *none = true;
    }
    return SUCCESS;
}
static result_t svg_parse_fill_rule(svg_context& ctx, const char** current,
                                    ::gfx::fill_rule* out_rule) {
    result_t res = svg_parse_alpha_str(ctx, current, ctx.tmp_value, MAX_VALUE);
    if (!SUCCEEDED(res)) {
        return res;
    }
    *out_rule = ::gfx::fill_rule::non_zero;
    if (0 == strcmp("evenodd", ctx.tmp_value)) {
        *out_rule = ::gfx::fill_rule::even_odd;
    }
    return SUCCESS;
}
static result_t svg_parse_linecap(svg_context& ctx, const char** current,
                                  ::gfx::line_cap* out_linecap) {
    result_t res = svg_parse_alpha_str(ctx, current, ctx.tmp_value, MAX_VALUE);
    if (!SUCCEEDED(res)) {
        return res;
    }
    *out_linecap = ::gfx::line_cap::butt;
    if (0 == strcmp("round", ctx.tmp_value)) {
        *out_linecap = ::gfx::line_cap::round;
    }
    if (0 == strcmp("square", ctx.tmp_value)) {
        *out_linecap = ::gfx::line_cap::square;
    }
    return SUCCESS;
}
static result_t svg_parse_linejoin(svg_context& ctx, const char** current,
                                   ::gfx::line_join* out_linejoin) {
    result_t res = svg_parse_alpha_str(ctx, current, ctx.tmp_value, MAX_VALUE);
    if (!SUCCEEDED(res)) {
        return res;
    }
    *out_linejoin = ::gfx::line_join::miter;
    if (0 == strcmp("round", ctx.tmp_value)) {
        *out_linejoin = ::gfx::line_join::round;
    }
    if (0 == strcmp("bevel", ctx.tmp_value)) {
        *out_linejoin = ::gfx::line_join::bevel;
    }
    return SUCCESS;
}

static result_t svg_parse_style(svg_context& ctx, const char** current,bool is_css) {
    result_t res = SUCCESS;
    int state = 0;
    int id = 0;
    while (*current && **current!='}') {
        if (!*current || !**current) {
            if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
                *current = nullptr;
                return SUCCESS;
            }
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
            if (!ctx.rdr.has_value()) {
                *current = nullptr;
                return SUCCESS;
            }
            *current = ctx.rdr.value();
        } else {
            switch (state) {
                case 0:
                    res = svg_parse_name_str(ctx, current, ctx.tmp_name,
                                             MAX_NAME);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    id = svg_attributeid(ctx.tmp_name);
                    state = 1;
                    break;
                case 1:
                    res = svg_skip_space(ctx, current);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    state = 2;
                    break;
                case 2:
                    if (**current == ':') {
                        ++(*current);
                        state = 3;
                        break;
                    }
                    return FMT_ERROR;
                case 3:
                    res = svg_skip_space(ctx, current);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    state = 4;
                    break;
                case 4:
                    res = svg_parse_attribute_id(ctx, current, id,is_css);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    state = 5;
                    break;
                case 5:
                    res = svg_skip_css_val(ctx, current);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    if(*current && **current==';') {
                        ++(*current);
                    }
                    state = 6;
                    break;
                case 6:
                    res = svg_skip_space(ctx, current);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                    state = 0;
                    break;
            }
        }
    }
    return res;
}

static result_t svg_parse_attribute_id(svg_context& ctx, const char** current,
                                       int id, bool is_css) {
    svg_attrib& attr = svg_get_attr(ctx);
    bool none_flag = false;
    //char vis_flag = 0;
    svg_coord coord;
    ::gfx::matrix m=attr.xform;
    result_t r;
    switch (id) {
        case ATTR_COLOR:
            r = svg_parse_color(ctx, current,COLOR_MODE_UNKNOWN, &attr.color);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.color = 1;
            break;
        case ATTR_DISPLAY:
            r = svg_parse_display(ctx, current, &none_flag);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if (none_flag) {
                attr.visible = 0;
            }
            if(is_css) ctx.css_has.attr.visible = 1;
            break;
        case ATTR_ID:
            r = svg_parse_name_str(ctx, current, attr.id, MAX_ID);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.id = 1;
            if(ctx.css_current==nullptr) {
                ctx.css_current = svg_find_css_data(ctx,attr.id,true);
            }
            
            break;
        case ATTR_FILL:
            r = svg_parse_paint(ctx, current, &attr.hasFill, &attr.fillColor,
                                attr.fillGradient, MAX_ID);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) {
                ctx.css_has.attr.hasFill = 1;
                ctx.css_has.attr.fillColor = 1;
                ctx.css_has.attr.fillGradient = 1;
            }
            break;
        case ATTR_FILL_OPACITY:
            r = svg_parse_float(ctx, current, true, &attr.fillOpacity);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.fillOpacity = 1;
            break;
        case ATTR_FILL_RULE:
            r = svg_parse_fill_rule(ctx, current, &attr.fillRule);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.fillRule = 1;
            break;
        case ATTR_OPACITY:
            r = svg_parse_float(ctx, current, true, &attr.opacity);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.opacity = 1;
            break;
            
        case ATTR_STOP_COLOR:
            r = svg_parse_color(ctx, current, COLOR_MODE_UNKNOWN,&attr.stopColor);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.stopColor = 1;
            break;
        case ATTR_STOP_OPACITY:
            r = svg_parse_float(ctx, current, true, &attr.stopOpacity);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.stopOpacity = 1;
            break;
        case ATTR_STROKE:
            r = svg_parse_paint(ctx, current, &attr.hasStroke,
                                &attr.strokeColor, attr.strokeGradient, MAX_ID);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) {
                ctx.css_has.attr.hasStroke = 1;
                ctx.css_has.attr.strokeColor = 1;
                ctx.css_has.attr.strokeGradient = 1;
            }
            break;
        case ATTR_STROKE_WIDTH:
            r = svg_parse_coordinate(ctx, current, &coord);
            if (!SUCCEEDED(r)) {
                return r;
            }
            attr.strokeWidth =
                svg_convert_to_pixels(ctx, coord, 0.0f, svg_actual_length(ctx));
            if(is_css) ctx.css_has.attr.strokeWidth = 1;
            break;
        case ATTR_STROKE_DASHARRAY:
            r = svg_parse_stroke_dash_array(ctx, current, &attr.strokeDashCount,
                                            attr.strokeDashArray);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.strokeDashArray = 1;
            break;
        case ATTR_STROKE_DASHOFFSET:
            r = svg_parse_coordinate(ctx, current, &coord);
            if (!SUCCEEDED(r)) {
                return r;
            }
            attr.strokeDashOffset =
                svg_convert_to_pixels(ctx, coord, 0.0f, svg_actual_length(ctx));
            if(is_css) ctx.css_has.attr.strokeDashOffset = 1;
            break;
        case ATTR_STROKE_LINECAP:
            r = svg_parse_linecap(ctx, current, &attr.strokeLineCap);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.strokeLineCap = 1;
            break;
        case ATTR_STROKE_LINEJOIN:
            r = svg_parse_linejoin(ctx, current, &attr.strokeLineJoin);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.strokeLineJoin = 1;
            break;
        case ATTR_STROKE_MITERLIMIT:
            r = svg_parse_float(ctx, current, true, &attr.miterLimit);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.miterLimit = 1;
            break;
        case ATTR_OFFSET:
            r = svg_parse_coordinate(ctx, current, &attr.stopOffset);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.stopOffset = 1;
            break;
        case ATTR_STROKE_OPACITY:
            r = svg_parse_float(ctx, current, true, &attr.strokeOpacity);
            if (!SUCCEEDED(r)) {
                return r;
            }
            if(is_css) ctx.css_has.attr.strokeOpacity = 1;
            break;
        case ATTR_TRANSFORM:
            r = svg_parse_transform(ctx, current, &m);
            if (!SUCCEEDED(r)) {
                return r;
            }
            attr.xform = m*attr.xform;
            break;
        case ATTR_FONT_SIZE:
            r = svg_parse_coordinate(ctx, current, &coord);
            if (!SUCCEEDED(r)) {
                return r;
            }
            attr.fontSize =
                svg_convert_to_pixels(ctx, coord, 0.0f, svg_actual_length(ctx));
            if(is_css) ctx.css_has.attr.fontSize = 1;
            break;
        case ATTR_STYLE:
            r = svg_parse_style(ctx, current,false);
            if (!SUCCEEDED(r)) {
                return r;
            }
            break;
        case ATTR_CLASS:
            r=svg_parse_alphanum_str(ctx,current,ctx.tmp_name,MAX_NAME);
            if (!SUCCEEDED(r)) {
                return r;
            }
            ctx.css_current = svg_find_css_data(ctx,ctx.tmp_name,false);
            break;
        default:
            r = svg_skip_to(ctx, current, ';', false);
            if (!SUCCEEDED(r)) {
                return r;
            }
            break;
    }

    return SUCCESS;
}
static result_t svg_parse_attribute(svg_context& ctx) {
    int id = svg_attributeid(ctx.rdr.value());
    if (!ctx.rdr.read()) {
        return IO_ERROR;
    }
    const char* sz = ctx.rdr.value();
    return svg_parse_attribute_id(ctx, &sz, id,false);
}
static result_t svg_parse_attributes(svg_context& ctx) {
    result_t res = SUCCESS;
    while ((ctx.rdr.node_type() == ml_node_type::attribute || ctx.rdr.read()) &&
           ctx.rdr.node_type() == ml_node_type::attribute) {
        res = svg_parse_attribute(ctx);
        if (!SUCCEEDED(res)) {
            break;
        }
    }
    return res;
}
static svg_grad_data* svg_find_gradient(svg_context& ctx, const char* id) {
    svg_grad_data* gd = ctx.grad_head;
    
    while(gd!=nullptr) {
        if(0==strcmp(gd->id,id)) {
            return gd;
        }
        gd=gd->next;
    }
    return nullptr;
    
}
static inline float svg_convert_length(const svg_coord& length, float maximum)
{
    if(length.units == UNITS_PERCENT)
        return length.value * maximum / 100.f;
    return length.value;
}

static float svg_resolve_length(svg_context& ctx, const svg_coord& length, char mode)
{
    float maximum = 0.f;
    if(length.units == UNITS_PERCENT) {
        if(mode == 'x') {
            maximum = ctx.view_width;
        } else if(mode == 'y') {
            maximum = ctx.view_height;
        } else if(mode == 'o') {
            maximum = hypotf(ctx.view_width, ctx.view_height) / math::sqrt2;
        }
    }

    return svg_convert_length(length, maximum);
}

static float svg_resolve_gradient_length(svg_context& ctx, const svg_coord& length, int units, char mode)
{
    if(units == UNITS_USER)
        return svg_resolve_length(ctx, length, mode);
    return svg_convert_length(length, 1.f);
}

static result_t svg_build_gradient(svg_context& ctx, const rectf& local_bounds, const matrix& transform, svg_grad_data* data, gradient* out_grad) {
    out_grad->type = data->type;
    out_grad->stops_size = 0;
    svg_grad_data* ref_data = nullptr;
    if(data->ref[0]!=0) {
        ref_data = svg_find_gradient(ctx,data->ref);
        if(ref_data!=nullptr) {
            //sl = sqrtf(sw * sw + sh * sh) / math::sqrt2;
            if(ref_data->type==gradient_type::linear) {
                out_grad->linear.x1 = svg_resolve_gradient_length(ctx,ref_data->linear.x1,ref_data->units,'x');
                out_grad->linear.y1 = svg_resolve_gradient_length(ctx,ref_data->linear.y1,ref_data->units,'y');
                out_grad->linear.x2 = svg_resolve_gradient_length(ctx,ref_data->linear.x2,ref_data->units,'x');
                out_grad->linear.y2 = svg_resolve_gradient_length(ctx,ref_data->linear.y2,ref_data->units,'y');
            } else {
                out_grad->radial.cx = svg_resolve_gradient_length(ctx, ref_data->radial.cx,ref_data->units, 'x');
                out_grad->radial.cy = svg_resolve_gradient_length(ctx, ref_data->radial.cy,ref_data->units, 'y');
                out_grad->radial.fx = svg_resolve_gradient_length(ctx, ref_data->radial.fx,ref_data->units, 'x');
                out_grad->radial.fy = svg_resolve_gradient_length(ctx, ref_data->radial.fy,ref_data->units, 'y');
                out_grad->radial.cr = svg_resolve_gradient_length(ctx, ref_data->radial.cr,ref_data->units, 'o');
                out_grad->radial.fr = svg_resolve_gradient_length(ctx, ref_data->radial.fr,ref_data->units, 'o');
            }    
            out_grad->spread = ref_data->spread;
            out_grad->transform = ref_data->transform;
            if(ref_data->stops_size>0) {
                size_t sz = sizeof(gradient_stop)*ref_data->stops_size;
                out_grad->stops = (gradient_stop*)ctx.allocator(sz);
                if(out_grad->stops==nullptr) {
                    return OUT_OF_MEMORY;
                }
                for(size_t i = 0;i<ref_data->stops_size;++i) {
                    out_grad->stops[i].color = ref_data->stops[i].color;
                    float alpha = out_grad->stops[i].color.opacity();
                    float opacity = ref_data->stops[i].opacity;
                    out_grad->stops[i].color.opacity(alpha*opacity);
                    out_grad->stops[i].offset = svg_resolve_gradient_length(ctx, ref_data->stops[i].offset,ref_data->units, 'o');
                }
                out_grad->stops_size = ref_data->stops_size;
            }
        }
    }

    out_grad->transform = data->transform;
    if(data->units == OBJECT_SPACE) {
        matrix m= matrix::create_translate(local_bounds.x1,local_bounds.y1);
        m.scale_inplace(local_bounds.width(),local_bounds.height());
        out_grad->transform = out_grad->transform * m;
    }
    
    out_grad->spread = data->spread;
    //sl = sqrtf(sw * sw + sh * sh) / math::sqrt2;
    if(data->type==gradient_type::linear) {
        out_grad->linear.x1 = svg_resolve_gradient_length(ctx,data->linear.x1,data->units,'x');
        out_grad->linear.y1 = svg_resolve_gradient_length(ctx,data->linear.y1,data->units,'y');
        out_grad->linear.x2 = svg_resolve_gradient_length(ctx,data->linear.x2,data->units,'x');
        out_grad->linear.y2 = svg_resolve_gradient_length(ctx,data->linear.y2,data->units,'y');
    } else {
        out_grad->radial.cx = svg_resolve_gradient_length(ctx, data->radial.cx,data->units, 'x');
        out_grad->radial.cy = svg_resolve_gradient_length(ctx, data->radial.cy,data->units, 'y');
        out_grad->radial.fx = svg_resolve_gradient_length(ctx, data->radial.fx,data->units, 'x');
        out_grad->radial.fy = svg_resolve_gradient_length(ctx, data->radial.fy,data->units, 'y');
        out_grad->radial.cr = svg_resolve_gradient_length(ctx, data->radial.cr,data->units, 'o');
        out_grad->radial.fr = svg_resolve_gradient_length(ctx, data->radial.fr,data->units, 'o');
    }
    if(data->stops_size>0) {
        size_t sz = sizeof(gradient_stop)*(data->stops_size+out_grad->stops_size);
        out_grad->stops = out_grad->stops_size>0?(gradient_stop*)ctx.reallocator(out_grad->stops,sz):(gradient_stop*)ctx.allocator(sz);
        if(out_grad->stops==nullptr) {
            return OUT_OF_MEMORY;
        }
        //int st = ref_data!=nullptr?ref_data->stops_size:0;
        for(size_t i = 0;i<data->stops_size;++i) {
            size_t j = out_grad->stops_size+i;
            out_grad->stops[j].color = data->stops[i].color;
            float alpha = out_grad->stops[i].color.opacity();
            float opacity = data->stops[i].opacity;
            out_grad->stops[j].color.opacity(alpha*opacity);
            out_grad->stops[j].offset = svg_resolve_gradient_length(ctx, data->stops[i].offset,data->units, 'o');
        }
        out_grad->stops_size += data->stops_size;
    }
    return SUCCESS;
}
static void svg_apply_css(svg_attrib* attr,svg_css_data* css ) {
    if(attr==nullptr||css==nullptr) {return;}
    if(css->has.color) {
        attr->color = css->data.color;
    }
    if(css->has.fillColor) {
        attr->fillColor = css->data.fillColor;
    }
    if(css->has.fillGradient) {
        strcpy(attr->fillGradient,css->data.fillGradient);
    }
    if(css->has.fillOpacity) {
        attr->fillOpacity = css->data.fillOpacity;
    }
    if(css->has.fillRule) {
        attr->fillRule = css->data.fillRule;
    }
    if(css->has.fontSize) {
        attr->fontSize= css->data.fontSize;
    }
    if(css->has.hasFill) {
        attr->hasFill= css->data.hasFill;
    }
    if(css->has.hasStroke) {
        attr->hasStroke= css->data.hasStroke;
    }
    //if(css->has.id) {
    //    strcpy(attr->id,css->data.id);
    //}
    if(css->has.miterLimit) {
        attr->miterLimit= css->data.miterLimit;
    }
    if(css->has.opacity) {
        attr->opacity = css->data.opacity;
    }
    if(css->has.stopColor) {
        attr->stopColor= css->data.stopColor;
    }
    if(css->has.stopOffset) {
        attr->stopOffset= css->data.stopOffset;
    }
    if(css->has.stopOpacity) {
        attr->stopOpacity= css->data.stopOpacity;
    }
    if(css->has.strokeColor) {
        attr->strokeColor= css->data.strokeColor;
    }
    if(css->has.strokeDashArray) {
        memcpy(attr->strokeDashArray, css->data.strokeDashArray,sizeof(float)*css->data.strokeDashCount);
        attr->strokeDashCount = css->data.strokeDashCount;
    }
    if(css->has.strokeDashOffset) {
        attr->strokeDashOffset = css->data.strokeDashOffset;
    }
    if(css->has.strokeGradient) {
        strcpy(attr->strokeGradient,css->data.strokeGradient);
    }
    if(css->has.strokeLineCap) {
        attr->strokeLineCap=css->data.strokeLineCap;
    }
    if(css->has.strokeLineJoin) {
        attr->strokeLineJoin=css->data.strokeLineJoin;
    }
    if(css->has.strokeOpacity) {
        attr->strokeOpacity=css->data.strokeOpacity;
    }
    if(css->has.strokeWidth) {
        attr->strokeWidth=css->data.strokeWidth;
    }
    if(css->has.visible) {
        attr->visible=css->data.visible;
    }
    if(css->has.xform) {
        attr->xform=css->data.xform;
    }
}
static result_t svg_apply_attribute(svg_context& ctx, const rectf& local_bounds, canvas_style* out_style) {
    result_t res = SUCCESS;
    svg_attrib& a = svg_get_attr(ctx);
    svg_apply_css(&a,ctx.css_current);
    out_style->fill_color =  a.fillColor;
    out_style->fill_gradient.stops = nullptr;
    out_style->fill_gradient.stops_size = 0;
    out_style->stroke_gradient.stops = nullptr;
    out_style->stroke_gradient.stops_size = 0;
    
    if(a.hasFill==2 && a.fillGradient[0]!=0) {
        svg_grad_data* gd = svg_find_gradient(ctx,a.fillGradient);
        if(gd!=nullptr) {
            res = svg_build_gradient(ctx,local_bounds,a.xform,gd,&out_style->fill_gradient);
            if(!SUCCEEDED(res)) {
                return res;
            }
        } else {
            a.hasFill = 0;
        }
    }
    if(a.hasStroke==2 && a.strokeGradient[0]!=0) {
        svg_grad_data* gd = svg_find_gradient(ctx,a.strokeGradient);
        if(gd!=nullptr) {
            res = svg_build_gradient(ctx,local_bounds,a.xform,gd,&out_style->stroke_gradient);
            if(!SUCCEEDED(res)) {
                if(out_style->fill_gradient.stops!=nullptr) {
                    ctx.deallocator(out_style->fill_gradient.stops);
                    out_style->fill_gradient.stops=nullptr;
                    out_style->fill_gradient.stops_size = 0;
                }
                return res;
            }
        } else {
            a.hasStroke = 0;
        }
    }
    out_style->fill_opacity = a.fillOpacity*a.opacity;
    switch (a.hasFill) {
        case 0:
            out_style->fill_paint_type = paint_type::none;
            break;
        case 1:
            out_style->fill_paint_type = paint_type::solid;
            break;
        case 2:
            out_style->fill_paint_type = paint_type::gradient;
            break;
    }

    out_style->stroke_color = a.strokeColor;
    out_style->stroke_opacity = a.strokeOpacity*a.opacity;
    switch (a.hasStroke) {
        case 0:
            out_style->stroke_paint_type = paint_type::none;
            break;
        case 1:
            out_style->stroke_paint_type = paint_type::solid;
            break;
        case 2:
            out_style->stroke_paint_type = paint_type::gradient;
            break;
    }

    dash dsh;
    dsh.offset = a.strokeDashOffset;
    dsh.values = (a.strokeDashCount) ? a.strokeDashArray : nullptr;
    dsh.values_size = a.strokeDashCount;
    out_style->stroke_dash = dsh;
    out_style->stroke_line_cap = a.strokeLineCap;
    out_style->stroke_line_join = a.strokeLineJoin;
    out_style->stroke_miter_limit = a.miterLimit;
    out_style->stroke_width = a.strokeWidth;
    out_style->fill_rule = a.fillRule;
    out_style->font_size = a.fontSize;
    // TODO: Figure out stop-color, stop-opacity
    return res;
}
static result_t svg_parse_path_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    canvas_path path;
    res = path.initialize();
    if (!SUCCEEDED(res)) {
        return res;
    }
    bool has_attrs = false;
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if(has_attrs==false)
            has_attrs = id!=ATTR_D;
        
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        switch (id) {
            case ATTR_D:
                res = svg_parse_path(ctx, path);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            case ATTR_STYLE:
                res = svg_parse_style(ctx, &sz,false);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;

            // TODO: ATTR_CLASS
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    
    return svg_draw_path(ctx,path);
}
static void svg_apply_view_transform(svg_context& ctx, float width,
                                     float height) {
    if(ctx.view_box_width==0 || ctx.view_box_height==0) {
        return;
    }
    float scale_x = width / ctx.view_box_width;
    float scale_y = height / ctx.view_box_height;
    if (ctx.align_type == view_align_none) {
        ctx.xform = ctx.xform * matrix::create_scale(scale_x, scale_y);
        ctx.xform = ctx.xform *
                    matrix::create_translate(-ctx.view_box_min_x, -ctx.view_box_min_y);
    } else {
        float scale = (ctx.align_scale == view_scale_meet)
                          ? math::min_(scale_x, scale_y)
                          : math::max_(scale_x, scale_y);
        float offset_x = -ctx.view_box_min_x * scale;
        float offset_y = -ctx.view_box_min_y * scale;
        float view_width = ctx.view_box_width * scale;
        float view_height = ctx.view_box_height * scale;
        switch (ctx.align_type) {
            case view_align_x_mid_y_min:
            case view_align_x_mid_y_mid:
            case view_align_x_mid_y_max:
                offset_x += (width - view_width) * 0.5f;
                break;
            case view_align_x_max_y_min:
            case view_align_x_max_y_mid:
            case view_align_x_max_y_max:
                offset_x += (width - view_width);
                break;
            default:
                break;
        }

        switch (ctx.align_type) {
            case view_align_x_min_y_mid:
            case view_align_x_mid_y_mid:
            case view_align_x_max_y_mid:
                offset_y += (height - view_height) * 0.5f;
                break;
            case view_align_x_min_y_max:
            case view_align_x_mid_y_max:
            case view_align_x_max_y_max:
                offset_y += (height - view_height);
                break;
            default:
                break;
        }

        ctx.xform = matrix::create_translate(offset_x, offset_y) * ctx.xform;
        ctx.xform = matrix::create_scale(scale, scale) * ctx.xform;
    }

    ctx.view_width = ctx.view_box_width;
    ctx.view_height = ctx.view_box_height;
}
static result_t svg_parse_gradient_stop_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    svg_attrib& a = svg_get_attr(ctx);
    a.stopOffset = {0,UNITS_PERCENT};
    a.stopColor.native_value = 0;
    a.stopOpacity = 1.0f;

    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
    
        res=svg_parse_attribute_id(ctx,&sz,id,false);
        if(!SUCCEEDED(res)) {
            return res;
        }    
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    svg_apply_css(&a,ctx.css_current);
    a.stopOpacity = fabsf(a.stopOpacity);
    if(a.stopOpacity>1.0f) {
        a.stopOpacity=1.0f;
    }
    if(ctx.grad_tail!=nullptr) {
        if(ctx.grad_tail->stops==nullptr) {
            ctx.grad_tail->stops = (svg_grad_stop*)ctx.allocator(sizeof(svg_grad_stop));
            if(ctx.grad_tail->stops==nullptr) {
                ctx.grad_tail->stops_size = 0;
                return OUT_OF_MEMORY;
            }
            ctx.grad_tail->stops_size = 1;
        } else {
            size_t sz = (ctx.grad_tail->stops_size+1)*sizeof(svg_grad_stop);
            ctx.grad_tail->stops = (svg_grad_stop*)ctx.reallocator(ctx.grad_tail->stops,sz);
            if(ctx.grad_tail->stops==nullptr) {
                ctx.grad_tail->stops_size = 0;
                return OUT_OF_MEMORY;
            }
            ++ctx.grad_tail->stops_size;
        }
        size_t idx=ctx.grad_tail->stops_size-1;
        ctx.grad_tail->stops[idx].color=a.stopColor;
        ctx.grad_tail->stops[idx].offset=a.stopOffset;
        ctx.grad_tail->stops[idx].opacity=a.stopOpacity;
    }
    return SUCCESS;
}
static result_t svg_parse_rect_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    canvas_path path;
    res = path.initialize();
    if (!SUCCEEDED(res)) {
        return res;
    }
    float x=0,y=0,width=0,height=0,rx=-1,ry=-1;
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        svg_coord coord;
        switch (id) {
            case ATTR_X:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                x=svg_convert_to_pixels(ctx,coord,svg_actual_orig_x(ctx),svg_actual_width(ctx));
                break;
            case ATTR_Y:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                y=svg_convert_to_pixels(ctx,coord,svg_actual_orig_y(ctx),svg_actual_height(ctx));
                break;
            case ATTR_WIDTH:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                width=svg_convert_to_pixels(ctx,coord,0.0f,svg_actual_width(ctx));
                break;
            case ATTR_HEIGHT:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                height=svg_convert_to_pixels(ctx,coord,0.0f,svg_actual_height(ctx));
                break;
            case ATTR_RX:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                rx=fabsf(svg_convert_to_pixels(ctx,coord, 0.0f, svg_actual_width(ctx)));
                break;
            case ATTR_RY:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                ry=fabsf(svg_convert_to_pixels(ctx,coord, 0.0f, svg_actual_height(ctx)));
                break;
            case ATTR_STYLE:
                res = svg_parse_style(ctx, &sz,false);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    if (rx < 0.0f && ry > 0.0f) rx = ry;
    if (ry < 0.0f && rx > 0.0f) ry = rx;
    if (rx < 0.0f) rx = 0.0f;
    if (ry < 0.0f) ry = 0.0f;
    if (rx > width / 2.0f) rx = width / 2.0f;
    if (ry > height / 2.0f) ry = height / 2.0f;
    if (rx < 0.00001f || ry < 0.0001f) {
        res= path.rectangle(rectf(pointf(x,y),sizef(width,height)));
        if(!SUCCEEDED(res)) {
            return res;
        }
    } else {
        res= path.rounded_rectangle(rectf(pointf(x,y),sizef(width,height)),{rx,ry});
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    
    return svg_draw_path(ctx,path);
}
static result_t svg_parse_line_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    canvas_path path;
    res = path.initialize();
    if (!SUCCEEDED(res)) {
        return res;
    }
    float x1=0,y1=0,x2=0,y2=0;
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        svg_coord coord;
        switch (id) {
            case ATTR_X1:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                x1=svg_convert_to_pixels(ctx,coord,svg_actual_orig_x(ctx),svg_actual_width(ctx));
                break;
            case ATTR_Y1:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                y1=svg_convert_to_pixels(ctx,coord,svg_actual_orig_y(ctx),svg_actual_height(ctx));
                break;
            case ATTR_X2:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                x2=svg_convert_to_pixels(ctx,coord,0.0f,svg_actual_width(ctx));
                break;
            case ATTR_Y2:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                y2=svg_convert_to_pixels(ctx,coord,0.0f,svg_actual_height(ctx));
                break;
            case ATTR_STYLE:
                res = svg_parse_style(ctx, &sz,false);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    res= path.move_to(pointf(x1,y1));
    if(!SUCCEEDED(res)) {
        return res;
    }

    res= path.line_to(pointf(x2,y2));
    if(!SUCCEEDED(res)) {
        return res;
    }
    return svg_draw_path(ctx,path);
}
static result_t svg_parse_poly_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    canvas_path path;
    res = path.initialize();
    if (!SUCCEEDED(res)) {
        return res;
    }
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        switch (id) {
            case ATTR_POINTS:
                res = svg_parse_poly_points(ctx,&sz,path);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                break;
            case ATTR_STYLE:
                res = svg_parse_style(ctx, &sz,false);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    if(ctx.tag_id==TAG_POLYGON) {
        res=path.close();
        if(!SUCCEEDED(res)) {
            return res;
        }
    }
    return svg_draw_path(ctx,path);
}
static result_t svg_parse_ellipse_or_circle_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    int id = 0;
    result_t res = SUCCESS;
    canvas_path path;
    res = path.initialize();
    if (!SUCCEEDED(res)) {
        return res;
    }
    float cx=0,cy=0,rx=-1,ry=-1,r=-1;
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        svg_coord coord;
        switch (id) {
            case ATTR_CX:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                cx=svg_convert_to_pixels(ctx,coord,svg_actual_orig_x(ctx),svg_actual_width(ctx));
                break;
            case ATTR_CY:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                cy=svg_convert_to_pixels(ctx,coord,svg_actual_orig_y(ctx),svg_actual_height(ctx));
                break;
            case ATTR_RX:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                rx=fabsf(svg_convert_to_pixels(ctx,coord, 0.0f, svg_actual_width(ctx)));
                break;
            case ATTR_RY:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                ry=fabsf(svg_convert_to_pixels(ctx,coord, 0.0f, svg_actual_height(ctx)));
                break;
            case ATTR_R:
                res = svg_parse_coordinate(ctx,&sz,&coord);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                r=fabsf(svg_convert_to_pixels(ctx,coord, 0.0f, svg_actual_length(ctx)));
                break;
            case ATTR_STYLE:
                res = svg_parse_style(ctx, &sz,false);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    if (rx < 0.0f && ry > 0.0f) rx = ry;
    if (ry < 0.0f && rx > 0.0f) ry = rx;
    if (rx < 0.0f) rx = 0.0f;
    if (ry < 0.0f) ry = 0.0f;
    res= ctx.tag_id==TAG_ELLIPSE?path.ellipse(pointf(cx,cy),{rx,ry}):path.circle(pointf(cx,cy),r);
    if(!SUCCEEDED(res)) {
        return res;
    }
    return svg_draw_path(ctx,path);
}
static result_t svg_parse_svg_elem(svg_context& ctx) {
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    int id = 0;
    svg_coord coord;
    result_t res = SUCCESS;
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            return IO_ERROR;
        }
        const char* sz = ctx.rdr.value();
        switch (id) {
            case ATTR_WIDTH:
                res = svg_parse_coordinate(ctx, &sz, &coord);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                ctx.dimensions.width =
                    svg_convert_to_pixels(ctx, coord, 0.0f, 0.0f);
                ctx.view_width = ctx.dimensions.width;
                break;
            case ATTR_HEIGHT:
                res = svg_parse_coordinate(ctx, &sz, &coord);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                ctx.dimensions.height =
                    svg_convert_to_pixels(ctx, coord, 0.0f, 0.0f);
                ctx.view_height = ctx.dimensions.height;
                break;
            case ATTR_VIEW_BOX:
                res = svg_parse_view_box(ctx, &sz);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            case ATTR_PRESERVE_ASPECT_RATIO:
                res = svg_parse_alpha_str(ctx, &sz, ctx.tmp_value, MAX_VALUE);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                if (strstr(ctx.tmp_value, "none") != 0) {
                    // No uniform scaling
                    ctx.align_type = view_align_none;
                } else {
                    // Parse X align
                    if (strstr(ctx.tmp_value, "xMinYMin") != 0)
                        ctx.align_type = view_align_x_mid_y_min;
                    else if (strstr(ctx.tmp_value, "xMidYMin") != 0)
                        ctx.align_type = view_align_x_mid_y_min;
                    else if (strstr(ctx.tmp_value, "xMaxYMin") != 0)
                        ctx.align_type = view_align_x_max_y_min;
                    else if (strstr(ctx.tmp_value, "xMinYMid") != 0)
                        ctx.align_type = view_align_x_min_y_mid;
                    else if (strstr(ctx.tmp_value, "xMidYMid") != 0)
                        ctx.align_type = view_align_x_mid_y_mid;
                    else if (strstr(ctx.tmp_value, "xMaxYMid") != 0)
                        ctx.align_type = view_align_x_max_y_mid;
                    else if (strstr(ctx.tmp_value, "xMinYMax") != 0)
                        ctx.align_type = view_align_x_min_y_max;
                    else if (strstr(ctx.tmp_value, "xMidYMax") != 0)
                        ctx.align_type = view_align_x_mid_y_max;
                    else if (strstr(ctx.tmp_value, "xMaxYMax") != 0)
                        ctx.align_type = view_align_x_max_y_max;
                    ctx.align_scale = view_scale_meet;
                    if (sz && *sz) {
                        res = svg_skip_space(ctx, &sz);
                        if (!SUCCEEDED(res)) {
                            return res;
                        }
                        if (sz && *sz) {
                            res = svg_parse_alpha_str(ctx, &sz, ctx.tmp_value,
                                                      MAX_VALUE);
                            if (!SUCCEEDED(res)) {
                                return res;
                            }
                        }
                        if (0 == strcmp(ctx.tmp_value, "slice")) {
                            ctx.align_scale = view_scale_slice;
                        }
                    }
                }
                break;
            // TODO: ATTR_CLASS
            default:
                if (id != 0) {
                    res = svg_parse_attribute_id(ctx, &sz, id,false);
                    if (!SUCCEEDED(res)) {
                        return res;
                    }
                } else {
                    // skip
                    while (ctx.rdr.node_type() ==
                           ml_node_type::attribute_content) {
                        if (!ctx.rdr.read()) {
                            return IO_ERROR;
                        }
                    }
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    
    float intrinsic_width = ctx.dimensions.width==0?-1:ctx.dimensions.width;
    float intrinsic_height = ctx.dimensions.height==0?-1:ctx.dimensions.height;
    if (intrinsic_width <= 0.f || intrinsic_height <= 0.f) {
        if (ctx.view_box_height > 0 && ctx.view_box_width > 0) {
            float intrinsic_ratio = ctx.view_box_width / ctx.view_box_height;
            if (intrinsic_width <= 0.f && intrinsic_height > 0.f) {
                intrinsic_width = intrinsic_height * intrinsic_ratio;
            } else if (intrinsic_width > 0.f && intrinsic_height <= 0.f) {
                intrinsic_height = intrinsic_width / intrinsic_ratio;
            } else {
                intrinsic_width = ctx.view_box_width;
                intrinsic_height = ctx.view_box_height;
            }
        } else {
            // don't really have anything to go by so use SVG defaults
            if (intrinsic_width == -1) intrinsic_width = 300;
            if (intrinsic_height == -1) {
                intrinsic_height = 150;
            }
        }
    }

    if (intrinsic_width <= 0.f || intrinsic_height <= 0.f) {
        return gfx_result::invalid_format;
    }
    ctx.dimensions.width = intrinsic_width;
    ctx.dimensions.height = intrinsic_height;

    float image_width = intrinsic_width;
    float image_height = intrinsic_height;
    // float dst_width = ctx.bounds.x2 - ctx.bounds.x1 + 1;
    // float dst_height = ctx.bounds.y2 - ctx.bounds.y1 + 1;
    // float scale_x = dst_width / image_width;
    // float scale_y = dst_height / image_height;
    // ctx.xform = ::gfx::matrix::create_translate(ctx.bounds.x1, ctx.bounds.y1) *ctx.xform ;
    // ctx.xform = ::gfx::matrix::create_scale(scale_x, scale_y) * ctx.xform ;
    
    svg_apply_view_transform(ctx, image_width, image_height);
    if(ctx.view_width==0) {
        ctx.view_width = image_width;
    }
    if(ctx.view_height==0) {
        ctx.view_width = image_height;
    }
    return SUCCESS;
}
static result_t svg_parse_css_data(svg_context& ctx, const char** current) {
    result_t res = svg_ensure_current(ctx,current);
    
    if(!SUCCEEDED(res)) {return res;}
    res=svg_skip_space(ctx,current);
    if(!ctx.rdr.has_content()) {
        return SUCCESS;
    }
    if(!SUCCEEDED(res)) {return res;}
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) {return res;}
    bool isid = false;
    switch(**current) {
        case '#':
            isid=true;
            break;
        case '.':
            break;
        default:
            return FMT_ERROR;
    }
    ++(*current);
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) {return res;}
    svg_css_data* data = (svg_css_data*)ctx.allocator(sizeof(svg_css_data));
    if(data==nullptr) {
        return OUT_OF_MEMORY;
    }
    
    data->is_id=isid;
    data->next = nullptr;
    res=svg_parse_alphanum_str(ctx,current,data->selector,MAX_NAME);
    if(!SUCCEEDED(res)) {return res;}
    res=svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) {return res;}
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) {return res;}
    if(!*current || **current!='{') { res= FMT_ERROR; goto error;}
    ++(*current);
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) { goto error;}
    res=svg_parse_style(ctx,current,true);
    if(!SUCCEEDED(res)) { goto error;}
    res=svg_skip_space(ctx,current);
    if(!SUCCEEDED(res)) { goto error;}
    res = svg_ensure_current(ctx,current);
    if(!SUCCEEDED(res)) { goto error;}
    data->has= ctx.css_has.attr;
    memset(&ctx.css_has,0,sizeof(svg_css_has));
    data->data=svg_get_attr(ctx);
    if(ctx.css_head==nullptr) {
        ctx.css_head = data;
        ctx.css_tail = data;
    } else {
        ctx.css_tail->next = data;
        ctx.css_tail = data;
    }
    if(*current==nullptr || **current!='}') {
        res = FMT_ERROR;
        goto error;
    }
    ++(*current);
    data = nullptr;

error:
    if(data!=nullptr) {
        ctx.deallocator(data);
    }
    return res;
}
static result_t svg_parse_style_elem(svg_context& ctx) {
    if(ctx.rdr.node_type()!=ml_node_type::element) {
        return FMT_ERROR;
    }
    while(ctx.rdr.node_type()!=ml_node_type::content && ctx.rdr.read());
    if(ctx.rdr.node_type()!=ml_node_type::content) {
        return IO_ERROR;
    }
    const char* sz;
    int depth = ctx.rdr.depth();
    while(ctx.rdr.node_type()!=ml_node_type::element_end && ctx.rdr.depth()>=depth) {
        if(ctx.rdr.node_type()==ml_node_type::content) {
            sz = ctx.rdr.value();
            while(ctx.rdr.has_content()) {
                result_t res=svg_parse_css_data(ctx,&sz);
                if(!SUCCEEDED(res)) {
                    return res;
                }
            }
        }
    }
    return SUCCESS;
}

static result_t svg_parse_gradient_elem(svg_context& ctx) {
    bool has_fx = false, has_fy = false;
    const char* sz ;
    int attr_id;
    
    if (!ctx.rdr.read() || ctx.rdr.node_type() != ml_node_type::attribute) {
        return FMT_ERROR;
    }
    ctx.css_current = nullptr;
    result_t res = SUCCESS;
    svg_coord x1,y1,x2,y2,fx,fy,cx,cy,r;
    svg_grad_data* data = (svg_grad_data*)ctx.allocator(sizeof(svg_grad_data));
    if(data==nullptr) {
        res= OUT_OF_MEMORY;
        goto error;
    }
    data->id[0]=0;
    data->ref[0]=0;
    data->transform = matrix::create_identity();
    data->next = nullptr;
    data->stops = nullptr;
    data->stops_size = 0;
    data->spread = spread_method::pad;
    data->type = ctx.tag_id == TAG_LINEAR_GRADIENT?gradient_type::linear:gradient_type::radial;
    data->units = OBJECT_SPACE;
    x1 = {0,UNITS_USER};
    x2 = {100,UNITS_PERCENT};
    y1 = {0,UNITS_USER};
    y2 = {0,UNITS_USER};
    cx = {50,UNITS_PERCENT};
    cy = {50,UNITS_PERCENT};
    fx = {50,UNITS_PERCENT};
    fy = {50,UNITS_PERCENT};
    r = {100,UNITS_PERCENT};
    while (ctx.rdr.node_type() == ml_node_type::attribute) {
        attr_id = svg_attributeid(ctx.rdr.value());
        if (!ctx.rdr.read()) {
            res = IO_ERROR;
            goto error;
        }
        sz = ctx.rdr.value();
  
        switch (attr_id) {
            case ATTR_X1:
                res = svg_parse_coordinate(ctx,&sz,&x1);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_X2:
                res = svg_parse_coordinate(ctx,&sz,&x2);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_Y1:
                res = svg_parse_coordinate(ctx,&sz,&y1);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_Y2:
                res = svg_parse_coordinate(ctx,&sz,&y2);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;

            case ATTR_CX:
                res = svg_parse_coordinate(ctx,&sz,&cx);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_FX:
                res = svg_parse_coordinate(ctx,&sz,&fx);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                has_fx = true;
                break;
            case ATTR_R:
                res = svg_parse_coordinate(ctx,&sz,&r);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_CY:
                res = svg_parse_coordinate(ctx,&sz,&cy);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_FY:
                res = svg_parse_coordinate(ctx,&sz,&fy);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                has_fy = true;
                break;
            case ATTR_SPREAD_METHOD:
                res=svg_parse_spread_method(ctx,&sz,&data->spread);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_GRADIENT_UNITS:
                res=svg_parse_gradient_units(ctx,&sz,&data->units);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_XLINK_HREF:
                res=svg_parse_ref(ctx,&sz,data->ref,MAX_ID);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_GRADIENT_TRANSFORM:
                res=svg_parse_transform(ctx,&sz,&data->transform);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ATTR_ID:
                res = svg_parse_attribute_id(ctx,&sz,attr_id,false);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                strcpy(data->id,svg_get_attr(ctx).id);
                break;
            default:
                res = svg_parse_attribute_id(ctx,&sz,attr_id,false);
                if(!SUCCEEDED(res)) {
                    goto error;
                }
                break;
        }
        if (ctx.rdr.node_type() == ml_node_type::attribute_end) {
            if (!ctx.rdr.read()) {
                return IO_ERROR;
            }
        }
    }
    
    if(data->type==gradient_type::linear) {
        data->linear.x1 = x1;
        data->linear.y1 = y1;
        data->linear.x2 = x2;
        data->linear.y2 = y2;
    } else {
        data->radial.cr = r;
        data->radial.cx = cx;
        data->radial.cy = cy;
        data->radial.fr = {0.0f,UNITS_USER};
        if(!has_fx) {
            data->radial.fx = cx;
        } else {
            data->radial.fx = fx;
        }
        if(!has_fy) {
            data->radial.fy = cy;
        } else {
            data->radial.fy = fy;
        }
    }
    if(ctx.grad_head==nullptr) {
        ctx.grad_head = data;
        ctx.grad_tail = data;
    } else {
        ctx.grad_tail->next=data;
        ctx.grad_tail=data;
    }
    data = nullptr;
error:
    if(data!=nullptr) {
        if(data->stops!=nullptr) {
            ctx.deallocator(data->stops);
        }
        ctx.deallocator(data);
    }
    
    return res;
}
static result_t svg_draw_path(svg_context& ctx,const canvas_path& path) {
    result_t res;
    canvas_style s;
    res = svg_apply_attribute(ctx, path.bounds(true), &s);
    if(res!=gfx_result::success) {
        return res;
    }
    svg_attrib& a = svg_get_attr(ctx);
    ctx.cvs->transform(a.xform * ctx.xform);
    ctx.cvs->style(s);
    res=ctx.cvs->path(path);
    if(!SUCCEEDED(res)) {
        goto error;
    }
    res= ctx.cvs->render(false,ctx.allocator,ctx.reallocator,ctx.deallocator);
error:
    if(s.fill_gradient.stops!=nullptr) {
        ctx.deallocator(s.fill_gradient.stops);
        s.fill_gradient.stops  =nullptr;
        s.fill_gradient.stops_size = 0;
    }
    if(s.stroke_gradient.stops!=nullptr) {
        ctx.deallocator(s.stroke_gradient.stops);
        s.stroke_gradient.stops  =nullptr;
        s.stroke_gradient.stops_size = 0;
    }
    return res;
}
static result_t svg_skip_to_end_elem(svg_context& ctx) {
    int depth = ctx.rdr.depth();
    while(ctx.rdr.node_type()!=ml_node_type::element_end && ctx.rdr.depth()>=depth) {
        if(!ctx.rdr.read()) {
            return IO_ERROR;
        }
    }
    return SUCCESS;
}
static result_t svg_parse_start_element(svg_context& ctx) {
    result_t res = SUCCESS;
    int id = svg_elementid(ctx.rdr.value());
    ctx.tag_id = id;
    if (ctx.in_defs) {
        switch (id) {
            case TAG_STYLE:
                res = svg_push_attr(ctx);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                svg_init_attribute(svg_get_attr(ctx));
                res= svg_parse_style_elem(ctx);
                if (!SUCCEEDED(res)) {
                    return res;
                }
                break;
            case TAG_RADIAL_GRADIENT:
            case TAG_LINEAR_GRADIENT:
                res=svg_parse_gradient_elem(ctx);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                break;
            case TAG_STOP:
                res=svg_parse_gradient_stop_elem(ctx);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                break;
            default:
                res=svg_skip_to_end_elem(ctx);
                if(!SUCCEEDED(res)) {
                    return res;
                }
                break;
        }
        return SUCCESS;
    }
    switch (id) {
        case TAG_STYLE:
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_init_attribute(svg_get_attr(ctx));
            res= svg_parse_style_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            break;
        case TAG_DEFS:
            ctx.in_defs = true;
            if(!ctx.rdr.read()) {
                return IO_ERROR;
            }
            break;
        case TAG_G:
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_attributes(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            break;
        case TAG_PATH:
            if (ctx.in_path) {
                return NOT_SUPPORTED;
            }
            ctx.in_path = true;
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_path_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_pop_attr(ctx);
            break;
        case TAG_RECT:
            if (ctx.in_path) {
                return NOT_SUPPORTED;
            }
            ctx.in_path = true;
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_rect_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_pop_attr(ctx);
            break;
        case TAG_ELLIPSE:
        case TAG_CIRCLE:
            if (ctx.in_path) {
                return NOT_SUPPORTED;
            }
            ctx.in_path = true;
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_ellipse_or_circle_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_pop_attr(ctx);
            break;
        case TAG_LINE:
            if (ctx.in_path) {
                return NOT_SUPPORTED;
            }
            ctx.in_path = true;
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_line_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_pop_attr(ctx);
            break;
        case TAG_POLYLINE:
        case TAG_POLYGON:
            if (ctx.in_path) {
                return NOT_SUPPORTED;
            }
            ctx.in_path = true;
            res = svg_push_attr(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            res = svg_parse_poly_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            svg_pop_attr(ctx);
            break;
        case TAG_SVG:
            res = svg_parse_svg_elem(ctx);
            if (!SUCCEEDED(res)) {
                return res;
            }
            break;
        case TAG_RADIAL_GRADIENT:
        case TAG_LINEAR_GRADIENT:
            res=svg_parse_gradient_elem(ctx);
            if(!SUCCEEDED(res)) {
                return res;
            }
            break;
        case TAG_STOP:
            res=svg_parse_gradient_stop_elem(ctx);
            if(!SUCCEEDED(res)) {
                return res;
            }
            break;
        default:
            res=svg_skip_to_end_elem(ctx);
            if(!SUCCEEDED(res)) {
                return res;
            }
            break;
            // return NOT_SUPPORTED;
    }
    return SUCCESS;
}
static result_t svg_parse_end_element(svg_context& ctx) {
    if(!ctx.rdr.is_empty_element()) {
        ctx.tag_id = svg_elementid(ctx.rdr.value());
    }
    switch (ctx.tag_id) {
        case TAG_G:
        case TAG_STYLE:
        //case TAG_LINEAR_GRADIENT:
        //case TAG_RADIAL_GRADIENT:
            svg_pop_attr(ctx);
            break;

        case TAG_RECT:
        case TAG_CIRCLE:
        case TAG_ELLIPSE:
        case TAG_LINE:
        case TAG_POLYGON:
        case TAG_POLYLINE:
        case TAG_PATH:
            ctx.in_path = false;
            break;
        case TAG_DEFS:
            ctx.in_defs = false;
            break;
        default:
            break;
    }
    return SUCCESS;
}
/*static void svg_transform_view_rect(svg_context& ctx, rectf* dst_rect,
                                    rectf* src_rect) {
    if (ctx.align_type == view_align_none) return;
    float view_width = dst_rect->width();
    float view_height = dst_rect->height();
    float image_width = src_rect->width();
    float image_height = src_rect->height();
    if (ctx.align_scale == view_scale_meet) {
        float scale = image_height / image_width;
        if (view_height > view_width * scale) {
            dst_rect->y2 = dst_rect->y1 + view_width * scale - 1;
            switch (ctx.align_type) {
                case view_align_x_min_y_mid:
                case view_align_x_mid_y_mid:
                case view_align_x_max_y_mid:
                    dst_rect->offset_inplace(
                        0, (view_height - dst_rect->height()) * 0.5f);
                    break;
                case view_align_x_min_y_max:
                case view_align_x_mid_y_max:
                case view_align_x_max_y_max:
                    dst_rect->offset_inplace(0,
                                             view_height - dst_rect->height());
                    break;
                default:
                    break;
            }
        }

        if (view_width > view_height / scale) {
            dst_rect->y2 = dst_rect->y1 + view_height / scale - 1;
            switch (ctx.align_type) {
                case view_align_x_mid_y_min:
                case view_align_x_mid_y_mid:
                case view_align_x_mid_y_max:
                    dst_rect->offset_inplace(
                        (view_width - dst_rect->width()) * 0.5f, 0);
                    break;
                case view_align_x_max_y_min:
                case view_align_x_max_y_mid:
                case view_align_x_max_y_max:
                    dst_rect->offset_inplace(view_width - dst_rect->width(), 0);
                    break;
                default:
                    break;
            }
        }
    } else if (ctx.align_scale == view_scale_slice) {
        float scale = image_height / image_width;
        if (view_height < view_width * scale) {
            src_rect->y2 =
                src_rect->y1 + view_height * (image_width / view_width) - 1;
            switch (ctx.align_type) {
                case view_align_x_min_y_mid:
                case view_align_x_mid_y_mid:
                case view_align_x_max_y_mid:
                    src_rect->offset_inplace(
                        0, (image_height - src_rect->height()) * 0.5f);
                    break;
                case view_align_x_min_y_max:
                case view_align_x_mid_y_max:
                case view_align_x_max_y_max:
                    src_rect->offset_inplace(0,
                                             image_height - src_rect->height());
                    break;
                default:
                    break;
            }
        }

        if (view_width < view_height / scale) {
            src_rect->x2 =
                src_rect->x1 + view_width * (image_height / view_height) - 1;
            switch (ctx.align_type) {
                case view_align_x_mid_y_min:
                case view_align_x_mid_y_mid:
                case view_align_x_mid_y_max:
                    src_rect->offset_inplace(
                        (image_width - src_rect->width()) * 0.5f, 0);
                    break;
                case view_align_x_max_y_min:
                case view_align_x_max_y_mid:
                case view_align_x_max_y_max:
                    src_rect->offset_inplace(image_width - src_rect->width(),
                                             0);
                    break;
                default:
                    break;
            }
        }
    }
}*/
static result_t svg_document_dimensions(stream& stream, sizef* out_dimensions, float dpi = 96.f) {
    if(stream.caps().read==0) {
        return INVALID_ARG;
    }
    unsigned long long spos = 0;
    if(stream.caps().seek!=0) {
        spos = stream.seek(0,seek_origin::current);
    }
    svg_context* pctx= new svg_context();
    if(pctx==nullptr) {
        return OUT_OF_MEMORY;
    }
    svg_init_context(*pctx);
    result_t res=SUCCESS;
    rectf vr;
    pctx->dpi = dpi;
    pctx->bounds = {0,0,299,299};
    pctx->dimensions = {300,300};
    pctx->cvs    = nullptr;
    pctx->rdr.set(stream);
    int id=-1;
    while(id!=TAG_SVG && pctx->rdr.read()) {
        if(pctx->rdr.node_type()==ml_node_type::element) {
            id = svg_elementid(pctx->rdr.value());
        }
    }
    if(id!=TAG_SVG) {
        return INVALID_ARG;
    }
    pctx->tag_id = id;
    res=svg_parse_svg_elem(*pctx);
    if(SUCCEEDED(res)) {
        out_dimensions->width=pctx->dimensions.width;
        out_dimensions->height=pctx->dimensions.height;
    }
    svg_delete_context(*pctx);
    delete pctx;
    if(stream.caps().seek!=0) {
        stream.seek(spos);
    }
    return res;
    
}
static result_t svg_render_document(stream& stream, canvas& destination, const matrix& transform, float dpi,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*)) {
    if(stream.caps().read==0) {
        return INVALID_ARG;
    }
    svg_context* pctx= new svg_context();
    if(pctx==nullptr) {
        return OUT_OF_MEMORY;
    }
    svg_init_context(*pctx);
    result_t res=SUCCESS;
    rectf vr;
    pctx->dpi = dpi;
    pctx->cvs    = &destination;
    pctx->xform = transform;
    pctx->allocator = allocator;
    pctx->reallocator = reallocator;
    pctx->deallocator = deallocator;
    pctx->rdr.set(stream);
    matrix init_xfrm = destination.transform();
    canvas_style init_style = destination.style();
    bool done = !pctx->rdr.read();
    while (!done) {
        ml::ml_node_type nt = pctx->rdr.node_type();
        switch (nt) {
            case ::ml::ml_node_type::element:
                res = svg_parse_start_element(*pctx);
                if (!SUCCEEDED(res)) {
                    goto error;
                }
                break;
            case ::ml::ml_node_type::element_end:
                res = svg_parse_end_element(*pctx);
                if (!SUCCEEDED(res)) {
                    goto error;
                }
                nt = pctx->rdr.node_type();
                if (nt == ::ml::ml_node_type::element_end &&
            (done=!pctx->rdr.read())) {
                    res=IO_ERROR;
                    goto error;
                }
                nt = pctx->rdr.node_type();
                break;
            case ::ml::ml_node_type::eof:
                done = true;
                break;
            case ::ml::ml_node_type::error_eclose:
            case ::ml::ml_node_type::error_eof:
            case ::ml::ml_node_type::error_eref:
            case ::ml::ml_node_type::error_overflow:
            case ::ml::ml_node_type::error_syntax:
                goto error;
            default:
                done = !pctx->rdr.read();
                break;
        }
    }
    
error:
    if(pctx!=nullptr) {
        svg_delete_context(*pctx);
        delete pctx;
    }
    destination.transform(init_xfrm);
    destination.style(init_style);
    return res;
}
gfx_result canvas::render_svg(stream& document, const matrix& transform, float dpi,void*(*allocator)(size_t),void*(*reallocator)(void*,size_t),void(*deallocator)(void*)) {
    if(allocator==nullptr) {
        allocator = this->m_allocator;
    }
    if(reallocator==nullptr) {
        reallocator = this->m_reallocator;
    }
    if(deallocator==nullptr) {
        deallocator = this->m_deallocator;
    }
    return svg_render_document(document,*this,transform,dpi,allocator,reallocator,deallocator);
}
gfx_result canvas::svg_dimensions(stream& document, sizef* out_dimensions,float dpi) {
    return svg_document_dimensions(document,out_dimensions,dpi);
}