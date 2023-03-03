#ifndef HTCW_GFX_POSITIONING_HPP
#define HTCW_GFX_POSITIONING_HPP
#include "gfx_core.hpp"
#include <htcw_bits.hpp>
namespace gfx {
    // represents a pointx with integer coordinates
    template <typename T>
    struct pointx final {
        using type = pointx;
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
        constexpr inline explicit operator pointx<bits::signedx<value_type>>() const {
            return pointx<bits::signedx<value_type>>(bits::signedx<value_type>(x),bits::signedx<value_type>(y));
        }
        constexpr inline explicit operator pointx<bits::unsignedx<value_type>>() const {
            return pointx<bits::unsignedx<value_type>>(bits::unsignedx<value_type>(x),bits::unsignedx<value_type>(y));
        }
        // offsets the point by the specified amounts.
        constexpr inline pointx offset(bits::signedx<value_type> x, bits::signedx<value_type> y) const {
            return type(this->x+x,this->y+y);
        }
        // offsets in-place the point by the specified amounts.
        constexpr inline void offset_inplace(bits::signedx<value_type> x, bits::signedx<value_type> y) {
            this->x+=x;
            this->y+=y;
        }
        // offsets the point by the specified amounts.
        constexpr inline pointx offset(pointx<bits::signedx<value_type>> adjust) const {
            return offset(adjust.x,adjust.y);
        }
        // offsets in-place the point by the specified amounts.
        constexpr inline void offset_inplace(pointx<bits::signedx<value_type>> adjust) {
            offset_inplace(adjust.x,adjust.y);
        }
        constexpr inline bool operator==(const type& rhs) const { 
            return x==rhs.x && y==rhs.y;   
        }
        constexpr static const inline pointx min() { return { bits::num_metrics<value_type>::min,bits::num_metrics<value_type>::min }; }
        constexpr static const inline pointx zero() { return { bits::num_metrics<value_type>::zero,bits::num_metrics<value_type>::zero }; }
        constexpr static const inline pointx max() { return { bits::num_metrics<value_type>::max,bits::num_metrics<value_type>::max }; }
    };
    template <typename T>
    struct rectx;
    // represents a size with integer coordinates
    template <typename T>
    struct sizex final {
        using type = sizex;
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
        // increases or decreases the width and height by the specified amounts.
        constexpr sizex inflate(typename bits::signedx<T> width,typename bits::signedx<T> height) const {
            return sizex(width+this->width,height+this->height);
        }
        constexpr inline rectx<T> bounds() const;
        constexpr explicit operator sizex<bits::signedx<value_type>>() const {
            return sizex<bits::signedx<value_type>>(bits::signedx<value_type>(width),bits::signedx<value_type>(height));
        }
        constexpr explicit operator sizex<bits::unsignedx<value_type>>() const {
            return sizex<bits::unsignedx<value_type>>(bits::unsignedx<value_type>(width),bits::unsignedx<value_type>(height));
        }
        constexpr inline bool operator==(const sizex& rhs) const { 
            return width==rhs.width && height==rhs.height;   
        }
        constexpr static const inline sizex min() { return { bits::num_metrics<value_type>::min,bits::num_metrics<value_type>::min }; }
        constexpr static const inline sizex max() { return { bits::num_metrics<value_type>::max,bits::num_metrics<value_type>::max }; }

