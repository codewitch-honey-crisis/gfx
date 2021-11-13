#ifndef HTCW_GFX_VIEW
#define HTCW_GFX_VIEW
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"
#include "gfx_draw_helpers.hpp"
#include "gfx_palette.hpp"
#include "gfx_drawing.hpp"
#include <math.h>
namespace gfx {
    template<typename Destination> class view final {
        Destination& m_destination;
        constexpr bool clip(spoint16 point, point16* out_result) {
            srect16 b = (srect16)m_destination.bounds();
            if(!b.intersects(point))
                return false;
            out_result->x = (typename point16::value_type)point.x;
            out_result->y = (typename point16::value_type)point.y;
            return true;
        }
        constexpr bool clip(srect16 rect, rect16* out_result) {
            srect16 b = (srect16)m_destination.bounds();
            if(!b.intersects(rect))
                return false;
            b = b.crop(rect);
            out_result->x1 = (typename rect16::value_type)b.x1;
            out_result->y1 = (typename rect16::value_type)b.y1;
            out_result->x2 = (typename rect16::value_type)b.x2;
            out_result->y2 = (typename rect16::value_type)b.y2;
            return true;
        }
    public:
        using type = view;
        // the type of the pixel used for the bitmap
        using pixel_type = typename Destination::pixel_type;
        using palette_type = typename Destination::palette_type;
        using caps = gfx::gfx_caps< false,false,false,false,false,Destination::caps::read,false>;
        spoint16 offset;
        spoint16 center;
        float rotation;

        view(Destination& destination) : m_destination(destination),offset(0,0), center(0,0), rotation(0.0) {

        }
        view(const type& rhs)=default;
        type& operator=(const type& rhs)=default;
        view(type&& rhs)=default;

        constexpr void translater(float& x,float& y) const {
            double rads = rotation * (M_PI / 180.0);
            double ctheta = cos(rads);
            double stheta = sin(rads);
            double rx = (ctheta * (x - (double)center.x) - stheta * (y - (double)center.y) + (double)center.x) + offset.x; 
            double ry = (stheta * (x - (double)center.x) + ctheta * (y - (double)center.y) + (double)center.y) + offset.y;
            x = (float)rx;
            y = (float)ry;
        }
        
        constexpr void translate_inplace(spoint16& point) {
            float x = point.x;
            float y = point.y;
            translater(x,y);
            point.x = (typename spoint16::value_type)x;
            point.y = (typename spoint16::value_type)y;
        }
        constexpr spoint16 translate(spoint16 point) {
            translate_inplace(point);
            return point;
        }
        
        constexpr inline srect16 translate(const srect16& rect) {
            return srect16(translate(rect.point1()),translate(rect.point2()));
        }
        gfx_result point(point16 location,pixel_type rhs) {
            spoint16 loc = (spoint16)location;
            translate_inplace(loc);
            if(!clip(loc, &location)) {
                return gfx_result::success;
            }
            return m_destination.point(location,rhs);
        }
        gfx_result point(point16 location, pixel_type* out_pixel) {
            spoint16 loc = (spoint16)location;
            translate_inplace(loc);
            if(!clip(loc, &location)) {
                *out_pixel = pixel_type();
                return gfx_result::success;
            }
            return m_destination.point(location,out_pixel);
        }
        gfx_result fill(const rect16& dst,pixel_type pixel) {
            if((rotation==0 && center.x==0 && center.y==0) || (dst.x1==dst.x2&&dst.y1==dst.y2)) {
                return m_destination.fill(dst,pixel);
            }
            spoint16 points[4];
            points[0] = translate(spoint16(dst.x1,dst.y1));
            points[1] = translate(spoint16(dst.x2,dst.y1));
            points[2] = translate(spoint16(dst.x2,dst.y2));
            points[3] = translate(spoint16(dst.x1,dst.y2));
            return draw::filled_polygon(m_destination,spath16(4,points),pixel);
        }
        gfx_result clear(const rect16& dst) {
            return fill(dst,pixel_type());
        }
        // indicates the dimensions of the bitmap
        inline size16 dimensions() const {
            return m_destination.dimensions();
        }
        // provides a bounding rectangle for the bitmap, starting at 0,0
        inline rect16 bounds() const {
            return m_destination.bounds();
        }
    };
}
#endif // HTCW_GFX_VIEW