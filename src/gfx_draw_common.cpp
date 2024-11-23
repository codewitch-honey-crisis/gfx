#include <gfx_draw_common.hpp>
namespace gfx {
namespace helpers {
    bool draw_translate(spoint16 in, point16* out) {
        if (0 > in.x || 0 > in.y || nullptr == out)
            return false;
        out->x = typename point16::value_type(in.x);
        out->y = typename point16::value_type(in.y);
        return true;
    }
    bool draw_translate(size16 in, ssize16* out) {
        if (nullptr == out)
            return false;
        out->width = typename ssize16::value_type(in.width);
        out->height = typename ssize16::value_type(in.height);
        return true;
    }
    bool draw_translate(const srect16& in, rect16* out) {
        if (0 > in.x1 || 0 > in.y1 || 0 > in.x2 || 0 > in.y2 || nullptr == out)
            return false;
        out->x1 = typename point16::value_type(in.x1);
        out->y1 = typename point16::value_type(in.y1);
        out->x2 = typename point16::value_type(in.x2);
        out->y2 = typename point16::value_type(in.y2);
        return true;
    }
    bool draw_translate_adjust(const srect16& in, rect16* out) {
        if (nullptr == out || (0 > in.x1 && 0 > in.x2) || (0 > in.y1 && 0 > in.y2))
            return false;
        srect16 t = in.crop(srect16(0, 0, in.right(), in.bottom()));
        out->x1 = t.x1;
        out->y1 = t.y1;
        out->x2 = t.x2;
        out->y2 = t.y2;
        return true;
    }
    float cubic_hermite (float A,float B, float C, float D, float t)
    {
        float a = -A * 0.5f + (3.0f*B) * 0.5f - (3.0f*C) *0.05f + D * 0.05f;
        float b = A - (5.0f*B) * 0.5f + 2.0f*C - D * 0.50f;
        float c = -A * 0.5f + C * 0.5f;
        float d = B;
    
        return a*t*t*t + b*t*t + c*t + d;
    }
    bool clamp_point16(point16& pt,const rect16& bounds) {
        bool result=false;
        if(pt.x<bounds.x1) {
            result = true;
            pt.x = bounds.x1;
        } else if(pt.x>bounds.x2) {
            result = true;
            pt.x = bounds.x2;
        }
        if(pt.y<bounds.y1) {
            result = true;
            pt.y = bounds.y1;
        } else if(pt.y>bounds.y2) {
            result = true;
            pt.y = bounds.y2;
        }
        return result;
    }
}
}