        inline float aspect_ratio() const {
            return (float)width/(float)height;
        }
    };
    enum struct rect_orientation {
        normalized = 0,
        denormalized = 1,
        flipped_horizontal = 2 | denormalized,
        flipped_vertical = 4 | denormalized
    };
    // represents a rectangle with integer coordinates
    template <typename T>
    struct rectx final
    {
        using type =rectx;
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
        inline rectx(const rectx& rhs)=default;
        inline rectx& operator=(const rectx& rhs)=default;
        inline rectx(rectx&& rhs)=default;
        inline rectx& operator=(rectx&& rhs)=default;
        // constructs a new instance with the specified coordinates
        constexpr inline rectx(T x1, T y1, T x2, T y2) : x1(x1), y1(y1), x2(x2), y2(y2) {
        }
        // constructs a new instance with the specified location and size
        constexpr inline rectx(pointx<T> location, sizex<T> size) : x1(location.x), y1(location.y), x2(location.x + size.width - 1), y2(location.y + size.height - 1) {
        }
        // constructs a new instance with the specified points
        constexpr inline rectx(pointx<T> point1, pointx<T> point2) : x1(point1.x), y1(point1.y), x2(point2.x), y2(point2.y) {
        }
        // constructs a new instance with the specified center and distance from the center to a side. This is useful for constructing circles out of bounding rectangles.
        constexpr inline rectx(pointx<T> center, typename sizex<T>::value_type distance) : 
            x1(center.x - distance ), 
            y1(center.y - distance ), 
            x2(center.x + distance - 1), 
            y2(center.y + distance - 1) {
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
        constexpr inline pointx<T> point1() const {
            return pointx<T>(x1,y1);
        }
        constexpr inline pointx<T> point2() const {
            return pointx<T>(x2,y2);
        }
        // indicates the height
        constexpr inline T height() const {
            return y2>=y1?y2-y1+1:y1-y2+1;
        }
        
        constexpr inline pointx<T> top_left() const {
            return pointx<T>(left(),top());
        }
        constexpr inline pointx<T> top_right() const {
            return pointx<T>(right(),top());
        }
        constexpr inline pointx<T> bottom_left() const {
            return pointx<T>(left(),bottom());
        }
        constexpr inline pointx<T> bottom_right() const {
            return pointx<T>(right(),bottom());
        }
        // indicates the location
        constexpr inline pointx<T> location() const {
            return top_left();
        }
        // indicates the size
        constexpr inline sizex<T> dimensions() const
        {
            return sizex<T>(width(),height());
        }
        // indicates the delta used to move from x1 to x2 by 1 step
        constexpr inline value_type delta_x() {
            return x2>x1?-1:1;
        }
        // indicates the delta used to move from y1 to y2 by 1 step
        constexpr inline value_type delta_y() {
            return y2>y1?-1:1;
        }
        // indicates whether or not the specified pointx intersects with the rectangle
        constexpr inline bool intersects(pointx<T> pointx) const {
            return pointx.x>=left() && 
                    pointx.x<=right() &&
                    pointx.y>=top() &&
                    pointx.y<=bottom();
        }
        // indicates whether or not the specified rectangle intersects with this rectangle
        constexpr bool intersects(const rectx& rect) const {
            return (left() <= rect.right() && right()>= rect.left() &&
                top() <= rect.bottom() && bottom() >= rect.top() ); 
        }
        // increases or decreases the x and y bounds by the specified amounts. The rectangle is anchored on the center, and the effective width and height increases or decreases by twice the value of x or y.
        constexpr rectx inflate(typename bits::signedx<T> x,typename bits::signedx<T> y) const {
            switch((int)orientation()) {
                case (int)rect_orientation::flipped_horizontal:
                    return rectx(x1-x,y1-y,x2+x,y2+y);
                case (int)rect_orientation::flipped_vertical:
                    return rectx(x1-x,y1+y,x2+x,y2-y);
                case (int)((int)rect_orientation::flipped_vertical|(int)rect_orientation::flipped_horizontal):
                    return rectx(x1+x,y1+y,x2-x,y2-y);
                
            }
            return rectx<T>(x1-x,y1-y,x2+x,y2+y);
        }
        constexpr inline rectx inflate(sizex<bits::signedx<T>> adjust) {
            return inflate(adjust.width,adjust.height);
        }
        // increases or decreases the x and y bounds by the specified amounts. The rectangle is anchored on the center, and the effective width and height increases or decreases by twice the value of x or y.
        constexpr void inflate_inplace(typename bits::signedx<T> x,typename bits::signedx<T> y) {
            switch((int)orientation()) {
                case (int)rect_orientation::flipped_horizontal:
                    x1-=x;
                    y1=-y;
                    x2+=x;
                    y2+=y;
                    return;
                case (int)rect_orientation::flipped_vertical:
                    x1-=x;
                    y1+=y;
                    x2+=x;
                    y2-=y;
                    return;
                case (int)((int)rect_orientation::flipped_vertical|(int)rect_orientation::flipped_horizontal):
                    x1+=x;
                    y1+=y;
                    x2-=x;
                    y2-=y;
                    return;
                
            }
            x1-=x;
            y1-=y;
            x2+=x;
            y2+=y;
        }
        constexpr inline void inflate_inplace(sizex<bits::signedx<T>> adjust) {
            inflate_inplace(adjust.width,adjust.height);
        }
        // indicates the aspect ratio of the rectangle
        constexpr inline float aspect_ratio() {
            return width()/(float)height();
        }
        // offsets the rectangle by the specified amounts.
        constexpr inline rectx offset(typename bits::signedx<T> x,typename bits::signedx<T> y) const {
            return rectx(x1+x,y1+y,x2+x,y2+y);
        }
        // offsets in-place the rectangle by the specified amounts.
        constexpr inline void offset_inplace(typename bits::signedx<T> x,typename bits::signedx<T> y) {
            x1+=x;
            y1+=y;
            x2+=x;
            y2+=y;
        }
        // offsets the rectangle by the specified amounts.
        constexpr inline rectx offset(pointx<bits::signedx<value_type>> adjust) const {
            return offset(adjust.x,adjust.y);
        }
        // offsets in-place the rectangle by the specified amounts.
        constexpr inline void offset_inplace(pointx<bits::signedx<value_type>> adjust) {
            offset_inplace(adjust.x,adjust.y);
        }
        constexpr inline rectx center_horizontal(const rectx& bounds) const {
            return offset((bounds.width()-width())/2,0).offset(bounds.left(),bounds.top());
        }
        constexpr inline rectx center_vertical(const rectx& bounds) const {
            return offset(0,(bounds.height()-height())/2).offset(bounds.left(),bounds.top());
        }
        constexpr inline rectx center(const rectx& bounds) const {
            return offset((bounds.width()-width())/2,(bounds.height()-height())/2).offset(bounds.left(),bounds.top());
        }
        constexpr inline void center_horizontal_inplace(const rectx& bounds) {
            offset_inplace((bounds.width()-width())/2,0);
            offset_inplace(bounds.left(),bounds.top());
        }
        constexpr inline void center_vertical_inplace(const rectx& bounds) {
            offset_inplace(0,(bounds.height()-height())/2);
            offset_inplace(bounds.left(),bounds.top());
        }
        constexpr inline void center_inplace(const rectx& bounds) {
            offset_inplace((bounds.width()-width())/2,(bounds.height()-height())/2);
            offset_inplace(bounds.left(),bounds.top());
        }
        // normalizes a rectangle, such that x1<=x2 and y1<=y2
        constexpr inline rectx normalize() const {
            return rectx(location(),dimensions());
        }
        // normalizes a rectangle in-place, such that x1<=x2 and y1<=y2
        constexpr inline void normalize_inplace() {
            pointx<T> pt1 = this->point1();
            pointx<T> pt2 = this->point2();
            if(pt1.x>pt2.x) {
                x1=pt2.x;
                x2=pt1.x;
            } 
            if(pt1.y>pt2.y) {
                y1=pt2.y;
                y2=pt1.y;
            }
        }
        // automatically repositions the rectangle so that it is no less than (0,0)
        constexpr inline rectx clamp_top_left_to_at_least_zero() {
            rectx r = *this;
            r.normalize_inplace();
            if(r.x1<0) {
                if(r.y1<0) {
                    r.offset_inplace(-r.x1,-r.y1);
                } else {
                    r.offset_inplace(-r.x1,0);    
                }
            } else if(r.y1<0) {
                r.offset_inplace(0,-r.y1);
            }
            return r;
        }
        // indicates whether or not the rectangle is flipped along the horizontal or vertical axes.
        constexpr rect_orientation orientation() const {
            int result = (int)rect_orientation::normalized;
            if(x1>x2)
                result |= (int)rect_orientation::flipped_horizontal;
            if(y1>y2)
                result |= (int)rect_orientation::flipped_vertical;
            return (rect_orientation)result;
        }
        // crops a copy of the rectangle by bounds
        constexpr rectx crop(const rectx<T>& bounds) const {
            if(x1<=x2) {
                if(y1<=y2) {
                    return rectx(
                            x1<bounds.left()?bounds.left():x1,
                            y1<bounds.top()?bounds.top():y1,
                            x2>bounds.right()?bounds.right():x2,
                            y2>bounds.bottom()?bounds.bottom():y2
                        );            
                } else {
                    return rectx(
                            x1<bounds.left()?bounds.left():x1,
                            y2<bounds.top()?bounds.top():y2,
                            x2>bounds.right()?bounds.right():x2,
                            y1>bounds.bottom()?bounds.bottom():y1
                        );            
                }
            } else {
                if(y1<=y2) {
                    return rectx(
                            x2<bounds.left()?bounds.left():x2,
                            y1<bounds.top()?bounds.top():y1,
                            x1>bounds.right()?bounds.right():x1,
                            y2>bounds.bottom()?bounds.bottom():y2
                        );            
                } else {
                    return rectx(
                            x2<bounds.left()?bounds.left():x2,
                            y2<bounds.top()?bounds.top():y2,
                            x1>bounds.right()?bounds.right():x1,
                            y1>bounds.bottom()?bounds.bottom():y1
                        );            
                }
            }
        }
        // indicates if this rectangle entirely contains the other rectangle
        constexpr bool contains(const rectx& other) const {
            return intersects(other.point1()) &&
                intersects(other.point2());
        }
        constexpr inline rectx flip_horizontal() const {
            return rectx<T>(x2,y1,x1,y2);
        }
        constexpr inline rectx flip_vertical() const {
            return rectx<T>(x1,y2,x2,y1);
        }
        constexpr inline rectx flip_all() const {
            return rectx<T>(x2,y2,x1,y1);
        }
        // splits a rectangle by another rectangle, returning between 0 and 4 rectangles as a result
        constexpr size_t split(rectx& split_rect,size_t out_count, rectx* out_rects) const {
            if(0==out_count || !intersects(split_rect)) return 0;
            size_t result = 0;
            if(split_rect.top()>top()) {
                *(out_rects++)=rectx(left(),top(),right(),split_rect.top()-1);
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.left()>left()) {
                *(out_rects++)=rectx(left(),split_rect.top(),split_rect.left()-1,split_rect.bottom());
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.right()>right()) {
                *(out_rects++)=rectx(split_rect.right(),split_rect.top(),right(),split_rect.bottom());
                ++result;
            }
            if(result==out_count) return result;
            if(split_rect.bottom()<bottom()) {
                *(out_rects++)=rectx(left(),split_rect.bottom()+1,right(),bottom());
                ++result;
            }
            return result;
        }
        explicit operator rectx<bits::signedx<value_type>>() const {
            using t = bits::signedx<value_type>;
            return rectx<bits::signedx<value_type>>(t(x1),t(y1),t(x2),t(y2));
        }
        explicit operator rectx<bits::unsignedx<value_type>>() const {
            using t = bits::unsignedx<value_type>;
            return rectx<bits::unsignedx<value_type>>(t(x1),t(y1),t(x2),t(y2));
        }
        constexpr inline bool operator==(const rectx& rhs) const { 
            return x1==rhs.x1 && y1==rhs.y1 &&
                x2==rhs.x2 && y2==rhs.y2;
        }
    };
    
