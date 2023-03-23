#include <gfx_math.hpp>
#include <gfx_svg_doc.hpp>
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
//#define SVG_DUMP_PARSE
#include <string.h>

#include <ml_reader.hpp>

#include "nanosvgrast.h"
using namespace gfx;
using namespace ml;

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
    NSVGpath* plist;
    size_t image_size;
    NSVGimage* image;
    NSVGgradientData* gradients;
    NSVGshape* shapesTail;
    float viewMinx, viewMiny, viewWidth, viewHeight;
    int alignX, alignY, alignType;
    float dpi;
    char pathFlag;
    char defsFlag;
};
#ifdef SVG_DUMP_PARSE
static void dump_path(NSVGpath* path) {
    if (path == nullptr) {
        //printf("\t\t<null>\n");
        return;
    }
    //printf("\t\tclosed: %d\n", (int)path->closed);
    //printf("\t\tbounds: %f %f %f %f\n", path->bounds[0], path->bounds[1], path->bounds[2], path->bounds[3]);
    //printf("\t\tnpts: %d\n", (int)path->npts);
}
static void dump_css_class(svg_css_class* cls) {
    if(cls==nullptr) {
        //printf("\t<null>\n");
        return;
    }
    //printf("\tselector: %s\n",cls->selector);
    //printf("\tvalue: %s\n",cls->value);
}
static void dump_shape(NSVGshape* shape) {
    if (shape == nullptr) {
        //printf("\t<null>\n");
        return;
    }
    //printf("\tid: %s\n", shape->id);
    //printf("\tflags: %d\n", (int)shape->flags);
    //printf("\tbounds: %f %f %f %f\n", shape->bounds[0], shape->bounds[1], shape->bounds[2], shape->bounds[3]);
    //printf("\topacity: %f\n", shape->opacity);
    //printf("\tfillGradient: %s\n", shape->fillGradient);
    //printf("\tfillRule: %d\n", (int)shape->fillRule);
    //printf("\tmiterLimit: %f\n", shape->miterLimit);
    //printf("\tstrokeDashCount: %d\n", (int)shape->strokeDashCount);
    //printf("\tstrokeDashOffset: %f\n", shape->strokeDashOffset);
    NSVGpath* pa = shape->paths;
    int count = 0;
    while (pa != nullptr) {
        ++count;
        pa = pa->next;
    }
    //printf("\tpaths: %d\n", count);
    pa = shape->paths;
    while (pa != nullptr) {
        dump_path(pa);
        pa = pa->next;
    }
}
static void dump_parse_res(svg_parse_result& p) {
    //printf("attrHead: %d\n", p.attrHead);
    //printf("npts: %d\n", p.npts);
    //printf("cpts: %d\n", p.cpts);
    //printf("viewMinx: %f, viewMiny: %f\n", p.viewMinx, p.viewMiny);
    //printf("viewWidth: %f, viewHeight: %f\n", p.viewWidth, p.viewHeight);
    //printf("alignX: %d, alignY: %d, alignType: %d\n", p.alignX, p.alignY, p.alignType);
    //printf("dpi: %f\n", p.dpi);
    //printf("pathsFlag: %d\n", (int)p.pathFlag);
    //printf("defsFlag: %d\n", (int)p.defsFlag);
    svg_css_class* cls = p.css_classes;
    int cls_count = 0;
    while(cls!=nullptr) {
        ++cls_count;
        cls = cls->next;
    }
    //printf("css_classes count: %d\n",cls_count);
    cls = p.css_classes;
    while(cls!=nullptr) {
        dump_css_class(cls);
        cls = cls->next;
    }
    if (p.image != nullptr) {
        //printf("image.width: %f\n", p.image->width);
        //printf("image.height: %f\n", p.image->height);
        NSVGshape* sh = p.image->shapes;
        int count = 0;
        while (sh != nullptr) {
            ++count;
            sh = sh->next;
        }
        //printf("image.shapes count: %d\n", count);
        sh = p.image->shapes;
        while (sh != nullptr) {
            dump_shape(sh);
            sh = sh->next;
            //break;
        }
    }
    //printf("\n");
}
static void dump_parse_res(NSVGparser& p) {
    //printf("attrHead: %d\n", p.attrHead);
    //printf("npts: %d\n", p.npts);
    //printf("cpts: %d\n", p.cpts);
    //printf("viewMinx: %f, viewMiny: %f\n", p.viewMinx, p.viewMiny);
    //printf("viewWidth: %f, viewHeight: %f\n", p.viewWidth, p.viewHeight);
    //printf("alignX: %d, alignY: %d, alignType: %d\n", p.alignX, p.alignY, p.alignType);
    //printf("dpi: %f\n", p.dpi);
    //printf("pathsFlag: %d\n", (int)p.pathFlag);
    //printf("defsFlag: %d\n", (int)p.defsFlag);
    if (p.image != nullptr) {
        //printf("image.width: %f\n", p.image->width);
        //printf("image.height: %f\n", p.image->height);
        NSVGshape* sh = p.image->shapes;
        int count = 0;
        while (sh != nullptr) {
            ++count;
            sh = sh->next;
        }
        //printf("image.shapes count: %d\n", count);
        sh = p.image->shapes;
        while (sh != nullptr) {
            dump_shape(sh);
            sh = sh->next;
            //break;
        }
    }
    //printf("\n");
}
#endif
static int svg_parse_name_value(svg_parse_result& p, const char* start, const char* end);
static void svg_parse_style(svg_parse_result& p, const char* str);
static int svg_parse_stroke_dash_array(svg_parse_result& p, const char* str, float* strokeDashArray);
static float svg_parse_coordinate(svg_parse_result& p, const char* str, float orig, float length);
static float svg_actual_orig_x(svg_parse_result& p);
static float svg_actual_orig_y(svg_parse_result& p);
static float svg_actual_width(svg_parse_result& p);
static float svg_actual_height(svg_parse_result& p);
static float svg_actual_length(svg_parse_result& p);
static gfx_result svg_push_attr(svg_parse_result& p) {
    if (p.attrHead < NSVG_MAX_ATTR - 1) {
        //printf("svg_push_attr\n");
        p.attrHead++;
        memcpy(&p.attr[p.attrHead], &p.attr[p.attrHead - 1], sizeof(NSVGattrib));
        return gfx_result::success;
    }
    return gfx_result::out_of_memory;
}

