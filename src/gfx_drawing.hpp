#ifndef HTCW_GFX_DRAWING_HPP
#define HTCW_GFX_DRAWING_HPP
#include "bits.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"
#include "gfx_font.hpp"
namespace gfx {
    enum struct bitmap_flags {
        crop = 0,
        resize = 1
    };
    // provides drawing primitives over a bitmap or compatible type 
    class draw {
        template<typename Destination,typename Source>
        static void draw_bitmap_impl(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect,bitmap_flags options,srect16* clip) {
            rect16 dr;
            if(!translate_adjust(dest_rect,&dr))
                return; // whole thing is offscreen
            // get the scale
            float sx = dest_rect.width()/(float)source_rect.width();
            float sy = dest_rect.height()/(float)source_rect.height();
            int o = (int)dest_rect.orientation();
            point16 loc(0,0);
            rect16 ccr;
            if(nullptr!=clip) {
                if(!translate_adjust(*clip,&ccr))
                    return; // clip rect is off screen
                dr = dr.crop(ccr);
            }
            dr = dr.normalize();
            size16 dim(dr.width(),dr.height());
            if(dim.width>source_rect.width())
                dim.width = source_rect.width();
            if(dim.height>source_rect.height())
                dim.height = source_rect.height();
            auto x2=dest_rect.left();
            auto y2=dest_rect.top();
            if(0>x2) {
                loc.x = -x2;
                if(nullptr==clip)
                    dim.width+=dest_rect.x1;
            }
            if(0>y2) {
                loc.y = -y2;
                if(nullptr==clip)
                    dim.height+=y2;
            }
            loc.x+=source_rect.top();
            loc.y+=source_rect.left();
            rect16 srcr = rect16(loc,dim);
            rect16 dstr(dr.x1,dr.y1,srcr.width()*sx+.5+dr.x1,srcr.height()*sy+.5+dr.y1);
            if(dstr.x2>dr.x2) dstr.x2=dr.x2;
            if(dstr.y2>dr.y2) dstr.y2=dr.y2;
            typename srect16::value_type ix,iy;
            if((int)rect_orientation::flipped_horizontal==(o&(int)rect_orientation::flipped_horizontal)) {
                dstr=dstr.flip_horizontal();
                ix=-1;
            } else
                ix=1;
            if((int)rect_orientation::flipped_vertical==(o&(int)rect_orientation::flipped_vertical)) {
                iy=-1;
                dstr=dstr.flip_vertical();
            } else
                iy=1;
            
            if((int)bitmap_flags::resize!=((int)options&(int)bitmap_flags::resize)) {
               // do cropping

               x2=srcr.x1;
               y2=srcr.y1;
               for(typename rect16::value_type y=dstr.y1;y!=dstr.y2+iy;y+=iy) {
                for(typename rect16::value_type x=dstr.x1;x!=dstr.x2+ix;x+=ix) {
                    typename Source::pixel_type px = source[point16(x2,y2)];
                    typename Destination::pixel_type px2=px.template convert<typename Destination::pixel_type>();
                    destination[point16(x,y)]=px2;
                    ++x2;
                    if(x2>srcr.x2)
                        break;
                }    
                x2=srcr.x1;
                ++y2;
                if(y2>srcr.y2)
                    break;
               }
               
            } else {
                // do resizing
                int o = (int)dest_rect.orientation();
                uint32_t x_ratio = (uint32_t)((dest_rect.width()<<16)/source_rect.width()) +1;
                uint32_t y_ratio = (uint32_t)((dest_rect.height()<<16)/source_rect.height()) +1;
                bool growX = dest_rect.width()>source_rect.width(),
                    growY = dest_rect.height()>source_rect.height();
                point16 p(-1,-1);
                point16 ps;
                srcr = source_rect.normalize();
                for(ps.y=srcr.y1;ps.y<srcr.y2;++ps.y) {
                    for(ps.x=srcr.x1;ps.x<srcr.x2;++ps.x) {
                        uint16_t px = ps.x;
                        uint16_t py = ps.y;
                        if((int)rect_orientation::flipped_horizontal==((int)rect_orientation::flipped_horizontal&o)) {
                            px=source_rect.width()-px-1;
                        }
                        if((int)rect_orientation::flipped_vertical==((int)rect_orientation::flipped_vertical&o)) {
                            py=source_rect.height()-py-1;
                        }
                        uint16_t ux(((px*x_ratio)>>16));
                        
                        uint16_t uy(((py*y_ratio)>>16));
                        ux+=dstr.left();
                        uy+=dstr.top();
                        if(ux!=p.x || uy!=p.y) {
                            typename Source::pixel_type px = source[ps];
                            p=point16(ux,uy);
                            typename Destination::pixel_type px2=px.template convert<typename Destination::pixel_type>();
                            if(!(growX || growY)) {
                                destination[p]= px2;
                            } else {
                                uint16_t w = x_ratio>>16,h=y_ratio>>16;
                                destination.fill(rect16(p,size16(w,h)),px2);
                            }
                        }
                        
                    }    
                    
                }
            }
        }
        template<typename Destination,typename Source>
        struct bmp_helper {
            inline static void draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect,bitmap_flags options,srect16* clip) {
                draw_bitmap_impl(destination,dest_rect,source,source_rect,options,clip);
            }
        };
        