    template<typename T>
    constexpr rectx<T> sizex<T>::bounds() const {
        return rectx<T>(pointx<T>(0,0),*this);
    }
    // represents a path as a series of points
    template <typename T>
    struct pathx final
    {
        using type = pathx<T>;
        using value_type = T;
        using point_type = pointx<T>;
        class accessor final {
            friend pathx;
            point_type* const pt;
            constexpr inline accessor(point_type* pt) : pt(pt) {
            }
            accessor(const accessor& rhs)=delete;
            accessor& operator=(const accessor& rhs)=delete;
            constexpr inline accessor(accessor&& rhs)=default;
            constexpr inline accessor& operator=(accessor&& rhs)=default;
        public:
            constexpr operator point_type() const {
                return *pt;
            }
            constexpr accessor& operator=(const point_type& rhs) {
                *pt = rhs;
                return *this;
            }
        };
    private:
        const size_t m_size;
        point_type* const m_points;
    public:
        constexpr inline pathx(size_t size,point_type* points) : m_size(size),m_points(points) {

        }
        constexpr inline size_t size() const {
            return m_size;
        }
        constexpr inline point_type* begin() {
            return m_points;
        }
        constexpr inline const point_type* begin() const {
            return m_points;
        }
        constexpr inline const point_type* end() const {
            return m_points+m_size;
        }
        constexpr inline accessor operator[](size_t index) {
            return accessor(&m_points[index]);
        }
        constexpr inline const accessor operator[](size_t index) const {
            return accessor(&m_points[index]);
        }
        rectx<value_type> bounds() const {
            using rect_type = rectx<value_type>;
            if(0==m_size) return rect_type(point_type(0,0),sizex<value_type>(0,0));
            rect_type result(*m_points,*m_points);
            const point_type* path = m_points + 1;
            size_t path_size = m_size;
            while(--path_size) {
                if(path->x<result.x1) {
                    result.x1=path->x;
                } else if(path->x>result.x2) {
                    result.x2=path->x;
                }
                if(path->y<result.y1) {
                    result.y1=path->y;
                } else if(path->y>result.y2) {
                    result.y2=path->y;
                }
                ++path;
            }
            return result;
        }
        inline sizex<value_type> dimensions() const {
            if(0==m_size) return sizex<value_type>(0,0);
            return bounds().dimensions();
        }
        // performs an inplace offset on a path
        void offset_inplace(bits::signedx<value_type> x,bits::signedx<value_type> y) {
            for(size_t i = 0;i<m_size;++i) {
                m_points[i]=m_points[i].offset(x,y);
            }
        }
        // performs an inplace offset on a path
        inline void offset_inplace(pointx<bits::signedx<value_type>> adjust) {
            offset_inplace(adjust.x,adjust.y);
        }
        // indicates whether a point intersects this path or polygon
        bool intersects(point_type location,bool polygon = false) const {
            if(0==m_size) return false;
            size_t j = m_size - 1;
            bool c = false;
            for(size_t i =0; i<m_size;++i) {
                if((location.x == m_points[i].x) && (location.y == m_points[i].y)) {
                    // point is a corner
                    return true;
                }
                if ((m_points[i].y > location.y) != (m_points[j].y > location.y)) {
                    const int slope = (location.x-m_points[i].x)*(m_points[j].y-m_points[i].y)-(m_points[j].x-m_points[i].x)*(location.y-m_points[i].y);
                    if(0==slope) {
                        // point is on boundary
                        return true;
                    }
                    if ((slope < 0) != (m_points[j].y < m_points[i].y)) {
                        c = !c;
                    }
                }
                j = i;
            }
            return c && polygon;
        }

    };
    // represents a point with 16-bit signed integer coordinates
    using spoint16 = pointx<int16_t>;
    // represents a size with 16-bit signed integer values
    using ssize16 = sizex<int16_t>;
    // represents a rectangle with 16-bit signed integer coordinates
    using srect16 = rectx<int16_t>;
    // represents a path with 16-bit signed integer coordinates
    using spath16 = pathx<int16_t>;
    // represents a point with 16-bit unsigned integer coordinates
    using point16 = pointx<uint16_t>;
    // represents a size with 16-bit unsigned integer values
    using size16 = sizex<uint16_t>;
    // represents a rectangle with 16-bit unsigned integer coordinates
    using rect16 = rectx<uint16_t>;
    // represents a path with 16-bit unsigned integer coordinates
    using path16 = pathx<uint16_t>;
    
    using sizef = sizex<float>;
    using pointf = pointx<float>;
    using rectf = rectx<float>;
    using pathf = pathx<float>;
    
}
#endif
