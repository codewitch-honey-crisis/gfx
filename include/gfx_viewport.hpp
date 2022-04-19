#ifndef HTCW_GFX_VIEWPORT
#define HTCW_GFX_VIEWPORT
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"
#include "gfx_draw_helpers.hpp"
#include "gfx_palette.hpp"
#include "gfx_drawing.hpp"
#include <math.h>
namespace gfx {
    template<typename Destination> class viewport final {
        Destination& m_destination;
        constexpr bool clip(spoint16 point, point16* out_result) const {
            srect16 b = (srect16)m_destination.bounds();
            if(!b.intersects(point))
                return false;
            out_result->x = (typename point16::value_type)point.x;
            out_result->y = (typename point16::value_type)point.y;
            return true;
        }
        constexpr bool clip(srect16 rect, rect16* out_result) const {
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
        using type = viewport;
        // the type of the pixel used for the bitmap
        using pixel_type = typename Destination::pixel_type;
        //using palette_type = typename Destination::palette_type;
        using caps = gfx::gfx_caps< false,false,false,false,false,Destination::caps::read,false>;
        spoint16 m_offset;
        spoint16 m_center;
        float m_rotation;
        double m_ctheta;
        double m_stheta;
        viewport(Destination& destination) : m_destination(destination),m_offset(0,0), m_center(0,0), m_rotation(0.0) {
            rotation(0);
        }
        viewport(const type& rhs)=default;
        type& operator=(const type& rhs)=default;
        viewport(type&& rhs)=default;
        constexpr float rotation() const {
            return m_rotation;
        }
        constexpr void rotation(float value) {
            m_rotation = value;
            // PI is inconsistently declared!
            double rads = m_rotation * (3.1415926536 / 180.0);
            m_ctheta = cos(rads);
            m_stheta = sin(rads);
        }
        constexpr spoint16 center() const {
            return m_center;
        }
        constexpr void center(spoint16 value) {
            m_center = value;
        }
        constexpr spoint16 offset() const {
            return m_offset;
        }
        constexpr void offset(spoint16 value) {
            m_offset = value;
        }
        constexpr void translater(float& x,float& y) const {
            double rx = (m_ctheta * (x - (double)m_center.x) - m_stheta * (y - (double)m_center.y) + (double)m_center.x) + m_offset.x; 
            double ry = (m_stheta * (x - (double)m_center.x) + m_ctheta * (y - (double)m_center.y) + (double)m_center.y) + m_offset.y;
            x = (float)rx;
            y = (float)ry;
        }
        
        constexpr void translate_inplace(spoint16& point) const {
            float x = point.x;
            float y = point.y;
            translater(x,y);
            point.x = (typename spoint16::value_type)x;
            point.y = (typename spoint16::value_type)y;
        }
        constexpr spoint16 translate(spoint16 point) const {
            translate_inplace(point);
            return point;
        }
        
        constexpr inline srect16 translate(const srect16& rect) const {
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
        gfx_result point(point16 location, pixel_type* out_pixel) const {
            spoint16 loc = (spoint16)location;
            translate_inplace(loc);
            if(!clip(loc, &location)) {
                *out_pixel = pixel_type();
                return gfx_result::success;
            }
            return m_destination.point(location,out_pixel);
        }
        gfx_result fill(const rect16& dst,pixel_type pixel) {
            if((m_offset.x==0 &&m_offset.y==0) && ((m_rotation==0 && m_center.x==0 && m_center.y==0) || (dst.x1==dst.x2&&dst.y1==dst.y2))) {
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
#endif // HTCW_GFX_VIEWPORT