        template<typename Destination>
        struct bmp_helper<Destination,Destination> {
            static void draw_bitmap(Destination& destination, const srect16& dest_rect, Destination& source, const rect16& source_rect,bitmap_flags options,srect16* clip) {
                // disqualify fast blting
                if(dest_rect.x1>dest_rect.x2 || 
                    dest_rect.y1>dest_rect.y2 || 
                    ((int)bitmap_flags::resize==((int)options&(int)bitmap_flags::resize)&&
                        (dest_rect.width()!=source_rect.width()||
                        dest_rect.height()!=source_rect.height())
                        )
                    ) {
                    draw_bitmap_impl(destination,dest_rect,source,source_rect,options,clip);
                    return;
                }
               
                rect16 dr;
                if(!translate_adjust(dest_rect,&dr))
                    return; // whole thing is offscreen
                point16 loc(0,0);
                rect16 ccr;
                if(nullptr!=clip) {
                    if(!translate_adjust(*clip,&ccr))
                        return; // clip rect is off screen
                    dr = dr.crop(ccr);
                }

                size16 dim(dr.width(),dr.height());
                if(dim.width>source_rect.width())
                    dim.width = source_rect.width();
                if(dim.height>source_rect.height())
                    dim.height = source_rect.height();
                if(0>dest_rect.x1) {
                    loc.x = -dest_rect.x1;
                    if(nullptr==clip)
                        dim.width+=dest_rect.x1;
                }
                if(0>dest_rect.y1) {
                    loc.y = -dest_rect.y1;
                    if(nullptr==clip)
                        dim.height+=dest_rect.y1;
                }
                loc.x+=source_rect.top();
                loc.y+=source_rect.left();
                rect16 r = rect16(loc,dim);
                
                // fast blt  
                source.blt(r,destination,dr.top_left());
                
            }
        };
        // Defining region codes
        constexpr static const int region_inside = 0; // 0000
        constexpr static const int region_left = 1; // 0001
        constexpr static const int region_right = 2; // 0010
        constexpr static const int region_bottom = 4; // 0100
        constexpr static const int region_top = 8; // 1000
        
