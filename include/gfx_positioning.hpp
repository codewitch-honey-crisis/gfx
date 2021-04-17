#ifndef HTCW_GFX_POSITIONING_HPP
#define HTCW_GFX_POSITIONING_HPP
#include "bits.hpp"
namespace gfx {
    PACK
    // represents a pointx with 16-bit integer coordinates
    template <typename T>
    struct PACKED pointx {
        using type = pointx<T>;
        using value_type = T;
        // the x coordinate
        T x;
        // the y coordinate
        T y;
        // constructs a new instance
        inline pointx() {}
        // constructs a new instance with the specified coordinates
        constexpr inline pointx(T x, T y) : x(x), y(y) {
        }
    };
    // represents a size with 16-bit integer coordinates
    template <typename T>
    struct PACKED sizex {
        using type = sizex<T>;
        using value_type = T;
        // the width
        T width;
        // the height
        T height;
        // constructs a new instance
        inline sizex() {}
        // constructs a new instance with the specified width and height
        constexpr inline sizex(T width, T height) : width(width), height(height) {
        }
    };
    // represents a rectangle with 16-bit integer coordinates
    template <typename T>
    struct PACKED rectx
    {
        using type =rectx<T>;
        using value_type = T;
        // the x1 coordinate
        T x1;
        // the y1 coordinate
        T y1;
        // the x2 coordinate
        T x2;
        // the y2 coordinate
        T y2;
        // constructs a new instance
        inline rectx() {}
        // constructs a new instance with the specified coordinates
        constexpr inline rectx(T x1, T y1, T x2, T y2) : x1(x1), y1(y1), x2(x2), y2(y2) {
        }
        // constructs a new instance with the specified location and size
        constexpr inline rectx(pointx<T> location, sizex<T> size) : x1(location.x), y1(location.y), x2(location.x + size.width - 1), y2(location.y + size.height - 1) {
        }
        // indicates the leftmost position
        constexpr inline T left() const {
            return (x1 <= x2) ? x1 : x2;
        }
        // indicates the rightmost position
        constexpr inline T right() const {
            return (x1 > x2) ? x1 : x2;
        }
        // indicates the topmost position
        constexpr inline T top() const {
            return (y1 <= y2) ? y1 : y2;
        }
        // indicates the bottommost position
        constexpr inline T bottom() const {
            return (y1 > y2) ? y1 : y2;
        }
        // indicates the width
        constexpr inline T width() const {
            return x2>=x1?x2-x1+1:x1-x2+1;
            
        }
        // indicates the height
        constexpr inline T height() const {
            return y2>=y1?y2-y1+1:y1-y2+1;
        }
        // indicates the location
        constexpr inline pointx<T> location() const {
            return pointx<T>(left(),top());
        }
        // indicates the size
        constexpr inline sizex<T> dimensions() const
        {
            return sizex<T>(width(),height());
        }
        // indicates whether or not the specified pointx intersects with the rectangle
        constexpr inline bool intersects(pointx<T> pointx) const {
            return pointx.x>=left() && 
                    pointx.x<=right() &&
                    pointx.y>=top() &&
                    pointx.y<=bottom();
        }
        // indicates whether or not the specified rectangle intersects with this rectangle
        constexpr bool intersects(const rectx<T>& rect) const {
            return rect.intersects(location()) || 
                rect.intersects(pointx(right(),bottom())) ||
                intersects(rect.location()) || 
                intersects(pointx(rect.right(),rect.bottom()));
        }
        // increases or decreases the x and y bounds by the specified amounts. The rectangle is anchored on the center, and the effective width and height increases or decreases by twice the value of x or y.
        constexpr rectx<T> inflate(typename bits::signedx<T> x,typename bits::signedx<T> y) const {
            pointx<T> pt = location();
            pt=pointx<T>((int)pt.x-x<0?0:pt.x-x,pt.y-y<0?1:pt.y-y);
            
            pointx<T> pt2 = pointx<T>(right(),bottom());
            pt2=pointx<T>((int)pt2.x+x<0?0:pt2.x+x,pt2.y+y<0?1:pt2.y+y);
            
            return rectx<T>(pt.x,pt.y,pt2.x,pt2.y);
        }
        // normalizes a rectangle, such that x1<=x2 and y1<=y2
        constexpr inline rectx<T> normalize() const {
            return rectx<T>(location(),dimensions());
        }
        // crops a copy of the rectangle by bounds
        constexpr rectx<T> crop(const rectx<T>& bounds) const {
            return rectx<T>(
                    left()<bounds.left()?bounds.left():left(),
                    top()<bounds.top()?bounds.top():top(),
                    right()>bounds.right()?bounds.right():right(),
                    bottom()>bounds.bottom()?bounds.bottom():bottom()
                );
                
        }
        // splits a rectangle by another rectangle, returning between 0 and 4 rectangles as a result
        constexpr size_t split(rectx<T>& split_rect,size_t out_count, rectx<T>* out_rects) const {
            if(0==out_count || !intersects(split_rect)) return 0;
            size_t result = 0;
            if(split_rect.top()>top()) {
                *(out_rects++)=rectx<T>(left(),top(),right(),split_rect.top()-1);
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.left()>left()) {
                *(out_rects++)=rectx<T>(left(),split_rect.top(),split_rect.left()-1,split_rect.bottom());
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.right()>right()) {
                *(out_rects++)=rectx<T>(split_rect.right(),split_rect.top(),right(),split_rect.bottom());
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.bottom()<bottom()) {
                *(out_rects++)=rectx<T>(left(),split_rect.bottom()+1,right(),bottom());
                ++result;
            }
            return result;
        }
    };
    RESTORE_PACK
    
    using spoint16 = pointx<int16_t>;
    using ssize16 = sizex<int16_t>;
    using srect16 = rectx<int16_t>;

    using point16 = pointx<uint16_t>;
    using size16 = sizex<uint16_t>;
    using rect16 = rectx<uint16_t>;

    using pointf = pointx<float>;
    using sizef = sizex<float>;
    using rectf = rectx<float>;
}
#endif