static void svg_pop_attr(svg_parse_result& p) {
    //printf("svg_pop_attr\n");
    if (p.attrHead > 0)
        p.attrHead--;
}
static NSVGattrib* svg_get_attr(svg_parse_result& p) {
    return &p.attr[p.attrHead];
}
static void svg_delete_parse_result(svg_parse_result& p) {
    svg_css_class* cls = p.css_classes;
    while(cls!=nullptr) {
        if(cls->value!=nullptr) {
            p.deallocator(cls->value);
        }
        svg_css_class* prev = cls;
        cls = cls->next;
        p.deallocator(prev);
    }
    p.css_classes = nullptr;
    nsvg__deletePaths(p.plist, p.deallocator);
    nsvg__deleteGradientData(p.gradients, p.deallocator);
    if(p.d!=nullptr) {
        p.deallocator(p.d);
    }
    nsvgDelete(p.image, p.deallocator);
    p.deallocator(p.pts);
    p.deallocator(&p);
}
static svg_css_class* svg_find_next_css_class(svg_css_class* start, const char* name) {
    size_t nl = strlen(name);
    while(start!=nullptr) {
        const char* sz = strstr(start->selector,name);
        if(sz>start->selector) {
            if((*(sz+nl)=='\0'||*(sz+nl)==',')&& *(sz-1)=='.') {
                return start;
            }
        }
        start=start->next;
    }
    return nullptr;
}
static gfx_result svg_apply_classes(svg_parse_result& p, const char* classes) {
    const char* ss = classes;
    char cn[64];
    char*sz;
    while(*ss) {
        sz = cn;
        *sz='\0';
        while(*ss && nsvg__isspace(*ss)) { ++ss; }
        if(!*ss) return gfx_result::invalid_format;
        int i = sizeof(cn)-1;
        while(i && *ss && !nsvg__isspace(*ss)) {
            *(sz++)=*(ss++);
            --i;
        }
        *sz='\0';
        if(p.css_classes!=nullptr && *cn) {
            svg_css_class* cls=svg_find_next_css_class(p.css_classes,cn);
            while(cls!=nullptr) {
                svg_parse_style(p,cls->value);
                cls=svg_find_next_css_class(cls->next,cn);
            }
        }
    }
    return gfx_result::success;
}
static int svg_parse_attr(svg_parse_result& p, const char* name, const char* value) {
    float xform[6];
    NSVGattrib* attr = svg_get_attr(p);
    if (!attr) return 0;
    if(strcmp(name,"class")==0) {
        svg_apply_classes(p,value);
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
            nsvg__parseUrl(attr->fillGradient, value);
        } else {
            attr->hasFill = 1;
            attr->fillColor = nsvg__parseColor(value);
        }
    } else if (strcmp(name, "opacity") == 0) {
        attr->opacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "fill-opacity") == 0) {
        attr->fillOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "stroke") == 0) {
        if (strcmp(value, "none") == 0) {
            attr->hasStroke = 0;
        } else if (strncmp(value, "url(", 4) == 0) {
            attr->hasStroke = 2;
            nsvg__parseUrl(attr->strokeGradient, value);
        } else {
            attr->hasStroke = 1;
            attr->strokeColor = nsvg__parseColor(value);
        }
    } else if (strcmp(name, "stroke-width") == 0) {
        attr->strokeWidth = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "stroke-dasharray") == 0) {
        attr->strokeDashCount = svg_parse_stroke_dash_array(p, value, attr->strokeDashArray);
    } else if (strcmp(name, "stroke-dashoffset") == 0) {
        attr->strokeDashOffset = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "stroke-opacity") == 0) {
        attr->strokeOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "stroke-linecap") == 0) {
        attr->strokeLineCap = nsvg__parseLineCap(value);
    } else if (strcmp(name, "stroke-linejoin") == 0) {
        attr->strokeLineJoin = nsvg__parseLineJoin(value);
    } else if (strcmp(name, "stroke-miterlimit") == 0) {
        attr->miterLimit = nsvg__parseMiterLimit(value);
    } else if (strcmp(name, "fill-rule") == 0) {
        attr->fillRule = nsvg__parseFillRule(value);
    } else if (strcmp(name, "font-size") == 0) {
        attr->fontSize = svg_parse_coordinate(p, value, 0.0f, svg_actual_length(p));
    } else if (strcmp(name, "transform") == 0) {
        nsvg__parseTransform(xform, value);
        nsvg__xformPremultiply(attr->xform, xform);
    } else if (strcmp(name, "stop-color") == 0) {
        attr->stopColor = nsvg__parseColor(value);
    } else if (strcmp(name, "stop-opacity") == 0) {
        attr->stopOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "offset") == 0) {
        attr->stopOffset = svg_parse_coordinate(p, value, 0.0f, 1.0f);
    } else if (strcmp(name, "id") == 0) {
        strncpy(attr->id, value, 63);
        attr->id[63] = '\0';
        //printf("ASSIGN ID: %s\n",attr->id);
    } else {
        return 0;
    }
    return 1;
}
static int svg_parse_attr(svg_parse_result& p) {
    reader_t& s = *p.reader;
    char name[32];
    char value[reader_t::capture_size];
    strncpy(name, s.value(), sizeof(name));
    if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
        return 0;
    }
    strncpy(value, s.value(), sizeof(value));
    if (!s.read() || s.node_type() == ml_node_type::attribute_content) {
        //printf("attribute is too long\n");
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
        case NSVG_UNITS_USER:
            return c.value;
        case NSVG_UNITS_PX:
            return c.value;
        case NSVG_UNITS_PT:
            return c.value / 72.0f * p.dpi;
        case NSVG_UNITS_PC:
            return c.value / 6.0f * p.dpi;
        case NSVG_UNITS_MM:
            return c.value / 25.4f * p.dpi;
        case NSVG_UNITS_CM:
            return c.value / 2.54f * p.dpi;
        case NSVG_UNITS_IN:
            return c.value * p.dpi;
        case NSVG_UNITS_EM:
            return c.value * attr->fontSize;
        case NSVG_UNITS_EX:
            return c.value * attr->fontSize * 0.52f;  // x-height of Helvetica.
        case NSVG_UNITS_PERCENT:
            return orig + c.value / 100.0f * length;
        default:
            return c.value;
    }
    return c.value;
}