        // Function to compute region code for a point(x, y)
        static int region_code(const srect16& rect,float x, float y)
        {
            // inside
            int code = region_inside;
        
            if (x < rect.x1) // to the left of rectangle
                code |= region_left;
            else if (x > rect.x2) // to the right of rectangle
                code |= region_right;
            if (y < rect.y1) // above the rectangle
                code |= region_top;
            else if (y > rect.y2) // below the rectangle
                code |= region_bottom;
        
            return code;
        }
        static bool line_clip(srect16* rect,const srect16* clip) {
            float x1=rect->x1,
                y1=rect->y1,
                x2=rect->x2,
                y2=rect->y2;
                

            // Compute region codes for P1, P2
            int code1 = region_code(*clip,x1, y1);
            int code2 = region_code(*clip,x2, y2);
          
            // Initialize line as outside the rectangular window
            bool accept = false;
        
            while (true) {
                if ((code1 == region_inside) && (code2 == region_inside)) {
                    // If both endpoints lie within rectangle
                    accept = true;
                    break;
                }
                else if (code1 & code2) {
                    // If both endpoints are outside rectangle,
                    // in same region
                    break;
                }
                else {
                    // Some segment of line lies within the
                    // rectangle
                    int code_out;
                    double x, y;
        
                    // At least one endpoint is outside the
                    // rectangle, pick it.
                    if (code1 != 0)
                        code_out = code1;
                    else
                        code_out = code2;
        
                    // Find intersection point;
                    // using formulas y = y1 + slope * (x - x1),
                    // x = x1 + (1 / slope) * (y - y1)
                    if (code_out & region_top) {
                        // point is above the rectangle
                        x = x1 + (x2 - x1) * (clip->y1 - y1) / (y2 - y1);
                        y = clip->y1;
                    } else if (code_out & region_bottom) {
                        // point is below the clip rectangle
                        x = x1 + (x2 - x1) * (clip->y2 - y1) / (y2 - y1);
                        y = clip->y2;
                    }
                    else if (code_out & region_right) {
                        // point is to the right of rectangle
                        y = y1 + (y2 - y1) * (clip->x2 - x1) / (x2 - x1);
                        x = clip->x2;
                    }
                    else if (code_out & region_left) {
                        // point is to the left of rectangle
                        y = y1 + (y2 - y1) * (clip->x1 - x1) / (x2 - x1);
                        x = clip->x1;
                    }
        
                    // Now intersection point x, y is found
                    // We replace point outside rectangle
                    // by intersection point
                    if (code_out == code1) {
                        x1 = x;
                        y1 = y;
                        code1 = region_code(*clip,x1, y1);
                    }
                    else {
                        x2 = x;
                        y2 = y;
                        code2 = region_code(*clip,x2, y2);
                    }
                }
            }
            if (accept) {
                rect->x1=x1+.5;
                rect->y1=y1+.5;
                rect->x2=x2+.5;
                rect->y2=y2+.5;
                return true;
            }
            return false;
        }

