#include <gfx_drawing.hpp>
namespace gfx {
    namespace helpers {
        double cubic_hermite (double A, double B, double C, double D, double t)
        {
            double a = -A / 2.0f + (3.0f*B) / 2.0f - (3.0f*C) / 2.0f + D / 2.0f;
            double b = A - (5.0f*B) / 2.0f + 2.0f*C - D / 2.0f;
            double c = -A / 2.0f + C / 2.0f;
            double d = B;
        
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