static float svg_parse_coordinate(svg_parse_result& p, const char* str, float orig, float length) {
    NSVGcoordinate coord = nsvg__parseCoordinateRaw(str);
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
        str = nsvg__getNextDashItem(str, item);
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
        while (*str && nsvg__isspace(*str)) ++str;
        start = str;
        while (*str && *str != ';') ++str;
        end = str;

        // Right Trim
        while (end > start && (*end == ';' || nsvg__isspace(*end))) --end;
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
    while (str > start && (*str == ':' || nsvg__isspace(*str))) --str;
    ++str;

    n = (int)(str - start);
    if (n > 511) n = 511;
    if (n) memcpy(p.aname, start, n);
    p.aname[n] = 0;

    while (val < end && (*val == ':' || nsvg__isspace(*val))) ++val;

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
    p.style_val[0]='\0';
    p.class_val[0]='\0';

    while (s.node_type() == ml_node_type::attribute) {
        if(strcmp("style",s.value())==0) {
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
            strcpy(p.style_val,s.value());
            while((s.node_type()==ml_node_type::attribute_content||s.node_type()==ml_node_type::attribute_end) && s.read());
        } else if(strcmp("class",s.value())==0) {
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
            strcpy(p.class_val,s.value());
            while((s.node_type()==ml_node_type::attribute_content||s.node_type()==ml_node_type::attribute_end) && s.read());
        } else {
            svg_parse_attr(p);
        }
    }
    gfx_result res;
    if(*p.class_val) {
        res = svg_apply_classes(p,p.class_val);
        if (res != gfx_result::success) {
            return res;
        }
    }
    if(*p.style_val) {
        svg_parse_style(p,p.style_val);
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
static NSVGgradient* svg_create_gradient(svg_parse_result& p, const char* id, const float* localBounds, float* xform, signed char* paintType) {
    NSVGgradientData* data = NULL;
    NSVGgradientData* ref = NULL;
    NSVGgradientStop* stops = NULL;
    NSVGgradient* grad;
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
    size_t grad_sz = sizeof(NSVGgradient) + sizeof(NSVGgradientStop) * (nstops - 1);
    grad = (NSVGgradient*)p.allocator(grad_sz);
    if (grad == NULL) return NULL;
    p.image_size+=grad_sz;
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

    if (data->type == NSVG_PAINT_LINEAR_GRADIENT) {
        float x1, y1, x2, y2, dx, dy;
        x1 = svg_convert_to_pixels(p, data->linear.x1, ox, sw);
        y1 = svg_convert_to_pixels(p, data->linear.y1, oy, sh);
        x2 = svg_convert_to_pixels(p, data->linear.x2, ox, sw);
        y2 = svg_convert_to_pixels(p, data->linear.y2, oy, sh);
        // Calculate transform aligned to the line
        dx = x2 - x1;
        dy = y2 - y1;
        grad->xform[0] = dy;
        grad->xform[1] = -dx;
        grad->xform[2] = dx;
        grad->xform[3] = dy;
        grad->xform[4] = x1;
        grad->xform[5] = y1;
    } else {
        float cx, cy, fx, fy, r;
        cx = svg_convert_to_pixels(p, data->radial.cx, ox, sw);
        cy = svg_convert_to_pixels(p, data->radial.cy, oy, sh);
        fx = svg_convert_to_pixels(p, data->radial.fx, ox, sw);
        fy = svg_convert_to_pixels(p, data->radial.fy, oy, sh);
        r = svg_convert_to_pixels(p, data->radial.r, 0, sl);
        // Calculate transform aligned to the circle
        grad->xform[0] = r;
        grad->xform[1] = 0;
        grad->xform[2] = 0;
        grad->xform[3] = r;
        grad->xform[4] = cx;
        grad->xform[5] = cy;
        grad->fx = fx / r;
        grad->fy = fy / r;
    }

    nsvg__xformMultiply(grad->xform, data->xform);
    nsvg__xformMultiply(grad->xform, xform);

    grad->spread = data->spread;
    memcpy(grad->stops, stops, nstops * sizeof(NSVGgradientStop));
    grad->nstops = nstops;

    *paintType = data->type;

    return grad;
}

static void svg_create_gradients(svg_parse_result& p) {
    NSVGshape* shape;

    for (shape = p.image->shapes; shape != NULL; shape = shape->next) {
        if (shape->fill.type == NSVG_PAINT_UNDEF) {
            if (shape->fillGradient[0] != '\0') {
                float inv[6], localBounds[4];
                nsvg__xformInverse(inv, shape->xform);
                nsvg__getLocalBounds(localBounds, shape, inv);
                shape->fill.gradient = svg_create_gradient(p, shape->fillGradient, localBounds, shape->xform, &shape->fill.type);
            }
            if (shape->fill.type == NSVG_PAINT_UNDEF) {
                shape->fill.type = NSVG_PAINT_NONE;
            }
        }
        if (shape->stroke.type == NSVG_PAINT_UNDEF) {
            if (shape->strokeGradient[0] != '\0') {
                float inv[6], localBounds[4];
                nsvg__xformInverse(inv, shape->xform);
                nsvg__getLocalBounds(localBounds, shape, inv);
                shape->stroke.gradient = svg_create_gradient(p, shape->strokeGradient, localBounds, shape->xform, &shape->stroke.type);
            }
            if (shape->stroke.type == NSVG_PAINT_UNDEF) {
                shape->stroke.type = NSVG_PAINT_NONE;
            }
        }
    }
}

static void svg_scale_to_viewbox(svg_parse_result& p, const char* units) {
    NSVGshape* shape;
    NSVGpath* path;
    float tx, ty, sx, sy, us, bounds[4], t[6], avgs;
    int i;
    float* pt;

    // Guess image size if not set completely.
    nsvg__imageBounds(p.image, bounds);

    if (p.viewWidth == 0) {
        if (p.image->width > 0) {
            p.viewWidth = p.image->width;
        } else {
            p.viewMinx = bounds[0];
            p.viewWidth = bounds[2] - bounds[0];
        }
    }
    if (p.viewHeight == 0) {
        if (p.image->height > 0) {
            p.viewHeight = p.image->height;
        } else {
            p.viewMiny = bounds[1];
            p.viewHeight = bounds[3] - bounds[1];
        }
    }
    if (p.image->width == 0)
        p.image->width = p.viewWidth;
    if (p.image->height == 0)
        p.image->height = p.viewHeight;

    tx = -p.viewMinx;
    ty = -p.viewMiny;
    sx = p.viewWidth > 0 ? p.image->width / p.viewWidth : 0;
    sy = p.viewHeight > 0 ? p.image->height / p.viewHeight : 0;
    // Unit scaling
    us = 1.0f / svg_convert_to_pixels(p, nsvg__coord(1.0f, nsvg__parseUnits(units)), 0.0f, 1.0f);

    // Fix aspect ratio
    if (p.alignType == NSVG_ALIGN_MEET) {
        // fit whole image into viewbox
        sx = sy = nsvg__minf(sx, sy);
        tx += nsvg__viewAlign(p.viewWidth * sx, p.image->width, p.alignX) / sx;
        ty += nsvg__viewAlign(p.viewHeight * sy, p.image->height, p.alignY) / sy;
    } else if (p.alignType == NSVG_ALIGN_SLICE) {
        // fill whole viewbox with image
        sx = sy = nsvg__maxf(sx, sy);
        tx += nsvg__viewAlign(p.viewWidth * sx, p.image->width, p.alignX) / sx;
        ty += nsvg__viewAlign(p.viewHeight * sy, p.image->height, p.alignY) / sy;
    }

    // Transform
    sx *= us;
    sy *= us;
    avgs = (sx + sy) / 2.0f;
    for (shape = p.image->shapes; shape != NULL; shape = shape->next) {
        shape->bounds[0] = (shape->bounds[0] + tx) * sx;
        shape->bounds[1] = (shape->bounds[1] + ty) * sy;
        shape->bounds[2] = (shape->bounds[2] + tx) * sx;
        shape->bounds[3] = (shape->bounds[3] + ty) * sy;
        for (path = shape->paths; path != NULL; path = path->next) {
            path->bounds[0] = (path->bounds[0] + tx) * sx;
            path->bounds[1] = (path->bounds[1] + ty) * sy;
            path->bounds[2] = (path->bounds[2] + tx) * sx;
            path->bounds[3] = (path->bounds[3] + ty) * sy;
            for (i = 0; i < path->npts; i++) {
                pt = &path->pts[i * 2];
                pt[0] = (pt[0] + tx) * sx;
                pt[1] = (pt[1] + ty) * sy;
            }
        }

        if (shape->fill.type == NSVG_PAINT_LINEAR_GRADIENT || shape->fill.type == NSVG_PAINT_RADIAL_GRADIENT) {
            nsvg__scaleGradient(shape->fill.gradient, tx, ty, sx, sy);
            memcpy(t, shape->fill.gradient->xform, sizeof(float) * 6);
            nsvg__xformInverse(shape->fill.gradient->xform, t);
        }
        if (shape->stroke.type == NSVG_PAINT_LINEAR_GRADIENT || shape->stroke.type == NSVG_PAINT_RADIAL_GRADIENT) {
            nsvg__scaleGradient(shape->stroke.gradient, tx, ty, sx, sy);
            memcpy(t, shape->stroke.gradient->xform, sizeof(float) * 6);
            nsvg__xformInverse(shape->stroke.gradient->xform, t);
        }

        shape->strokeWidth *= avgs;
        shape->strokeDashOffset *= avgs;
        for (i = 0; i < shape->strokeDashCount; i++)
            shape->strokeDashArray[i] *= avgs;
    }
}
static gfx_result svg_parse_gradient_elem(svg_parse_result& ctx, signed char type) {
    reader_t& s = *ctx.reader;
 
    NSVGgradientData* grad = (NSVGgradientData*)ctx.allocator(sizeof(NSVGgradientData));
    if (grad == NULL) return gfx_result::out_of_memory;
    ctx.image_size+=sizeof(NSVGgradientData);
    memset(grad, 0, sizeof(NSVGgradientData));
    grad->units = NSVG_OBJECT_SPACE;
    grad->type = type;
    if (grad->type == NSVG_PAINT_LINEAR_GRADIENT) {
        grad->linear.x1 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
        grad->linear.y1 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
        grad->linear.x2 = nsvg__coord(100.0f, NSVG_UNITS_PERCENT);
        grad->linear.y2 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
    } else if (grad->type == NSVG_PAINT_RADIAL_GRADIENT) {
        grad->radial.cx = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
        grad->radial.cy = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
        grad->radial.r = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
    }

    nsvg__xformIdentity(grad->xform);
    if (!s.read()) {
        return gfx_result::invalid_format;
    }
    while (s.node_type() == ml_node_type::attribute) {
        strncpy(ctx.lname, s.value(), sizeof(ctx.lname));
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
            strncpy(name, s.value(), sizeof(name));
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            strncpy(value, s.value(), sizeof(value));
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
                    nsvg__parseTransform(grad->xform, value);
                } else if (strcmp(name, "cx") == 0) {
                    grad->radial.cx = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "cy") == 0) {
                    grad->radial.cy = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "r") == 0) {
                    grad->radial.r = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "fx") == 0) {
                    grad->radial.fx = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "fy") == 0) {
                    grad->radial.fy = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "x1") == 0) {
                    grad->linear.x1 = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "y1") == 0) {
                    grad->linear.y1 = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "x2") == 0) {
                    grad->linear.x2 = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "y2") == 0) {
                    grad->linear.y2 = nsvg__parseCoordinateRaw(value);
                } else if (strcmp(name, "spreadMethod") == 0) {
                    if (strcmp(value, "pad") == 0)
                        grad->spread = NSVG_SPREAD_PAD;
                    else if (strcmp(value, "reflect") == 0)
                        grad->spread = NSVG_SPREAD_REFLECT;
                    else if (strcmp(value, "repeat") == 0)
                        grad->spread = NSVG_SPREAD_REPEAT;
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
    NSVGgradientStop* stop;
    int i, idx;

    curAttr->stopOffset = 0;
    curAttr->stopColor = 0;
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
    grad->stops = (NSVGgradientStop*)p.reallocator(grad->stops, sizeof(NSVGgradientStop) * grad->nstops);
    if (grad->stops == NULL) return gfx_result::out_of_memory;
    p.image_size+=sizeof(NSVGgradientStop);
    
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
    stop->color |= (unsigned int)(curAttr->stopOpacity * 255) << 24;
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
        p.image_size+=(2*sizeof(float));
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
    NSVGpath* path = NULL;
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

    path = (NSVGpath*)p.allocator(sizeof(NSVGpath));
    p.image_size+=sizeof(NSVGpath);
    if (path == NULL) goto error;
    memset(path, 0, sizeof(NSVGpath));
    pts_sz = p.npts * 2 * sizeof(float);
    path->pts = (float*)p.allocator(pts_sz);
    if (path->pts == NULL) goto error;
    p.image_size+=pts_sz;
    path->closed = closed;
    path->npts = p.npts;

    // Transform path.
    for (i = 0; i < p.npts; ++i)
        nsvg__xformPoint(&path->pts[i * 2], &path->pts[i * 2 + 1], p.pts[i * 2], p.pts[i * 2 + 1], attr->xform);

    // Find bounds
    for (i = 0; i < path->npts - 1; i += 3) {
        curve = &path->pts[i * 2];
        nsvg__curveBounds(bounds, curve);
        if (i == 0) {
            path->bounds[0] = bounds[0];
            path->bounds[1] = bounds[1];
            path->bounds[2] = bounds[2];
            path->bounds[3] = bounds[3];
        } else {
            path->bounds[0] = nsvg__minf(path->bounds[0], bounds[0]);
            path->bounds[1] = nsvg__minf(path->bounds[1], bounds[1]);
            path->bounds[2] = nsvg__maxf(path->bounds[2], bounds[2]);
            path->bounds[3] = nsvg__maxf(path->bounds[3], bounds[3]);
        }
    }

    path->next = p.plist;
    p.plist = path;

    return gfx_result::success;

error:
    if (path != NULL) {
        if (path->pts != NULL) p.deallocator(path->pts);
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
    d = nsvg__sqr(x1p) / nsvg__sqr(rx) + nsvg__sqr(y1p) / nsvg__sqr(ry);
    if (d > 1) {
        d = sqrtf(d);
        rx *= d;
        ry *= d;
    }
    // 2) Compute cx', cy'
    s = 0.0f;
    sa = nsvg__sqr(rx) * nsvg__sqr(ry) - nsvg__sqr(rx) * nsvg__sqr(y1p) - nsvg__sqr(ry) * nsvg__sqr(x1p);
    sb = nsvg__sqr(rx) * nsvg__sqr(y1p) + nsvg__sqr(ry) * nsvg__sqr(x1p);
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
    a1 = nsvg__vecang(1.0f, 0.0f, ux, uy);  // Initial angle
    da = nsvg__vecang(ux, uy, vx, vy);      // Delta angle

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
        nsvg__xformPoint(&x, &y, dx * rx, dy * ry, t);                       // position
        nsvg__xformVec(&tanx, &tany, -dy * rx * kappa, dx * ry * kappa, t);  // tangent
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
static gfx_result svg_add_shape(svg_parse_result& p) {
    NSVGattrib* attr = svg_get_attr(p);
    float scale = 1.0f;
    NSVGshape* shape;
    NSVGpath* path;
    int i;

    if (p.plist == NULL)
        return gfx_result::invalid_state;
    shape = (NSVGshape*)p.allocator(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    p.image_size+=sizeof(NSVGshape);
    memset(shape, 0, sizeof(NSVGshape));

    memcpy(shape->id, attr->id, sizeof shape->id);
    //printf("ADD SHAPE ID %s. attrHead is %d\n",shape->id,p.attrHead);
    memcpy(shape->fillGradient, attr->fillGradient, sizeof shape->fillGradient);
    memcpy(shape->strokeGradient, attr->strokeGradient, sizeof shape->strokeGradient);
    memcpy(shape->xform, attr->xform, sizeof shape->xform);
    scale = nsvg__getAverageScale(attr->xform);
    shape->strokeWidth = attr->strokeWidth * scale;
    shape->strokeDashOffset = attr->strokeDashOffset * scale;
    shape->strokeDashCount = (char)attr->strokeDashCount;
    for (i = 0; i < attr->strokeDashCount; i++)
        shape->strokeDashArray[i] = attr->strokeDashArray[i] * scale;
    shape->strokeLineJoin = attr->strokeLineJoin;
    shape->strokeLineCap = attr->strokeLineCap;
    shape->miterLimit = attr->miterLimit;
    shape->fillRule = attr->fillRule;
    shape->opacity = attr->opacity;

    shape->paths = p.plist;
    p.plist = NULL;

    // Calculate shape bounds
    shape->bounds[0] = shape->paths->bounds[0];
    shape->bounds[1] = shape->paths->bounds[1];
    shape->bounds[2] = shape->paths->bounds[2];
    shape->bounds[3] = shape->paths->bounds[3];
    for (path = shape->paths->next; path != NULL; path = path->next) {
        shape->bounds[0] = nsvg__minf(shape->bounds[0], path->bounds[0]);
        shape->bounds[1] = nsvg__minf(shape->bounds[1], path->bounds[1]);
        shape->bounds[2] = nsvg__maxf(shape->bounds[2], path->bounds[2]);
        shape->bounds[3] = nsvg__maxf(shape->bounds[3], path->bounds[3]);
    }

    // Set fill
    if (attr->hasFill == 0) {
        shape->fill.type = NSVG_PAINT_NONE;
    } else if (attr->hasFill == 1) {
        shape->fill.type = NSVG_PAINT_COLOR;
        shape->fill.color = attr->fillColor;
        shape->fill.color |= (unsigned int)(attr->fillOpacity * 255) << 24;
    } else if (attr->hasFill == 2) {
        shape->fill.type = NSVG_PAINT_UNDEF;
    }

    // Set stroke
    if (attr->hasStroke == 0) {
        shape->stroke.type = NSVG_PAINT_NONE;
    } else if (attr->hasStroke == 1) {
        shape->stroke.type = NSVG_PAINT_COLOR;
        shape->stroke.color = attr->strokeColor;
        shape->stroke.color |= (unsigned int)(attr->strokeOpacity * 255) << 24;
    } else if (attr->hasStroke == 2) {
        shape->stroke.type = NSVG_PAINT_UNDEF;
    }

    // Set flags
    shape->flags = (attr->visible ? NSVG_FLAGS_VISIBLE : 0x00);

    // Add to tail
    if (p.image->shapes == NULL)
        p.image->shapes = shape;
    else
        p.shapesTail->next = shape;
    p.shapesTail = shape;

    return gfx_result::success;

error:
    if (shape) p.deallocator(shape);
    //printf("out of memory allocating shape\n");
    return gfx_result::out_of_memory;
}
static gfx_result svg_parse_rect_elem(svg_parse_result& p) {
    //printf("parse <rect>\n");
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
    //printf("parse <circle>\n");
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
    //printf("parse <ellipse>\n");
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
    //printf("parse <line>\n");
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
    //printf("parse <%s>\n", s.value());
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
                ss = nsvg__getNextPathItem(ss, item);
                args[nargs++] = (float)nsvg__atof(item);
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
            str = nsvg__getNextPathItemWhenArcFlag(str, item);
        if (!*item) {
            str = nsvg__getNextPathItem(str, item);
        }
        if (!*item) break;
        
        if (cmd != '\0' && nsvg__isCoordinate(item)) {
            if (nargs < 10) {
                args[nargs] = (float)nsvg__atof(item);
                ++nargs;
            }
            if (nargs >= rargs) {
                switch (cmd) {
                    case 'm':
                    case 'M':
                        res = svg_path_move_to(p, &cpx, &cpy, args, cmd == 'm' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("move to error\n");
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
                            //printf("line to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'H':
                    case 'h':
                        res = svg_path_hline_to(p, &cpx, &cpy, args, cmd == 'h' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("hline to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'V':
                    case 'v':
                        res = svg_path_vline_to(p, &cpx, &cpy, args, cmd == 'v' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("vline to error\n");
                            return res;
                        }
                        cpx2 = cpx;
                        cpy2 = cpy;
                        break;
                    case 'C':
                    case 'c':
                        res = svg_path_cubic_bez_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'c' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("path cubic bez to error\n");
                            return res;
                        }
                        break;
                    case 'S':
                    case 's':
                        res = svg_path_cubic_bez_short_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 's' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("path cubic bez to short error\n");
                            return res;
                        }
                        break;
                    case 'Q':
                    case 'q':
                        res = svg_path_quad_bez_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'q' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("path quad bez to error\n");
                            return res;
                        }
                        break;
                    case 'T':
                    case 't':
                        res = svg_path_quad_bez_short_to(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 't' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("path quad bez to short error\n");
                            return res;
                        }
                        break;
                    case 'A':
                    case 'a':
                        res = svg_path_arc_to(p, &cpx, &cpy, args, cmd == 'a' ? 1 : 0);
                        if (gfx_result::success != res) {
                            //printf("path arc to error\n");
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
                        //printf("add path error\n");
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
                            //printf("add path error\n");
                            return res;
                        }
                    }
                }
                // Start new subpath.
                svg_reset_path(p);
                res = svg_move_to(p, cpx, cpy);
                if (gfx_result::success != res) {
                    //printf("move to error\n");
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
            //printf("add path error\n");
            return res;
        }
    }
    if (p.plist != nullptr) {
        //printf("path add_shape\n");
        res = svg_add_shape(p);
        if (gfx_result::success != res) {
            //printf("add shape error\n");
            return res;
        }
        return res;
    }
    return gfx_result::success;
}
static gfx_result svg_parse_path_elem(svg_parse_result& ctx) {
    //printf("parse <path>\n");
    gfx_result res;
    reader_t& s = *ctx.reader;
    if (!s.read() || s.node_type() != ml_node_type::attribute) {
        return gfx_result::invalid_format;
    }
    ctx.style_val[0]='\0';
    ctx.class_val[0]='\0';
    
    bool should_parse_path = false;
    while (s.node_type() == ml_node_type::attribute) {
        if (0 == strcmp("d", s.value())) {
            if(!s.read() || s.node_type()!=ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            should_parse_path = true;
            size_t cdsz = strlen(s.value())+1;
            if(ctx.d==nullptr) {
                ctx.d = (char*)ctx.allocator(cdsz);
                if(ctx.d==nullptr) {
                    return gfx_result::out_of_memory;
                }
                ctx.d_size = cdsz;
            } else if(ctx.d_size< cdsz) {
                char* tmp = (char*)ctx.reallocator(ctx.d,cdsz);
                if(tmp==nullptr) {
                    return gfx_result::out_of_memory;
                }
                ctx.d = tmp;
                ctx.d_size=cdsz;
            }
            strcpy(ctx.d,s.value());
            while (s.read() && s.node_type() == ml_node_type::attribute_content) {
                cdsz+=strlen(s.value());
                if(ctx.d_size<cdsz) {
                    char* tmp = (char*)ctx.reallocator(ctx.d,cdsz);
                    if(tmp==nullptr) {
                        return gfx_result::out_of_memory;
                    }
                    ctx.d = tmp;
                    ctx.d_size=cdsz;
                }
                strcat(ctx.d,s.value());
            }
            while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
            ;   
        } else if(0 == strcmp("style", s.value())) {
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
            strncpy(ctx.style_val,s.value(),sizeof(ctx.style_val));
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
        } else if(0 == strcmp("class", s.value())) {
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
            strncpy(ctx.class_val,s.value(),sizeof(ctx.class_val));
            if(!s.read()) {
                return gfx_result::invalid_format;
            }
        } else {
            svg_parse_attr(ctx);
        }
        while ((s.node_type() == ml_node_type::attribute_content || s.node_type() == ml_node_type::attribute_end) && s.read())
            ;
    }
    if(*ctx.class_val) {
        res = svg_apply_classes(ctx,ctx.class_val);
        if (res != gfx_result::success) {
            return res;
        }
    }
    if(*ctx.style_val) {
        svg_parse_style(ctx,ctx.style_val);
    }
    if(should_parse_path) {
        res = svg_parse_path(ctx,ctx.d);
        if (res != gfx_result::success) {
            return res;
        }
    }
    return gfx_result::success;
}
static gfx_result svg_parse_css_selector(svg_parse_result&p,char** cur) {
    gfx_result res;
    char* tmp = nullptr;
    char* ss = *cur;
    svg_css_class* cls = nullptr;
    while(nsvg__isspace(*ss)) ++ss;
    if(*ss=='.') {
        cls = (svg_css_class*)p.allocator(sizeof(svg_css_class));
        if(cls==nullptr) {
            res= gfx_result::out_of_memory;
            goto error;
        }
        int i = sizeof(cls->selector)-1;
        char* psz = cls->selector;
        while(i && *ss!='{') {
            if(!nsvg__isspace(*ss)) {
                *psz++=*ss;
                --i;
            }
            ++ss;
        }
        *psz='\0';
        while(*ss && *ss!='{') ++ss;
        if(!*ss) {
            res= gfx_result::invalid_format;
            goto error;
        }
        // skip '{'
        ++ss;
        size_t sz = 1;
        while(*ss && *ss!='}') {
            // skip whitespace
            while(*ss && nsvg__isspace(*ss)) ++ss;
            if(!*ss) {
                res= gfx_result::invalid_format;
                goto error;
            }
            char*se = ss;
            while(*se && *se!='}' && !nsvg__isspace(*se)) ++ se;
            if(!*se) {
                res= gfx_result::invalid_format;
                goto error;
            }
            size_t new_sz = sz+(se-ss);
            if(tmp==nullptr) {
                tmp = (char*)p.allocator(new_sz+1);
            } else {
                tmp = (char*)p.reallocator(tmp,new_sz+1);
            }
            if(tmp==nullptr) {
                res = gfx_result::out_of_memory;
                goto error;
            }
            memcpy(tmp+sz-1,ss,se-ss);
            tmp[sz+(se-ss)-1]='\0';
            sz = new_sz;
            ss+=(se-ss+1);
            if(*se=='}') break;
        }
        cls->value = tmp;
        cls->next = nullptr;
        if(p.css_classes==nullptr) {
            p.css_classes = cls;
        } else {
            p.css_class_tail->next =cls;
        }
        p.css_class_tail = cls;
        *cur = ss;
        return gfx_result::success;
    } 
    // not supported. Skip this selector
    while(*ss && *ss!='}') ++ss;
    if(!*ss) {
        res= gfx_result::invalid_format;
        goto error;
    }
    if(!*ss) {
        *cur = ss;
        return gfx_result::canceled;
    }
    *cur = ss;
    return gfx_result::success;
error:
    if(cls!=nullptr) {
        p.deallocator(cls);
    }
    if(tmp!=nullptr) {
        p.deallocator(tmp);
    }
    return res;
}
static gfx_result svg_parse_style_elem(svg_parse_result&p) {
    //printf("parse <style>\n");
    reader_t& s = *p.reader;
    char* content = nullptr;
    size_t content_size = 0;
    while(s.read() && s.node_type()!=ml_node_type::element_end) {
        if(s.node_type()==ml_node_type::content) {
            if(content_size==0) {
                content_size = 1;
            }
            content_size += strlen(s.value());
            if(content==nullptr) {
                content = (char*)p.allocator(content_size);
                if(content==nullptr) {
                    return gfx_result::out_of_memory;
                }
                *content = '\0';
            } else {
                content = (char*)p.reallocator(content,content_size);
                if(content==nullptr) {
                    return gfx_result::out_of_memory;
                }
            }
            strcat(content,s.value());
        }
    }
    char* ss = content;
    gfx_result res= gfx_result::success;
    while(res==gfx_result::success) {
        res = svg_parse_css_selector(p,&ss);
        if(res!=gfx_result::success) {
            p.deallocator(content);
            return res;
        }
    }
    p.deallocator(content);
    return gfx_result::success;
}
static gfx_result svg_parse_svg_elem(svg_parse_result& ctx) {
    //printf("parse <svg>\n");
    reader_t& s = *ctx.reader;
    while (s.read() && s.node_type() == ml_node_type::attribute) {
        if (0 == strcmp("width", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            ctx.image->width = svg_parse_coordinate(ctx, s.value(), 0.0f, 0.0f);
        } else if (0 == strcmp("height", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            ctx.image->height = svg_parse_coordinate(ctx, s.value(), 0.0f, 0.0f);
        } else if (0 == strcmp("viewBox", s.value())) {
            if (!s.read() || s.node_type() != ml_node_type::attribute_content) {
                return gfx_result::invalid_format;
            }
            const char* ss = s.value();
            char buf[64];
            ss = nsvg__parseNumber(ss, buf, 64);
            ctx.viewMinx = nsvg__atof(buf);
            while (*ss && (nsvg__isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = nsvg__parseNumber(ss, buf, 64);
            ctx.viewMiny = nsvg__atof(buf);
            while (*ss && (nsvg__isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = nsvg__parseNumber(ss, buf, 64);
            ctx.viewWidth = nsvg__atof(buf);
            while (*ss && (nsvg__isspace(*ss) || *ss == '%' || *ss == ',')) ss++;
            if (!*ss) return gfx_result::invalid_format;
            ss = nsvg__parseNumber(ss, buf, 64);
            ctx.viewHeight = nsvg__atof(buf);
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

static void svg_parse_start_element(svg_parse_result& p)
{   
    reader_t& s = *p.reader;
    strncpy(p.lname,s.value(),sizeof(p.lname));
    //printf("parse <%s>\n",p.lname);
	if (p.defsFlag) {
		// Skip everything but gradients and styles in defs
		if (strcmp(s.value(), "linearGradient") == 0) {
			svg_parse_gradient_elem(p, NSVG_PAINT_LINEAR_GRADIENT);
		} else if (strcmp(s.value(), "radialGradient") == 0) {
			svg_parse_gradient_elem(p,  NSVG_PAINT_RADIAL_GRADIENT);
		} else if (strcmp(s.value(), "stop") == 0) {
			svg_parse_gradient_stop_elem(p);
		} else if(strcmp(s.value(),"style")==0) {
            svg_parse_style_elem(p);
        }
        //printf("%s in defs\n",s.value());
        return;
	} 
    
	if (strcmp(s.value(), "g") == 0) {
		//printf("parse <g>\n");
        svg_push_attr(p);
		svg_parse_attribs(p);
	} else if (strcmp(s.value(), "path") == 0) {
		if (p.pathFlag)	// Do not allow nested paths.
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
	} else if (strcmp(s.value(), "line") == 0)  {
		svg_push_attr(p);
		svg_parse_line_elem(p);
		svg_pop_attr(p);
	} else if (strcmp(s.value(), "polyline") == 0)  {
		svg_push_attr(p);
		svg_parse_poly_elem(p, 0);
		svg_pop_attr(p);
	} else if (strcmp(s.value(), "polygon") == 0)  {
		svg_push_attr(p);
		svg_parse_poly_elem(p,1);
		svg_pop_attr(p);
	} else  if (strcmp(s.value(), "linearGradient") == 0) {
		svg_parse_gradient_elem(p, NSVG_PAINT_LINEAR_GRADIENT);
	} else if (strcmp(s.value(), "radialGradient") == 0) {
		svg_parse_gradient_elem(p, NSVG_PAINT_RADIAL_GRADIENT);
	} else if (strcmp(s.value(), "stop") == 0) {
		svg_parse_gradient_stop_elem(p);
	} else if (strcmp(s.value(), "defs") == 0) {
		p.defsFlag = 1;
	} else if (strcmp(s.value(), "svg") == 0) {
		svg_parse_svg_elem(p);
	}
}

static void svg_parse_end_element(svg_parse_result& p)
{
    reader_t& s = *p.reader;
	if (strcmp(s.is_empty_element()?p.lname:s.value(), "g") == 0) {
		svg_pop_attr(p);
	} else if (strcmp(s.is_empty_element()?p.lname:s.value(), "path") == 0) {
		p.pathFlag = 0;
	} else if (strcmp(s.is_empty_element()?p.lname:s.value(), "defs") == 0) {
        //printf("end defs %d\n",(int)s.is_empty_element());
		p.defsFlag = 0;
	}
}

gfx_result svg_parse_document(reader_t& reader, svg_parse_result* result) {
    result->reader = &reader;
    while(reader.read()) {
        switch(reader.node_type()) {
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

void svg_doc::do_move(svg_doc& rhs) {
    m_doc_data = rhs.m_doc_data;
    rhs.m_doc_data = nullptr;
}
void svg_doc::do_free() {
    if (m_doc_data != nullptr) {
        nsvgDelete((NSVGimage*)m_doc_data);
        m_doc_data = nullptr;
    }
}
svg_doc::svg_doc() : m_doc_data(nullptr) {
}
svg_doc::~svg_doc() {
    do_free();
}
svg_doc::svg_doc(svg_doc&& rhs) {
    do_move(rhs);
}
svg_doc& svg_doc::operator=(svg_doc&& rhs) {
    do_free();
    do_move(rhs);
    return *this;
}
svg_doc::svg_doc(stream* svg_stream, uint16_t dpi, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    svg_doc tmp;
    if(gfx_result::success == svg_doc::read(svg_stream,&tmp,dpi,allocator,reallocator,deallocator)) {
        do_move(tmp);
    }
}
gfx_result svg_doc::read(stream* svg_stream, svg_doc* out_doc, uint16_t dpi, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    if (out_doc == nullptr || svg_stream == nullptr || !svg_stream->caps().read) {
        return gfx_result::invalid_argument;
    }
    reader_t reader(svg_stream);
    void* p = allocator(sizeof(svg_parse_result));
    if(p==nullptr) {
        return gfx_result::out_of_memory;
    }
    svg_parse_result& parse_res = *(svg_parse_result*)p;
    memset(&parse_res, 0, sizeof(svg_parse_result));
    parse_res.image_size = 0;
    parse_res.image = (NSVGimage*)allocator(sizeof(NSVGimage));
    if (parse_res.image == nullptr) {
        return gfx_result::out_of_memory;
    }
    parse_res.image_size+=sizeof(NSVGimage);
    memset(parse_res.image, 0, sizeof(NSVGimage));
    // Init style
    nsvg__xformIdentity(parse_res.attr[0].xform);
    memset(parse_res.attr[0].id, 0, sizeof(parse_res.attr[0].id));
    parse_res.attr[0].fillColor = NSVG_RGB(0, 0, 0);
    parse_res.attr[0].strokeColor = NSVG_RGB(0, 0, 0);
    parse_res.attr[0].opacity = 1;
    parse_res.attr[0].fillOpacity = 1;
    parse_res.attr[0].strokeOpacity = 1;
    parse_res.attr[0].stopOpacity = 1;
    parse_res.attr[0].strokeWidth = 1;
    parse_res.attr[0].strokeLineJoin = NSVG_JOIN_MITER;
    parse_res.attr[0].strokeLineCap = NSVG_CAP_BUTT;
    parse_res.attr[0].miterLimit = 4;
    parse_res.attr[0].fillRule = NSVG_FILLRULE_NONZERO;
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
    NSVGimage* img = parse_res.image;
    parse_res.image = NULL;
    out_doc->m_doc_data = (void*)img;
    //printf("image size: %d kb\n",parse_res.image_size/1024);
    svg_delete_parse_result(parse_res);

    return gfx_result::success;
}
#if 0
gfx_result svg_doc::read2(stream* svg_stream, svg_doc* out_doc, uint16_t dpi, void*(allocator)(size_t), void*(reallocator)(void*, size_t), void(deallocator)(void*)) {
    if (out_doc == nullptr || svg_stream == nullptr || !svg_stream->caps().read) {
        return gfx_result::invalid_argument;
    }
    NSVGparser* p = nsvg__createParser();
    if (p == nullptr) {
        return gfx_result::out_of_memory;
    }
    p->dpi = dpi;
    size_t len = (size_t)svg_stream->seek(0, seek_origin::end);
    char* tmp = (char*)malloc(len);
    if (tmp == nullptr) {
        return gfx_result::out_of_memory;
    }
    svg_stream->seek(0);
    svg_stream->read((uint8_t*)tmp, len);

    nsvg__parseXML(tmp, nsvg__startElement, nsvg__endElement, nsvg__content, p);

    // Create gradients after all definitions have been parsed
    nsvg__createGradients(p);

    // Scale to viewBox
    nsvg__scaleToViewbox(p, "px");

    out_doc->m_doc_data = p->image;
    //dump_parse_res(*p);
    p->image = NULL;

    nsvg__deleteParser(p);

    free(tmp);
    return gfx_result::success;
}
#endif
bool svg_doc::initialized() const {
    return m_doc_data != nullptr;
}
sizef svg_doc::dimensions() const {
    if(m_doc_data==nullptr) {
        return {0,0};
    }
    NSVGimage* img = (NSVGimage*)m_doc_data;
    return {img->width, img->height};
}
float svg_doc::scale(float line_height) const {
    if(m_doc_data==nullptr) {
        return NAN;
    }
    return line_height/dimensions().height;
}
float svg_doc::scale(sizef dimensions) const {
    if(m_doc_data==nullptr) {
        return NAN;
    }
    float rw = this->dimensions().width/dimensions.width;
    float rh = this->dimensions().height/dimensions.height;
    if(rw>=rh) {
        return 1.0/rw;
    }
    return 1.0/rh;
}
float svg_doc::scale(ssize16 dimensions) const {
    return scale(sizef(dimensions.width,dimensions.height));
}   
float svg_doc::scale(size16 dimensions) const {
    return scale(sizef(dimensions.width,dimensions.height));
}   
void svg_doc::draw(float scale, const srect16& rect, void(read_callback)(int x, int y, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* a, void* state), void* read_callback_state, void(write_callback)(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* state), void* write_callback_state, void*(allocator)(size_t),void*(reallocator)(void*,size_t),void(deallocator)(void*)) const {
    if (initialized()) {
        NSVGrasterizer* rasterizer = nsvgCreateRasterizer(allocator,reallocator,deallocator);
        nsvgRasterize(rasterizer, (NSVGimage*)m_doc_data, rect.x1, rect.y1, scale, nullptr, read_callback, read_callback_state, write_callback, write_callback_state, rect.width(), rect.height(), rect.width() * 4);
        nsvgDeleteRasterizer(rasterizer);
    }
}