        static bool translate(spoint16 in,point16* out)  {
            if(0>in.x||0>in.y||nullptr==out)
                return false;
            out->x=typename point16::value_type(in.x);
            out->y=typename point16::value_type(in.y);
            return true;
        }
        static bool translate(size16 in,ssize16* out)  {
            if(nullptr==out)
                return false;
            out->width=typename ssize16::value_type(in.width);
            out->height=typename ssize16::value_type(in.height);
            return true;
        }
        static bool translate(const srect16& in,rect16* out)  {
            if(0>in.x1||0>in.y1||0>in.x2||0>in.y2||nullptr==out)
                return false;
            out->x1 = typename point16::value_type(in.x1);
            out->y1 = typename point16::value_type(in.y1);
            out->x2 = typename point16::value_type(in.x2);
            out->y2 = typename point16::value_type(in.y2);
            return true;
        }
        static bool translate_adjust(const srect16& in,rect16* out)  {
            if(nullptr==out || (0>in.x1&&0>in.x2)||(0>in.y1&&0>in.y2))
                return false;
            srect16 result = in.crop(srect16(0,0,in.right(),in.bottom()));
            out->x1=result.x1;
            out->y1=result.y1;
            out->x2=result.x2;
            out->y2=result.y2;
            return true;
        }
        template<typename Destination>
        static void ellipse_impl(Destination& destination, const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr,bool filled=false) {
            using int_type = typename srect16::value_type;
            int_type x_adj =(1-(rect.width()&1));
            int_type y_adj =(1-(rect.height()&1));
            int_type rx = rect.width()/2-x_adj;
            int_type ry = rect.height()/2-y_adj;
            if(0==rx)
                rx=1;
            if(0==ry)
                ry=1;
            int_type xc = rect.width()/2+rect.left()-x_adj;
            int_type yc = rect.height()/2+rect.top()-y_adj;
            float dx, dy, d1, d2, x, y;
            x = 0;
            y = ry;
        
            // Initial decision parameter of region 1
            d1 = (ry * ry)
                - (rx * rx * ry)
                + (0.25 * rx * rx);
            dx = 2 * ry * ry * x;
            dy = 2 * rx * rx * y;
        
            // For region 1
            while (dx < dy+y_adj) {
                if(filled) {
                    line(destination,srect16(-x + xc, y + yc+y_adj,x + xc+x_adj, y + yc+y_adj),color,clip);
                    line(destination,srect16(-x + xc, -y + yc,x + xc+x_adj, -y + yc),color,clip);
                } else {
                    // Print points based on 4-way symmetry
                    point(destination,spoint16(x + xc+x_adj, y + yc+y_adj),color,clip);
                    point(destination,spoint16(-x + xc, y + yc+y_adj),color,clip);
                    point(destination,spoint16(x + xc+x_adj, -y + yc),color,clip);
                    point(destination,spoint16(-x + xc, -y + yc),color,clip);
                }
                // Checking and updating value of
                // decision parameter based on algorithm
                if (d1 < 0) {
                    ++x;
                    dx = dx + (2 * ry * ry);
                    d1 = d1 + dx + (ry * ry);
                }
                else {
                    ++x;
                    --y;
                    dx = dx + (2 * ry * ry);
                    dy = dy - (2 * rx * rx);
                    d1 = d1 + dx - dy + (ry * ry);
                }
            }
        
            // Decision parameter of region 2
            d2 = ((ry * ry) * ((x + 0.5) * (x + 0.5)))
                + ((rx * rx) * ((y - 1) * (y - 1)))
                - (rx * rx * ry * ry);
            
            // Plotting points of region 2
            while (y >= 0) {
        
                // printing points based on 4-way symmetry
                if(filled) {
                    line(destination,srect16(-x + xc, y + yc+y_adj,x + xc+x_adj, y + yc+y_adj),color,clip);
                    line(destination,srect16(-x + xc, -y + yc,x + xc+x_adj, -y + yc),color,clip);
                } else {
                    point(destination,spoint16(x + xc+x_adj, y + yc+y_adj),color,clip);
                    point(destination,spoint16(-x + xc, y + yc+y_adj),color,clip);
                    point(destination,spoint16(x + xc+x_adj, -y + yc),color,clip);
                    point(destination,spoint16(-x + xc, -y + yc),color,clip);
                }
                    
                // Checking and updating parameter
                // value based on algorithm
                if (d2 > 0) {
                    --y;
                    dy = dy - (2 * rx * rx);
                    d2 = d2 + (rx * rx) - dy;
                }
                else {
                    --y;
                    ++x;
                    dx = dx + (2 * ry * ry);
                    dy = dy - (2 * rx * rx);
                    d2 = d2 + dx - dy + (rx * rx);
                }
            }
        }
        template<typename Destination>
        static void arc_impl(Destination& destination, const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr,bool filled=false) {
            using int_type = typename srect16::value_type;
            int_type x_adj =(1-(rect.width()&1));
            int_type y_adj =(1-(rect.height()&1));
            int orient;
            if(rect.y1<=rect.y2) {
                if(rect.x1<=rect.x2) {
                    orient=0;
                } else {
                    orient=2;
                }
            } else {
                if(rect.x1<=rect.x2) {
                    orient=1;
                } else {
                    orient=3;
                }
            }
            int_type rx = rect.width()-x_adj-1;
            int_type ry = rect.height()-1;
            int_type xc = rect.width()+rect.left()-x_adj-1;
            int_type yc = rect.height()+rect.top()-1;
            if(0==rx)
                rx=1;
            if(0==ry)
                ry=1;
            float dx, dy, d1, d2, x, y;
            x = 0;
            y = ry;
        
            // Initial decision parameter of region 1
            d1 = (ry * ry)
                - (rx * rx * ry)
                + (0.25 * rx * rx);
            dx = 2 * ry * ry * x;
            dy = 2 * rx * rx * y;
        
            // For region 1
            while (dx < dy+y_adj) {
                if(filled) {
                    switch(orient) {
                        case 0: //x1<=x2,y1<=y2
                            line(destination,srect16(-x + xc, -y + yc, xc+x_adj, -y + yc),color,clip);        
                            break;
                        case 1: //x1<=x2,y1>y2
                            line(destination,srect16(-x + xc, y + yc-ry, xc+x_adj, y + yc-ry),color,clip);
                            break;
                        case 2: //x1>x2,y1<=y2
                            line(destination,srect16( xc-rx, -y + yc,x + xc+x_adj-rx, -y + yc),color,clip);        
                            break;
                        case 3: //x1>x2,y1>y2
                            line(destination,srect16( xc-rx, y + yc-ry,x + xc-rx, y + yc-ry),color,clip);
                            break;
                    }
                    
                } else {
                    switch(orient) {
                        case 0: //x1<=x2,y1<=y2
                            point(destination,spoint16(-x + xc, -y + yc),color,clip);
                            if(x_adj && -y+yc==rect.y1) {
                                point(destination,rect.top_right(),color,clip);
                            }
                            
                            break;
                        case 1: //x1<=x2,y1>y2
                            point(destination,spoint16(-x + xc, y + yc-ry),color,clip);
                            if(x_adj && y+yc-ry==rect.y1) {
                                point(destination,rect.bottom_right(),color,clip);
                            }
                            break;
                        case 2: //x1>x2,y1<=y2
                            point(destination,spoint16(x + xc+x_adj-rx, -y + yc),color,clip);
                            if(x_adj && -y+yc==rect.y1) {
                                point(destination,rect.top_left(),color,clip);
                            }
                            break;
                        case 3: //x1>x2,y1>y2
                            point(destination,spoint16(x + xc+x_adj-rx, y + yc-ry),color,clip);
                            break;
                    }
                    
                    
                }
                // Checking and updating value of
                // decision parameter based on algorithm
                if (d1 < 0) {
                    ++x;
                    dx = dx + (2 * ry * ry);
                    d1 = d1 + dx + (ry * ry);
                }
                else {
                    ++x;
                    --y;
                    dx = dx + (2 * ry * ry);
                    dy = dy - (2 * rx * rx);
                    d1 = d1 + dx - dy + (ry * ry);
                }
            }
        
            // Decision parameter of region 2
            d2 = ((ry * ry) * ((x + 0.5) * (x + 0.5)))
                + ((rx * rx) * ((y - 1) * (y - 1)))
                - (rx * rx * ry * ry);
            
            // Plotting points of region 2
            while (y >= 0) {
        
                // printing points based on 4-way symmetry
                if(filled) {
                    switch(orient) {
                        case 0: //x1<=x2,y1<=y2
                            line(destination,srect16(-x + xc, -y + yc,xc+x_adj, -y + yc),color,clip);
                            break;
                        case 1: //x1<=x2,y1>y2
                            line(destination,srect16(-x + xc, y + yc-ry,xc+x_adj, y + yc-ry),color,clip);
                            break;
                        case 2: //x1>x2,y1<=y2
                            line(destination,srect16( xc-rx, -y + yc,x + xc+x_adj-rx, -y + yc),color,clip);
                            break;
                        case 3: //x1>x2,y1>y2
                            line(destination,srect16(xc-rx, y + yc-ry,x + xc+x_adj-rx, y + yc-ry),color,clip);
                            break;
                    }
                            
                } else {
                    switch(orient) {
                        case 0: //x1<=x2,y1<=y2
                            point(destination,spoint16(-x + xc, -y + yc),color,clip);
                            break;
                        case 1: //x1<=x2,y1>y2
                            point(destination,spoint16(-x + xc, y + yc-ry),color,clip);
                            break;
                        case 2: //x1>x2,y1<=y2
                            point(destination,spoint16(x + xc+x_adj-rx, -y + yc),color,clip);
                            break;
                        case 3: //x1>x2,y1>y2
                            point(destination,spoint16(x + xc+x_adj-rx, y + yc-ry),color,clip);
                            break;
                    }
                            
                }
                    
                // Checking and updating parameter
                // value based on algorithm
                if (d2 > 0) {
                    --y;
                    dy = dy - (2 * rx * rx);
                    d2 = d2 + (rx * rx) - dy;
                }
                else {
                    --y;
                    ++x;
                    dx = dx + (2 * ry * ry);
                    dy = dy - (2 * rx * rx);
                    d2 = d2 + dx - dy + (rx * rx);
                }
            }
        }
    public:
        // draws a point at the specified location and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void point(Destination& destination, spoint16 location,typename Destination::pixel_type color,srect16* clip=nullptr) {
            if(clip!=nullptr && !clip->intersects(location))
                return;
            point16 p;
            if(translate(location,&p)) {
                destination[p]=color;
            }
        }
        // draws a filled rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void filled_rectangle(Destination& destination, const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr) {
            rect16 r;
            if(nullptr==clip) {
                
                if(translate_adjust(rect,&r))
                    destination.fill(r,color);
                return;
            }
            if(!clip->intersects(rect))
                return;
            
            srect16 sr = rect.crop(*clip);
            if(translate_adjust(sr,&r))
                destination.fill(r,color);
        }
        // draws a rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void rectangle(Destination& destination, const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr) {
            // top or bottom
            line(destination,srect16(rect.x1,rect.y1,rect.x2,rect.y1),color,clip);
            // left or right
            line(destination,srect16(rect.x1,rect.y1,rect.x1,rect.y2),color,clip);
            // right or left
            line(destination,srect16(rect.x2,rect.y1,rect.x2,rect.y2),color,clip);
            // bottom or top
            line(destination,srect16(rect.x1,rect.y2,rect.x2,rect.y2),color,clip);
        }
        // draws a line with the specified start and end point and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void line(Destination& destination, const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr) {
            if(rect.x1==rect.x2||rect.y1==rect.y2) {
                filled_rectangle(destination,rect,color,clip);
                return;
            }
            srect16 c = (nullptr!=clip)?*clip:rect;
            ssize16 ss;
            translate(destination.dimensions(),&ss);
            c = c.crop(srect16(spoint16(0,0),ss));
            srect16 r = rect;
            if(!c.contains(r))
                line_clip(&r,&c);
            
            
            float xinc,yinc,x,y;
            float dx,dy,e;
            dx=std::abs(r.x2-r.x1);
            dy=std::abs(r.y2-r.y1);
            if(r.x1<r.x2)
                xinc=1;
            else
                xinc=-1;
            if(r.y1<r.y2)
                yinc=1;
            else
                yinc=-1;
            x=r.x1;
            y=r.y1;
            point(destination,spoint16(x,y),color,nullptr);
            if(dx>=dy)
            {
                e=(2*dy)-dx;
                while(x!=r.x2)
                {
                    if(e<0)
                    {
                        e+=(2*dy);
                    }
                    else
                    {
                        e+=(2*(dy-dx));
                        y+=yinc;
                    }
                    x+=xinc;
                    point(destination,spoint16(x,y),color,nullptr);
                }
            }
            else
            {
                e=(2*dx)-dy;
                while(y!=r.y2)
                {
                    if(e<0)
                    {
                        e+=(2*dx);
                    }
                    else
                    {
                        e+=(2*(dx-dy));
                        x+=xinc;
                    }
                    y+=yinc;
                    point(destination,spoint16(x,y),color,nullptr);
                }
            }
        }
        // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void ellipse(Destination& destination,const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr)  {
            ellipse_impl(destination,rect,color,clip,false);
        }
        // draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void filled_ellipse(Destination& destination,const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr)  {
            ellipse_impl(destination,rect,color,clip,true);
        }
        // draws an arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void arc(Destination& destination,const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr)  {
            arc_impl(destination,rect,color,clip,false);
        }
        // draws a arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void filled_arc(Destination& destination,const srect16& rect,typename Destination::pixel_type color,srect16* clip=nullptr)  {
            arc_impl(destination,rect,color,clip,true);
        }
        // draws a rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void rounded_rectangle(Destination& destination,const srect16& rect,const pointx<float>& ratios, typename Destination::pixel_type color,srect16* clip=nullptr)  {
            srect16 r=rect.normalize();
            float fx = .025>ratios.x?.025:ratios.x>.5?.5:ratios.x;
            float fy = .025>ratios.y?.025:ratios.y>.5?.5:ratios.y;
            int rw = (r.width()*fx+.5);
            int rh = (r.height()*fy+.5);
            // top
            line(destination,srect16(r.x1+rw,r.y1,r.x2-rw,r.y1),color,clip);
            // left
            line(destination,srect16(r.x1,r.y1+rh,r.x1,r.y2-rh),color,clip);
            // right
            line(destination,srect16(r.x2,r.y1+rh,r.x2,r.y2-rh),color,clip);
            // bottom
            line(destination,srect16(r.x1+rw,r.y2,r.x2-rw,r.y2),color,clip);

            arc(destination,srect16(r.top_left(),ssize16(rw+1,rh+1)),color,clip);
            arc(destination,srect16(spoint16(r.x2-rw,r.y1),ssize16(rw+1,rh+1)).flip_horizontal(),color,clip);
            arc(destination,srect16(spoint16(r.x1,r.y2-rh),ssize16(rw,rh)).flip_vertical(),color,clip);
            arc(destination,srect16(spoint16(r.x2-rw,r.y2-rh),ssize16(rw+1,rh)).flip_all(),color,clip);
        }
        // draws a filled rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination>
        static void filled_rounded_rectangle(Destination& destination,const srect16& rect,const pointx<float>& ratios, typename Destination::pixel_type color,srect16* clip=nullptr)  {
            srect16 r=rect.normalize();
            float fx = .025>ratios.x?.025:ratios.x>.5?.5:ratios.x;
            float fy = .025>ratios.y?.025:ratios.y>.5?.5:ratios.y;
            int rw = (r.width()*fx+.5);
            int rh = (r.height()*fy+.5);
            // top
            filled_rectangle(destination,srect16(r.x1+rw,r.y1,r.x2-rw,r.y1+rh-1),color,clip);
            // middle
            filled_rectangle(destination,srect16(r.x1,r.y1+rh,r.x2,r.y2-rh-1),color,clip);
            // bottom
            filled_rectangle(destination,srect16(r.x1+rw,r.y2-rh,r.x2-rw,r.y2),color,clip);

            filled_arc(destination,srect16(r.top_left(),ssize16(rw+1,rh+1)),color,clip);
            filled_arc(destination,srect16(spoint16(r.x2-rw,r.y1),ssize16(rw+1,rh+1)).flip_horizontal(),color,clip);
            filled_arc(destination,srect16(spoint16(r.x1,r.y2-rh),ssize16(rw,rh)).flip_vertical(),color,clip);
            filled_arc(destination,srect16(spoint16(r.x2-rw,r.y2-rh),ssize16(rw+1,rh)).flip_all(),color,clip);
        }
        // draws a portion of a bitmap or display buffer to the specified rectangle with an optional clipping rentangle
        template<typename Destination,typename Source>
        static inline void bitmap(Destination& destination,const srect16& dest_rect,Source& source,const rect16& source_rect,bitmap_flags options=bitmap_flags::crop,srect16* clip=nullptr) {
            bmp_helper<Destination,Source>
                ::draw_bitmap(destination,dest_rect,source,source_rect,options,clip);
        }
        // draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
        template<typename Destination>
        static void text(
            Destination& destination,
            srect16& dest_rect,
            const char* text,
            const font& font,
            typename Destination::pixel_type color,
            typename Destination::pixel_type backcolor=::gfx::color<typename Destination::pixel_type>::black,
            bool transparent_background = true,
            unsigned int tab_width=4,
            srect16* clip=nullptr) {
            const char*sz=text;
            int cw;
            uint16_t rw;
            srect16 chr(dest_rect.top_left(),ssize16(font.width(*sz),font.height()));
            if(0==*sz) return;
            font_char fc = font[*sz];
            while(*sz) {
                switch(*sz) {
                    case '\r':
                        chr.x1=dest_rect.x1;
                        ++sz;
                        if(*sz) {
                            font_char nfc = font[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            fc=nfc;
                        }  
                        break;
                    case '\n':
                        chr.x1=dest_rect.x1;
                        ++sz;
                        if(*sz) {
                            font_char nfc = font[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            fc=nfc;
                        }
                        chr=chr.offset(0,font.height());
                        if(chr.y2>dest_rect.bottom())
                            return;
                        break;
                    case '\t':
                        ++sz;
                        if(*sz) {
                            font_char nfc = font[*sz];
                            rw=chr.x1+nfc.width()-1;
                            fc=nfc;
                        } else
                            rw=chr.width();
                        cw = font.average_width()*4;
                        chr.x1=((chr.x1/cw)+1)*cw;
                        chr.x2=chr.x1+rw-1;
                        if(chr.right()>dest_rect.right()) {
                            chr.x1=dest_rect.x1;
                            chr=chr.offset(0,font.height());
                        } 
                        if(chr.y2>dest_rect.bottom())
                            return;
                        
                        break;
                    default:
                        // draw the character
                        size_t wb = (fc.width()+7)/8;
                        const uint8_t* p = fc.data();
                        for(size_t j=0;j<font.height();++j) {
                            bits::int_max m = 1 << (fc.width()-1);
                            bits::int_max accum=0;
                            memcpy(&accum,p,wb);
                            p+=wb;
                            for(size_t n=0;n<fc.width();++n) {
                                if(accum&m) {
                                    point(destination,spoint16(chr.left()+n,chr.top()+j),color,clip);
                                } else if(!transparent_background) {
                                    point(destination,spoint16(chr.left()+n,chr.top()+j),backcolor,clip);
                                }

                                accum<<=1;
                            }
                        }
                        chr=chr.offset(fc.width(),0);
                        ++sz;
                        if(*sz) {
                            font_char nfc = font[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            if(chr.right()>dest_rect.right()) {
                                
                                chr.x1=dest_rect.x1;
                                chr=chr.offset(0,font.height());
                            }
                            if(chr.y2>dest_rect.bottom())
                                return;
                            fc=nfc;
                        }
                        break;
                }
            }
        }
    };
}
#endif