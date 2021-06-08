#ifndef HTCW_GFX_DRAWING_HPP
#define HTCW_GFX_DRAWING_HPP
#include "bits.hpp"
#include <math.h>
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"
#include "gfx_draw_helpers.hpp"
#include "gfx_palette.hpp"
#include "gfx_font.hpp"
#include "gfx_image.hpp"
namespace gfx {
    
    enum struct bitmap_resize {
        crop = 0,
        resize_fast = 1,
        resize_bilinear = 2,
        resize_bicubic = 3
    };
    namespace helpers {
        // adapted from Alan Wolfe's blog https://blog.demofox.org/2015/08/15/resizing-images-with-bicubic-interpolation/
        /*
        Unless otherwise noted, all content of this website is licensed under the MIT license below (https://opensource.org/licenses/MIT):
        Copyright 2019 Alan Wolfe
        Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
        THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
        */
        // t is a value that goes from 0 to 1 to interpolate in a C1 continuous way across uniformly sampled data points.
        // when t is 0, this will return B.  When t is 1, this will return C.  Inbetween values will return an interpolation
        // between B and C.  A and B are used to calculate slopes at the edges.
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
        template<typename Source>
        gfx_result sample_nearest (const Source& source, const rect16& src_rect, float u, float v,typename Source::pixel_type* out_pixel) {
            // calculate coordinates
            
            point16 pt(uint16_t(u * src_rect.width()+src_rect.left()),
                uint16_t(v * src_rect.height()+src_rect.top()));

            clamp_point16(pt,src_rect);
            // return pixel
            return source.point(pt,out_pixel);
            
        }
        template<typename Source>
        gfx_result sample_bilinear (const Source& source, const rect16& src_rect, float u, float v,typename Source::pixel_type* out_pixel) {
            using pixel_type = typename Source::pixel_type;
            using rgba_type = rgba_pixel<HTCW_MAX_WORD>;
            // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
            float x = (u * src_rect.width()) - 0.5f+src_rect.left();
            int xint = int(x);
            float xfract = x - floor(x);
        
            float y = (v * src_rect.height()) - 0.5f+src_rect.top();
            int yint = int(y);
            float yfract = y - floor(y);
            gfx_result r;
            // get pixels
            
            point16 pt00(xint+0,yint+0);
            point16 pt10(xint+1,yint+0);
            point16 pt01(xint+0,yint+1);
            point16 pt11(xint+1,yint+1);
            pixel_type px00,px10,px01,px11;
            rgba_type cpx00,cpx10,cpx01,cpx11;
            clamp_point16(pt00,src_rect);
            clamp_point16(pt10,src_rect);
            clamp_point16(pt01,src_rect);
            clamp_point16(pt11,src_rect);
            r=source.point(pt00,&px00);
            if(gfx_result::success!=r) {
                return r;
            }
            r=convert_palette_to(source,px00,&cpx00);
            if(gfx_result::success!=r) {
                return r;
            }
            r=source.point(pt10,&px10);
            if(gfx_result::success!=r) {
                return r;
            }
            r=convert_palette_to(source,px10,&cpx10);
            if(gfx_result::success!=r) {
                return r;
            }
            r=source.point(pt01,&px01);
            if(gfx_result::success!=r) {
                return r;
            }
            r=convert_palette_to(source,px01,&cpx01);
            if(gfx_result::success!=r) {
                return r;
            }
            r=source.point(pt11,&px11);
            if(gfx_result::success!=r) {
                return r;
            }
            r=convert_palette_to(source,px11,&cpx11);
            if(gfx_result::success!=r) {
                return r;
            }
            // interpolate bi-linearly!
            rgba_type ccol0;
            r=cpx00.blend(cpx10,xfract,&ccol0);
            if(gfx_result::success!=r) {
                return r;
            }
            rgba_type ccol1;
            r=cpx01.blend(cpx11,xfract,&ccol1);
            if(gfx_result::success!=r) {
                return r;
            }
            rgba_type ccol2;
            r=ccol0.blend(ccol1,yfract,&ccol2);
            if(gfx_result::success!=r) {
                return r;
            }
            r=convert_palette_from(source,ccol2,out_pixel);
            return gfx_result::success;
        }
        template<typename Source>
        gfx_result sample_bicubic (const Source& source, const rect16& src_rect, float u, float v,typename Source::pixel_type* out_pixel) {
            using pixel_type = typename Source::pixel_type;
            using rgba_type = rgba_pixel<HTCW_MAX_WORD>;
            // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
            float x = (u * src_rect.width()) - 0.5+src_rect.left();
            int xint = int(x);
            float xfract = x - floor(x);
        
            float y = (v * src_rect.height()) - 0.5+src_rect.top();
            int yint = int(y);
            float yfract = y - floor(y);

            gfx_result r;

            point16 pt00(xint-1,yint-1),pt10(xint+0,yint-1),pt20(xint+1,yint-1),pt30(xint+2,yint-1),
                    pt01(xint-1,yint+0),pt11(xint+0,yint+0),pt21(xint+1,yint+0),pt31(xint+2,yint+0),
                    pt02(xint-1,yint+1),pt12(xint+0,yint+1),pt22(xint+1,yint+1),pt32(xint+2,yint+1),
                    pt03(xint-1,yint+2),pt13(xint+0,yint+2),pt23(xint+1,yint+2),pt33(xint+2,yint+2);
                    
            pixel_type px00,px10,px20,px30,
                    px01,px11,px21,px31,
                    px02,px12,px22,px32,
                    px03,px13,px23,px33;
            rgba_type cpx00,cpx10,cpx20,cpx30,
                    cpx01,cpx11,cpx21,cpx31,
                    cpx02,cpx12,cpx22,cpx32,
                    cpx03,cpx13,cpx23,cpx33;
            
            rgba_type rpx;
            
            double d0,d1,d2,d3;
            
            // interpolate bi-cubically!

            // 1st row
            clamp_point16(pt00,src_rect);clamp_point16(pt10,src_rect);clamp_point16(pt20,src_rect);clamp_point16(pt30,src_rect);
            r = source.point(pt00,&px00); if(gfx_result::success!=r) return r;
            r = source.point(pt10,&px10); if(gfx_result::success!=r) return r;
            r = source.point(pt20,&px20); if(gfx_result::success!=r) return r;
            r = source.point(pt30,&px30); if(gfx_result::success!=r) return r;
            
            r=(convert_palette_to(source,px00,&cpx00)); if(gfx_result::success!=r) return r;
            r=(convert_palette_to(source,px10,&cpx10)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px20,&cpx20)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px30,&cpx30)); if(gfx_result::success!=r) return r;

            // 2nd row
            clamp_point16(pt01,src_rect);clamp_point16(pt11,src_rect);clamp_point16(pt21,src_rect);clamp_point16(pt31,src_rect);
            r = source.point(pt01,&px01); if(gfx_result::success!=r) return r;
            r = source.point(pt11,&px11); if(gfx_result::success!=r) return r;
            r = source.point(pt21,&px21); if(gfx_result::success!=r) return r;
            r = source.point(pt31,&px31); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px01,&cpx01)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px11,&cpx11)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px21,&cpx21)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px31,&cpx31)); if(gfx_result::success!=r) return r;

            // 3rd row
            clamp_point16(pt02,src_rect);clamp_point16(pt12,src_rect);clamp_point16(pt22,src_rect);clamp_point16(pt32,src_rect);
            r = source.point(pt02,&px02); if(gfx_result::success!=r) return r;
            r = source.point(pt12,&px12); if(gfx_result::success!=r) return r;
            r = source.point(pt22,&px22); if(gfx_result::success!=r) return r;
            r = source.point(pt32,&px32); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px02,&cpx02)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px12,&cpx12)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px22,&cpx22)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px32,&cpx32)); if(gfx_result::success!=r) return r;

            // 4th row
            clamp_point16(pt03,src_rect);clamp_point16(pt13,src_rect);clamp_point16(pt23,src_rect);clamp_point16(pt33,src_rect);
            r = source.point(pt03,&px03); if(gfx_result::success!=r) return r;
            r = source.point(pt13,&px13); if(gfx_result::success!=r) return r;
            r = source.point(pt23,&px23); if(gfx_result::success!=r) return r;
            r = source.point(pt33,&px33); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px03,&cpx03)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px13,&cpx13)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px23,&cpx23)); if(gfx_result::success!=r) return r;
            r = (convert_palette_to(source,px33,&cpx33)); if(gfx_result::success!=r) return r;
            
            // Clamp the values since the curve can put the value below 0 or above 1
            const size_t chiR = rgba_type::channel_index_by_name<channel_name::R>::value;
            d0 = cubic_hermite(cpx00.template channelr_unchecked<chiR>(),
                            cpx10.template channelr_unchecked<chiR>(),
                            cpx20.template channelr_unchecked<chiR>(),
                            cpx30.template channelr_unchecked<chiR>(),
                            xfract);
            d1 = cubic_hermite(cpx01.template channelr_unchecked<chiR>(),
                            cpx11.template channelr_unchecked<chiR>(),
                            cpx21.template channelr_unchecked<chiR>(),
                            cpx31.template channelr_unchecked<chiR>(),
                            xfract);
            d2 = cubic_hermite(cpx02.template channelr_unchecked<chiR>(),
                            cpx12.template channelr_unchecked<chiR>(),
                            cpx22.template channelr_unchecked<chiR>(),
                            cpx32.template channelr_unchecked<chiR>(),
                            xfract);
            d3 = cubic_hermite(cpx03.template channelr_unchecked<chiR>(),
                            cpx13.template channelr_unchecked<chiR>(),
                            cpx23.template channelr_unchecked<chiR>(),
                            cpx33.template channelr_unchecked<chiR>(),
                            xfract);
            rpx.channelr<channel_name::R>(helpers::clamp(cubic_hermite(d0,d1,d2,d3,yfract),0.0,1.0));
            
            const size_t chiG = rgba_type::channel_index_by_name<channel_name::G>::value;
            d0 = cubic_hermite(cpx00.template channelr_unchecked<chiG>(),
                            cpx10.template channelr_unchecked<chiG>(),
                            cpx20.template channelr_unchecked<chiG>(),
                            cpx30.template channelr_unchecked<chiG>(),
                            xfract);
            d1 = cubic_hermite(cpx01.template channelr_unchecked<chiG>(),
                            cpx11.template channelr_unchecked<chiG>(),
                            cpx21.template channelr_unchecked<chiG>(),
                            cpx31.template channelr_unchecked<chiG>(),
                            xfract);
            d2 = cubic_hermite(cpx02.template channelr_unchecked<chiG>(),
                            cpx12.template channelr_unchecked<chiG>(),
                            cpx22.template channelr_unchecked<chiG>(),
                            cpx32.template channelr_unchecked<chiG>(),
                            xfract);
            d3 = cubic_hermite(cpx03.template channelr_unchecked<chiG>(),
                            cpx13.template channelr_unchecked<chiG>(),
                            cpx23.template channelr_unchecked<chiG>(),
                            cpx33.template channelr_unchecked<chiG>(),
                            xfract);
            rpx.channelr<channel_name::G>(helpers::clamp(cubic_hermite(d0,d1,d2,d3,yfract),0.0,1.0));
            
            const size_t chiB = rgba_type::channel_index_by_name<channel_name::B>::value;
            d0 = cubic_hermite(cpx00.template channelr_unchecked<chiB>(),
                            cpx10.template channelr_unchecked<chiB>(),
                            cpx20.template channelr_unchecked<chiB>(),
                            cpx30.template channelr_unchecked<chiB>(),
                            xfract);
            d1 = cubic_hermite(cpx01.template channelr_unchecked<chiB>(),
                            cpx11.template channelr_unchecked<chiB>(),
                            cpx21.template channelr_unchecked<chiB>(),
                            cpx31.template channelr_unchecked<chiB>(),
                            xfract);
            d2 = cubic_hermite(cpx02.template channelr_unchecked<chiB>(),
                            cpx12.template channelr_unchecked<chiB>(),
                            cpx22.template channelr_unchecked<chiB>(),
                            cpx32.template channelr_unchecked<chiB>(),
                            xfract);
            d3 = cubic_hermite(cpx03.template channelr_unchecked<chiB>(),
                            cpx13.template channelr_unchecked<chiB>(),
                            cpx23.template channelr_unchecked<chiB>(),
                            cpx33.template channelr_unchecked<chiB>(),
                            xfract);
            rpx.channelr<channel_name::B>(helpers::clamp(cubic_hermite(d0,d1,d2,d3,yfract),0.0,1.0));
            
            const size_t chiA = rgba_type::channel_index_by_name<channel_name::A>::value;
            if(-1<chiA) {
                const size_t chiA = rgba_type::channel_index_by_name<channel_name::A>::value;
                d0 = cubic_hermite(cpx00.template channelr_unchecked<chiA>(),
                                cpx10.template channelr_unchecked<chiA>(),
                                cpx20.template channelr_unchecked<chiA>(),
                                cpx30.template channelr_unchecked<chiA>(),
                                xfract);
                d1 = cubic_hermite(cpx01.template channelr_unchecked<chiA>(),
                                cpx11.template channelr_unchecked<chiA>(),
                                cpx21.template channelr_unchecked<chiA>(),
                                cpx31.template channelr_unchecked<chiA>(),
                                xfract);
                d2 = cubic_hermite(cpx02.template channelr_unchecked<chiA>(),
                                cpx12.template channelr_unchecked<chiA>(),
                                cpx22.template channelr_unchecked<chiA>(),
                                cpx32.template channelr_unchecked<chiA>(),
                                xfract);
                d3 = cubic_hermite(cpx03.template channelr_unchecked<chiA>(),
                                cpx13.template channelr_unchecked<chiA>(),
                                cpx23.template channelr_unchecked<chiA>(),
                                cpx33.template channelr_unchecked<chiA>(),
                                xfract);
                rpx.channelr<channel_name::A>(helpers::clamp(cubic_hermite(d0,d1,d2,d3,yfract),0.0,1.0));
            }
            return convert_palette_from(source,rpx,out_pixel);
        }
    }
    // provides drawing primitives over a bitmap or compatible type 
    struct draw {

    private:
        
        template<typename Destination,typename Source,bool CopyFrom,bool CopyTo,bool BltDst,bool BltSrc,bool Async> 
        struct draw_bmp_caps_helper {
            
        };
        
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,true,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return source.copy_to(src_rect,destination,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,false,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return destination.copy_from(src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,true,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return source.copy_to_async(src_rect,destination,location);
            }
        };
        
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,false,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return destination.copy_from(src_rect,source,location);
                //return copy_from_impl_helper<Destination,Source,Destination::caps::batch,Destination::caps::async>::do_draw(destintion,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,false,false,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                
                return destination.copy_from_async(src_rect,source,location);
                //return copy_from_impl_helper<Destination,Source,Destination::caps::batch,Destination::caps::async>::do_draw(destintion,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,false,false,false,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return destination.copy_from(src_rect,source,location);
                //return copy_from_impl_helper<Destination,Source,Destination::caps::batch,Destination::caps::async>::do_draw(destintion,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,false,false,false,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                using thas_alpha=typename Source::pixel_type::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return copy_from_impl_helper<Destination,Source,!(has_alpha&&!dhas_alpha),false>::do_draw(destination,src_rect,source,location);
                //return destination.copy_from(src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,false,false,true,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return source.copy_to_async(src_rect,destination,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,false,false,true,false,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return copy_from_impl_helper<Destination,Source,false,false>::do_draw(destination,src_rect,source,location);
                //return destination.copy_from(src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,false,false,false,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                using thas_alpha=typename Source::pixel_type::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return copy_from_impl_helper<Destination,Source,!(has_alpha&&!dhas_alpha),Destination::caps::async>::do_draw(destination,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,true,false,false,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                using thas_alpha=typename Source::pixel_type::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return copy_from_impl_helper<Destination,Source,!(has_alpha&&!dhas_alpha),false>::do_draw(destination,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,true,false,false,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                using thas_alpha=typename Source::pixel_type::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return copy_from_impl_helper<Destination,Source,!(has_alpha&&!dhas_alpha),Destination::caps::async>::do_draw(destination,src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,true,true,true,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return destination.copy_from(src_rect,source,location);
            }
        };
        // copyfrom, copyto, bltdst, bltsrc, async
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,true,false,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return destination.copy_from(src_rect,source,location);
            }
        };
        // copyfrom, copyto, bltdst, bltsrc, async
        template<typename Destination,typename Source> 
        struct draw_bmp_caps_helper<Destination,Source,true,true,true,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return destination.copy_from_async(src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,true,true,false,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return destination.copy_from_async(src_rect,source,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,false,true,false,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                return source.copy_to(src_rect,destination,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,false,true,true,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return source.copy_to_async(src_rect,destination,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,false,true,false,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                //return copy_from_impl_helper<Destination,Source,Destination::caps::batch,Destination::caps::async>::do_draw(destination,src_rect,source,location);
                return source.copy_to_async(src_rect,destination,location);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async
        struct draw_bmp_caps_helper<Destination,Source,false,false,false,false,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                // reuse the non batch version of our batch helper since it already has this code
                return draw_bmp_batch_caps_helper<Destination,Source,false,true>::do_draw(destination,rect16(location,src_rect.dimensions()),source,src_rect,(int)rect_orientation::normalized);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async 
        struct draw_bmp_caps_helper<Destination,Source,false,false,false,false,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                // reuse the non batch version of our batch helper since it already has this code
                return draw_bmp_batch_caps_helper<Destination,Source,false,false>::do_draw(destination,rect16(location,src_rect.dimensions()),source,src_rect,(int)rect_orientation::normalized);
            }
        };
        template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async 
        struct draw_bmp_caps_helper<Destination,Source,false,true,true,true,false> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                return source.copy_to(src_rect,destination,location);
            }
        };
        /*template<typename Destination,typename Source> 
        // copyfrom, copyto, bltdst, bltsrc, async 
        struct draw_bmp_caps_helper<Destination,Source,false,true,true,true,true> {
            inline static gfx::gfx_result do_draw(Destination& destination, Source& source, const gfx::rect16& src_rect,gfx::point16 location) {
                // suspend if we can
                suspend_token<Destination> stok(destination,true);
                return source.copy_to(src_rect,destination,location);
            }
        };*/
        
        template<typename Destination,typename Source,bool Batch,bool Async>
        struct copy_from_impl_helper {};
        template<typename Destination,typename Source>
        struct copy_from_impl_helper<Destination,Source,true,true> {
            static gfx_result do_draw(Destination& destination,const rect16& src_rect,const Source& src,point16 location) {
                gfx_result r;
                rect16 srcr = src_rect.normalize().crop(src.bounds());
                rect16 dstr(location,src_rect.dimensions());
                dstr=dstr.crop(destination.bounds());
                if(srcr.width()>dstr.width()) {
                    srcr.x2=srcr.x1+dstr.width()-1;
                }
                if(srcr.height()>dstr.height()) {
                    srcr.y2=srcr.y1+dstr.height()-1;
                }
                uint16_t w = dstr.dimensions().width;
                uint16_t h = dstr.dimensions().height;
                r=destination.begin_batch_async(dstr);
                if(gfx_result::success!=r) {
                    return r;
                }
                for(uint16_t y=0;y<h;++y) {
                    for(uint16_t x=0;x<w;++x) {
                        typename Source::pixel_type pp;
                        r=src.point(point16(x+srcr.x1,y+srcr.y1), &pp);
                        if(r!=gfx_result::success)
                            return r;
                        typename Destination::pixel_type p;
                        r=convert_palette_to(destination,src,pp,&p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r = destination.write_batch_async(p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                    }
                }
                r=destination.commit_batch_async();
                if(gfx_result::success!=r) {
                    return r;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct copy_from_impl_helper<Destination,Source,false,true> {
            static gfx_result do_draw(Destination& destination,const rect16& src_rect,const Source& src,point16 location) {
                gfx_result r;
                rect16 srcr = src_rect.normalize().crop(src.bounds());
                rect16 dstr(location,src_rect.dimensions());
                dstr=dstr.crop(destination.bounds());
                if(srcr.width()>dstr.width()) {
                    srcr.x2=srcr.x1+dstr.width()-1;
                }
                if(srcr.height()>dstr.height()) {
                    srcr.y2=srcr.y1+dstr.height()-1;
                }
                uint16_t w = dstr.dimensions().width;
                uint16_t h = dstr.dimensions().height;
                
                for(uint16_t y=0;y<h;++y) {
                    for(uint16_t x=0;x<w;++x) {
                        typename Source::pixel_type pp;
                        r=src.point(gfx::point16(x+srcr.x1,y+srcr.y1), &pp);
                        if(r!=gfx_result::success)
                            return r;
                        typename Destination::pixel_type p;
                        r=convert_palette_to(destination,src,pp,&p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r=point_impl(destination,spoint16(dstr.x1+x,dstr.y1+y),p,nullptr,true);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                    }
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct copy_from_impl_helper<Destination,Source,true,false> {
            static gfx_result do_draw(Destination& destination,const rect16& src_rect,const Source& src,point16 location) {
                gfx_result r;
                rect16 srcr = src_rect.normalize().crop(src.bounds());
                rect16 dstr(location,src_rect.dimensions());
                dstr=dstr.crop(destination.bounds());
                if(srcr.width()>dstr.width()) {
                    srcr.x2=srcr.x1+dstr.width()-1;
                }
                if(srcr.height()>dstr.height()) {
                    srcr.y2=srcr.y1+dstr.height()-1;
                }
                uint16_t w = dstr.dimensions().width;
                uint16_t h = dstr.dimensions().height;
                r=destination.begin_batch(dstr);
                if(gfx_result::success!=r) {
                    return r;
                }
                for(uint16_t y=0;y<h;++y) {
                    for(uint16_t x=0;x<w;++x) {
                        typename Source::pixel_type pp;
                        r=src.point(point16(x+srcr.x1,y+srcr.y1), &pp);
                        if(r!=gfx_result::success)
                            return r;
                        typename Destination::pixel_type p;
                        r=convert_palette_to(destination,src,pp,&p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        //uint16_t pv = p.value();
                        
                        r = destination.write_batch(p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                    }
                }
                r=destination.commit_batch();
                if(gfx_result::success!=r) {
                    return r;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct copy_from_impl_helper<Destination,Source,false,false> {
            static gfx_result do_draw(Destination& destination,const rect16& src_rect,const Source& src,point16 location) {
                gfx_result r;
                rect16 srcr = src_rect.normalize().crop(src.bounds());
                rect16 dstr(location,src_rect.dimensions());
                dstr=dstr.crop(destination.bounds());
                if(srcr.width()>dstr.width()) {
                    srcr.x2=srcr.x1+dstr.width()-1;
                }
                if(srcr.height()>dstr.height()) {
                    srcr.y2=srcr.y1+dstr.height()-1;
                }
                uint16_t w = dstr.dimensions().width;
                uint16_t h = dstr.dimensions().height;
                for(uint16_t y=0;y<h;++y) {
                    for(uint16_t x=0;x<w;++x) {
                        typename Source::pixel_type pp;
                        r=src.point(point16(x+srcr.x1,y+srcr.y1), &pp);
                        if(r!=gfx_result::success)
                            return r;
                        typename Destination::pixel_type p;
                        r=convert_palette_to(destination,src,pp,&p);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r = point_impl(destination,spoint16(x,y),p,nullptr,false);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                    }
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source,bool Batch,bool Async>
        struct draw_bmp_batch_caps_helper { 
        };
        template<typename Destination,typename Source>
        struct draw_bmp_batch_caps_helper<Destination,Source,false,false> {
            static gfx_result do_draw(Destination& destination, const gfx::rect16& dstr, Source& source,const gfx::rect16& srcr,int o) {
                // do cropping
                bool flipX = (int)rect_orientation::flipped_horizontal==(o&(int)rect_orientation::flipped_horizontal);
                bool flipY= (int)rect_orientation::flipped_vertical==(o&(int)rect_orientation::flipped_vertical);
                    
                uint16_t x2=srcr.x1;
                uint16_t y2=srcr.y1;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                for(typename rect16::value_type y=dstr.y1;y<=dstr.y2;++y) {
                    for(typename rect16::value_type x=dstr.x1;x<=dstr.x2;++x) {
                        typename Source::pixel_type px;
                        uint16_t ssx=x2;
                        uint16_t ssy=y2;
                        if(flipX) {
                            ssx= srcr.x2-x2;
                        }
                        if(flipY) {
                            ssy=srcr.y2-y2;
                        }
                        gfx_result r = source.point(point16(ssx,ssy),&px);
                        if(gfx_result::success!=r)
                            return r;
                        typename Destination::pixel_type px2=convert<typename Source::pixel_type, typename Destination::pixel_type>(px);
                        r=point_impl(destination,spoint16(x,y),px2,nullptr,false);
                        if(gfx_result::success!=r)
                            return r;
                        ++x2;
                        if(x2>srcr.x2)
                            break;
                    }    
                    x2=srcr.x1;
                    ++y2;
                    if(y2>srcr.y2)
                        break;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct draw_bmp_batch_caps_helper<Destination,Source,true,false> {
            static gfx_result do_draw(Destination& destination, const gfx::rect16& dstr, Source& source,const gfx::rect16& srcr,int o) {
                // do cropping
                bool flipX = (int)rect_orientation::flipped_horizontal==(o&(int)rect_orientation::flipped_horizontal);
                bool flipY= (int)rect_orientation::flipped_vertical==(o&(int)rect_orientation::flipped_vertical);
                //gfx::rect16 sr = source_rect.crop(source.bounds()).normalize();
                uint16_t x2=0;
                uint16_t y2=0;
                gfx_result r = destination.begin_batch(dstr);
                if(gfx_result::success!=r)
                    return r;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination);
                for(typename rect16::value_type y=dstr.y1;y<=dstr.y2;++y) {
                    for(typename rect16::value_type x=dstr.x1;x<=dstr.x2;++x) {
                        typename Source::pixel_type px;
                        uint16_t ssx=x2+srcr.x1;
                        uint16_t ssy=y2+srcr.y1;
                        if(flipX) {
                            ssx= srcr.x2-x2;
                        }
                        if(flipY) {
                            ssy=srcr.y2-y2;
                        }
                        r = source.point(point16(ssx,ssy),&px);
                        if(gfx_result::success!=r)
                            return r;
                        typename Destination::pixel_type px2=convert<typename Source::pixel_type, typename Destination::pixel_type>(px);
                        r=destination.write_batch(px2);
                        if(gfx_result::success!=r)
                            return r;
                        ++x2;
                    }    
                    x2=0;
                    ++y2;
                }
                r=destination.commit_batch();
                if(gfx_result::success!=r) {
                    return r;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct draw_bmp_batch_caps_helper<Destination,Source,false,true> {
            static gfx_result do_draw(Destination& destination, const gfx::rect16& dstr, Source& source,const gfx::rect16& srcr,int o) {
                // do cropping
                bool flipX = (int)rect_orientation::flipped_horizontal==(o&(int)rect_orientation::flipped_horizontal);
                bool flipY= (int)rect_orientation::flipped_vertical==(o&(int)rect_orientation::flipped_vertical);
                    
                uint16_t x2=srcr.x1;
                uint16_t y2=srcr.y1;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                for(typename rect16::value_type y=dstr.y1;y<=dstr.y2;++y) {
                    for(typename rect16::value_type x=dstr.x1;x<=dstr.x2;++x) {
                        typename Source::pixel_type px;
                        uint16_t ssx=x2;
                        uint16_t ssy=y2;
                        if(flipX) {
                            ssx= srcr.x2-x2;
                        }
                        if(flipY) {
                            ssy=srcr.y2-y2;
                        }
                        gfx_result r = source.point(point16(ssx,ssy),&px);
                        if(gfx_result::success!=r)
                            return r;
                        typename Destination::pixel_type px2=convert<typename Source::pixel_type, typename Destination::pixel_type>(px);
                        r=point_impl(destination,spoint16(x,y),px2,nullptr,true);
                        if(gfx_result::success!=r)
                            return r;
                        ++x2;
                        if(x2>srcr.x2)
                            break;
                    }    
                    x2=srcr.x1;
                    ++y2;
                    if(y2>srcr.y2)
                        break;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        struct draw_bmp_batch_caps_helper<Destination,Source,true,true> {
            static gfx_result do_draw(Destination& destination, const gfx::rect16& dstr, Source& source,const gfx::rect16& srcr,int o) {
                // do cropping
                bool flipX = (int)rect_orientation::flipped_horizontal==(o&(int)rect_orientation::flipped_horizontal);
                bool flipY= (int)rect_orientation::flipped_vertical==(o&(int)rect_orientation::flipped_vertical);
                uint16_t x2=0;
                uint16_t y2=0;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,true);
                gfx_result r = destination.begin_batch_async(dstr);
                if(gfx_result::success!=r)
                    return r;
                // suspend if we can
                for(typename rect16::value_type y=dstr.y1;y<=dstr.y2;++y) {
                    for(typename rect16::value_type x=dstr.x1;x<=dstr.x2;++x) {
                        typename Source::pixel_type px;
                        uint16_t ssx=x2+srcr.x1;
                        uint16_t ssy=y2+srcr.y1;
                        if(flipX) {
                            ssx= srcr.x2-x2;
                        }
                        if(flipY) {
                            ssy=srcr.y2-y2;
                        }
                        r = source.point(point16(ssx,ssy),&px);
                        if(gfx_result::success!=r)
                            return r;
                        typename Destination::pixel_type px2=convert<typename Source::pixel_type, typename Destination::pixel_type>(px);
                        r=destination.write_batch_async(px2);
                        if(gfx_result::success!=r)
                            return r;
                        ++x2;
                    }    
                    x2=0;
                    ++y2;
                }
                r=destination.commit_batch_async();
                if(gfx_result::success!=r) {
                    return r;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source>
        static gfx_result draw_bitmap_impl(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect,bitmap_resize resize_type,const typename Source::pixel_type* transparent_color, const srect16* clip,bool async) {
            
            using batch=helpers::batcher<Destination,Destination::caps::batch,Destination::caps::async>;
            gfx_result r;
            rect16 srcr=source_rect.normalize().crop(source.bounds());
            srect16 dsr=dest_rect.crop((srect16)destination.bounds()).normalize();
            if(nullptr!=clip) {
                dsr=dsr.crop(*clip);
            }
            rect16 ddr = (rect16)dsr;
            int o = (int)dest_rect.orientation();
            const int w = dest_rect.width(),h=dest_rect.height();
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);

            using thas_alpha = typename Source::pixel_type::template has_channel_names<channel_name::A>;
            const bool has_alpha = thas_alpha::value;
            if(!has_alpha && transparent_color==nullptr) {
                r=batch::begin_batch(destination,ddr,async);
                if(gfx_result::success!=r) {
                    return r;
                }
            }
            if(bitmap_resize::crop==resize_type) {
                for (int y = 0; y < h; ++y) {
                    int yy = y+dest_rect.top();
                    if(yy<dsr.y1)
                        continue;
                    else if(yy>dsr.y2)
                        break;
                
                    int syy = y+srcr.top();
                    if((int)rect_orientation::flipped_vertical==((int)rect_orientation::flipped_vertical&o)) {
                        syy=srcr.bottom()-y;
                    }
                    for (int x = 0; x < w; ++x) {
                        int xx = x+dest_rect.left();
                        if(xx<dsr.x1) {
                            continue;
                        } else if(xx>dsr.x2)
                            break;
                        spoint16 spt(xx,yy);
                        int sxx = x+srcr.left();
                        if((int)rect_orientation::flipped_horizontal==((int)rect_orientation::flipped_horizontal&o)) {
                            sxx = srcr.right()-x;
                        }
                        typename Source::pixel_type srcpx;
                        point16 srcpt(sxx,syy);
                        r=source.point(srcpt,&srcpx);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        if(!has_alpha && nullptr==transparent_color) {
                            typename Destination::pixel_type px;
                            r=convert_palette(destination,source,srcpx,&px);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                            r=batch::write_batch(destination,(point16)spt,px,async);                        
                        } else if(nullptr==transparent_color || transparent_color->native_value!=srcpx.native_value) {
                            r=helpers::blender<Destination,Source,has_alpha&&Destination::caps::read>::point(destination,(point16)spt,source,srcpt,srcpx);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                            /*// already clipped, hence nullptr
                            double alpha = srcpx.template channelr<channel_name::A>();
                            if(has_alpha && alpha>=1.0) {
                                typename Destination::pixel_type bg;
                                r=destination.point((point16)spt,&bg);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                                typename Source::pixel_type sbgpx;
                                r=convert_palette_to(destination,bg,&sbgpx);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                                r=srcpx.blend(sbgpx,alpha,&srcpx);
                            }
                            typename Destination::pixel_type dp;
                            r=convert_palette_from(destination,srcpx,&dp);
                            // TODO: Make this do async
                            r=destination.point(spt,dp);
                            //r=draw::point_impl(destination,spt,srcpx,nullptr,async);
                            if(gfx_result::success!=r) {
                                return r;
                            }*/
                        }
                    }
                }
            } else { // resize
                for (int y = 0; y < h; ++y) {
                    int yy = y+dest_rect.top();
                    if(yy<dsr.y1)
                        continue;
                    else if(yy>dsr.y2)
                        break;
                
                    float v = float(y) / float(h - 1);
                    if((int)rect_orientation::flipped_vertical==((int)rect_orientation::flipped_vertical&o)) {
                        v=1.0-v;
                    }
                    for (int x = 0; x < w; ++x) {
                        int xx = x+dest_rect.left();
                        if(xx<dsr.x1) {
                            continue;
                        } else if(xx>dsr.x2)
                            break;
                        spoint16 spt(xx,yy);
                        float u = float(x) / float(w - 1);
                        if((int)rect_orientation::flipped_horizontal==((int)rect_orientation::flipped_horizontal&o)) {
                            u=1.0-u;
                        }
                        typename Source::pixel_type sampx;
                        
                        switch(resize_type) {
                            case bitmap_resize::resize_bicubic:
                                r=helpers::sample_bicubic(source,srcr,u,v,&sampx);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                                break;
                            case bitmap_resize::resize_bilinear:
                                r=helpers::sample_bilinear(source,srcr,u,v,&sampx);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                                break;
                            default:
                                r=helpers::sample_nearest(source,srcr,u,v,&sampx);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                                break;
                        }
                        if(!has_alpha && nullptr==transparent_color) {
                            typename Destination::pixel_type px;
                            r=convert_palette(destination,source,sampx,&px);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                            r=batch::write_batch(destination,(point16)spt,px,async);
                        } else if(nullptr==transparent_color || transparent_color->native_value!=sampx.native_value) {
                            // already clipped, hence nullptr
                            r=helpers::blender<Destination,Source,has_alpha&&Destination::caps::read>::point(destination,(point16)spt,source,point16(x+srcr.x1,y+srcr.y1),sampx);
                            //r=draw::point_impl(destination,spt,sampx,nullptr,async);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                        }
                    }
                }
            }
            if(!has_alpha && transparent_color==nullptr) {
                r=batch::commit_batch(destination,async);
                if(gfx_result::success!=r) {
                    return r;
                }
            }
            return gfx_result::success;
        }
        
        template<typename Destination,typename Source,typename DestinationPixelType,typename SourcePixelType>
        struct bmp_helper {
            inline static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect,bitmap_resize resize_type,const typename Source::pixel_type* transparent_color, const srect16* clip,bool async) {
                return draw_bitmap_impl(destination,dest_rect,source,source_rect,resize_type,transparent_color,clip,async);
            }
        };
        
        template<typename Destination,typename Source,typename PixelType>
        struct bmp_helper<Destination,Source,PixelType,PixelType> {
            static gfx_result draw_bitmap(Destination& destination, const srect16& dest_rect, Source& source, const rect16& source_rect,bitmap_resize resize_type, const typename Source::pixel_type* transparent_color,const srect16* clip,bool async) {
                const bool optimized = (Destination::caps::blt && Source::caps::blt) || (Destination::caps::copy_from || Source::caps::copy_to);
                
                // disqualify fast blting
                if(!optimized || transparent_color!=nullptr || dest_rect.x1>dest_rect.x2 || 
                    dest_rect.y1>dest_rect.y2 || 
                    ((bitmap_resize::crop!=resize_type)&&
                        (dest_rect.width()!=source_rect.width()||
                        dest_rect.height()!=source_rect.height())
                        )
                    ) {
                    return draw_bitmap_impl(destination,dest_rect,source,source_rect,resize_type,transparent_color,clip,async);
                    
                }
                
                rect16 dr;
                if(!translate_adjust(dest_rect,&dr))
                    return gfx_result::success; // whole thing is offscreen
                point16 loc(0,0);
                rect16 ccr;
                if(nullptr!=clip) {
                    if(!translate_adjust(*clip,&ccr))
                        return gfx_result::success; // clip rect is off screen
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
                loc.x+=source_rect.left();
                loc.y+=source_rect.top();
                rect16 r = rect16(loc,dim);
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
                //printf("draw_bmp_caps_helper<Destination,Source,%s,%s,%s,%s,%s>\r\n",Destination::caps::copy_from?"true":"false",Source::caps::copy_to?"true":"false",Destination::caps::blt?"true":"false",Source::caps::blt?"true":"false",Destination::caps::async?"true":"false");
                if(async)
                    return draw_bmp_caps_helper<Destination,Source,Destination::caps::copy_from,Source::caps::copy_to, Destination::caps::blt,Source::caps::blt,Destination::caps::async>::do_draw(destination,source, r,dr.top_left());
                else
                    return draw_bmp_caps_helper<Destination,Source,Destination::caps::copy_from,Source::caps::copy_to,Destination::caps::blt,Source::caps::blt,false>::do_draw(destination,source, r,dr.top_left());
                    
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
                    double x=0, y=0;
        
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
            srect16 t = in.crop(srect16(0,0,in.right(),in.bottom()));
            out->x1=t.x1;
            out->y1=t.y1;
            out->x2=t.x2;
            out->y2=t.y2;
            return true;
        }
        
        template<typename Destination,typename PixelType>
        static gfx_result ellipse_impl(Destination& destination, const srect16& rect,PixelType color,const srect16* clip,bool filled,bool async) {
            gfx_result r;
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
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            
            // Initial decision parameter of region 1
            d1 = (ry * ry)
                - (rx * rx * ry)
                + (0.25 * rx * rx);
            dx = 2 * ry * ry * x;
            dy = 2 * rx * rx * y;
            int_type oy=-1,ox=-1;
            // For region 1
            while (dx < dy+y_adj) {
                if(filled) {
                    if(oy!=y) {
                        r=line_impl(destination,srect16(-x + xc, y + yc+y_adj,x + xc+x_adj, y + yc+y_adj),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                        r=line_impl(destination,srect16(-x + xc, -y + yc,x + xc+x_adj, -y + yc),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                    }
                } else {
                   if(oy!=y || ox!=x) {
                        // Print points based on 4-way symmetry
                        r=point_impl(destination,spoint16(x + xc+x_adj, y + yc+y_adj),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                        r=point_impl(destination,spoint16(-x + xc, y + yc+y_adj),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                        r=point_impl(destination,spoint16(x + xc+x_adj, -y + yc),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                        r=point_impl(destination,spoint16(-x + xc, -y + yc),color,clip,async);
                        if(r!=gfx_result::success)
                            return r;
                    }
                }
                ox=x;
                oy=y;
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
                    r=line_impl(destination,srect16(-x + xc, y + yc+y_adj,x + xc+x_adj, y + yc+y_adj),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
                    r=line_impl(destination,srect16(-x + xc, -y + yc,x + xc+x_adj, -y + yc),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
                } else {
                    r=point_impl(destination,spoint16(x + xc+x_adj, y + yc+y_adj),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
                    r=point_impl(destination,spoint16(-x + xc, y + yc+y_adj),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
                    r=point_impl(destination,spoint16(x + xc+x_adj, -y + yc),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
                    r=point_impl(destination,spoint16(-x + xc, -y + yc),color,clip,async);
                    if(r!=gfx_result::success)
                        return r;
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
            return gfx_result::success;
        }
        template<typename Destination,typename PixelType>
        static gfx_result arc_impl(Destination& destination, const srect16& rect,PixelType color,const srect16* clip,bool filled,bool async) {
            using int_type = typename srect16::value_type;
            gfx_result r;
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
            int_type oy=-1,ox=-1;
            // Initial decision parameter of region 1
            d1 = (ry * ry)
                - (rx * rx * ry)
                + (0.25 * rx * rx);
            dx = 2 * ry * ry * x;
            dy = 2 * rx * rx * y;
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            // For region 1
            while (dx < dy+y_adj) {
                if(filled) {
                    if(oy!=y) {
                        switch(orient) {
                            case 0: //x1<=x2,y1<=y2
                                r=line_impl(destination,srect16(-x + xc, -y + yc, xc+x_adj, -y + yc),color,clip,async);        
                                if(r!=gfx_result::success)
                                    return r;
                                break;
                            case 1: //x1<=x2,y1>y2
                                r=line_impl(destination,srect16(-x + xc, y + yc-ry, xc+x_adj, y + yc-ry),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                break;
                            case 2: //x1>x2,y1<=y2
                                r=line_impl(destination,srect16( xc-rx, -y + yc,x + xc+x_adj-rx, -y + yc),color,clip,async);        
                                if(r!=gfx_result::success)
                                    return r;
                                break;
                            case 3: //x1>x2,y1>y2
                                r=line_impl(destination,srect16( xc-rx, y + yc-ry,x + xc-rx, y + yc-ry),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                break;
                        }
                    }
                } else {
                    if(oy!=y||ox!=x) {
                        switch(orient) {
                            case 0: //x1<=x2,y1<=y2
                                r=point_impl(destination,spoint16(-x + xc, -y + yc),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                if(x_adj && -y+yc==rect.y1) {
                                    r=point_impl(destination,rect.top_right(),color,clip,async);
                                    if(r!=gfx_result::success)
                                        return r;
                                }
                                
                                break;
                            case 1: //x1<=x2,y1>y2
                                r=point_impl(destination,spoint16(-x + xc, y + yc-ry),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                if(x_adj && y+yc-ry==rect.y1) {
                                    r=point_impl(destination,rect.bottom_right(),color,clip,async);
                                    if(r!=gfx_result::success)
                                        return r;
                                }
                                break;
                            case 2: //x1>x2,y1<=y2
                                r=point_impl(destination,spoint16(x + xc+x_adj-rx, -y + yc),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                if(x_adj && -y+yc==rect.y1) {
                                    r=point_impl(destination,rect.top_left(),color,clip,async);
                                    if(r!=gfx_result::success)
                                        return r;
                                }
                                break;
                            case 3: //x1>x2,y1>y2
                                r=point_impl(destination,spoint16(x + xc+x_adj-rx, y + yc-ry),color,clip,async);
                                if(r!=gfx_result::success)
                                    return r;
                                break;
                        }  
                    }
                }
                ox=x;
                oy=y;
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
                            r=line_impl(destination,srect16(-x + xc, -y + yc,xc+x_adj, -y + yc),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 1: //x1<=x2,y1>y2
                            r=line_impl(destination,srect16(-x + xc, y + yc-ry,xc+x_adj, y + yc-ry),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 2: //x1>x2,y1<=y2
                            r=line_impl(destination,srect16( xc-rx, -y + yc,x + xc+x_adj-rx, -y + yc),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 3: //x1>x2,y1>y2
                            r=line_impl(destination,srect16(xc-rx, y + yc-ry,x + xc+x_adj-rx, y + yc-ry),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                    }
                            
                } else {
                    switch(orient) {
                        case 0: //x1<=x2,y1<=y2
                            r=point_impl(destination,spoint16(-x + xc, -y + yc),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 1: //x1<=x2,y1>y2
                            r=point_impl(destination,spoint16(-x + xc, y + yc-ry),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 2: //x1>x2,y1<=y2
                            r=point_impl(destination,spoint16(x + xc+x_adj-rx, -y + yc),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
                            break;
                        case 3: //x1>x2,y1>y2
                            r=point_impl(destination,spoint16(x + xc+x_adj-rx, y + yc-ry),color,clip,async);
                            if(r!=gfx_result::success)
                                return r;
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
            return gfx_result::success;
        }
        // TODO: Augment this for async reads when supported:
        template<typename Destination,bool Read>
        struct read_point_helper {
        };
        template<typename Destination>
        struct read_point_helper<Destination,true> {
            inline static gfx_result do_read(const Destination& destination,point16 location,typename Destination::pixel_type* pixel) {
                return destination.point(location,pixel);
            }
        };
        template<typename Destination>
        struct read_point_helper<Destination,false> {
            inline static gfx_result do_read(const Destination& destination,point16 location,typename Destination::pixel_type* pixel) {
                typename Destination::pixel_type result;
                *pixel = result;
                return gfx_result::success;
            }
        };
        template<typename Destination,typename PixelType,bool Async>
        struct draw_point_helper {
        };
        template<typename Destination,typename PixelType>
        struct draw_point_helper<Destination,PixelType,true> {
            static gfx_result do_draw(Destination& destination,point16 location,PixelType color,bool async) {
                gfx_result r;
                if(!async) return draw_point_helper<Destination,PixelType,false>::do_draw(destination,location,color,false);
                typename Destination::pixel_type dpx;
                // TODO: recode to use an async read when finally implemented
                using thas_alpha=typename PixelType::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                if(has_alpha && !dhas_alpha) {
                    using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
                    const size_t chiA = tindexA::value;
                    using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
                    auto alp = color.template channel_unchecked<chiA>();
                    if(alp!=tchA::max) {
                        // we have to mix at this level because the destination doesn't support alpha
                        if(alp==tchA::min) {
                            return gfx_result::success;
                        }
                        typename Destination::pixel_type bgpx;
                        r= read_point_helper<Destination,Destination::caps::read>::do_read(destination,location,&bgpx);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r=convert_palette_from(destination, color,&dpx,&bgpx);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        if(async) {
                            return destination.point_async(location,dpx);
                        } 
                        return destination.point(location,dpx);
                        
                    }
                }
                //printf("color.native_value = %llx",(unsigned long long)color.native_value);
                r=convert_palette_from(destination,color,&dpx);
                //printf(", dpx.native_value = %d\r\n",(int)dpx.native_value);
                if(gfx_result::success!=r) {
                        return r;
                }
                if(async) {
                    return destination.point_async(location,dpx);
                } 
                return destination.point(location,dpx);
            }
        };
        template<typename Destination,typename PixelType>
        struct draw_point_helper<Destination,PixelType,false> {
            static gfx_result do_draw(Destination& destination,point16 location,PixelType color,bool async) {
                gfx_result r=gfx_result::success;
                typename Destination::pixel_type dpx;
                // TODO: recode to use an async read when finally implemented
                using thas_alpha=typename PixelType::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                if(has_alpha && !dhas_alpha) {
                    using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
                    const size_t chiA = tindexA::value;
                    using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
                    auto alp = color.template channel_unchecked<chiA>();
                    if(alp!=tchA::max) {
                        // we have to mix at this level because the destination doesn't support alpha
                        if(alp==tchA::min) {
                            return gfx_result::success;
                        }
                        typename Destination::pixel_type bgpx;
                        r= read_point_helper<Destination,Destination::caps::read>::do_read(destination,location,&bgpx);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r=convert_palette_from(destination,color,&dpx,&bgpx);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        return destination.point(location,dpx);
                    }
                }
                r=convert_palette_from(destination,color,&dpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return destination.point(location,dpx);
            }
        };
        template<typename Destination,typename PixelType>
        static gfx_result point_impl(Destination& destination, spoint16 location,PixelType color,const srect16* clip,bool async) {
            if(clip!=nullptr && !clip->intersects(location))
                return gfx_result::success;
            if(!((srect16)destination.bounds()).intersects(location))
                return gfx_result::success;
            point16 p;
            if(translate(location,&p)) {
                if(async)
                    return draw_point_helper<Destination,PixelType,Destination::caps::async>::do_draw(destination,p,color,async);
                return draw_point_helper<Destination,PixelType,false>::do_draw(destination,p,color,async);
            }
            return gfx_result::success;
        }
        template<typename Destination,typename PixelType,bool Async>
        struct draw_filled_rect_helper {
        };
        template<typename Destination,typename PixelType>
        struct draw_filled_rect_helper<Destination,PixelType,true> {
            static gfx_result do_draw(Destination& destination,const rect16& rect,PixelType color,bool async) {
                gfx_result r;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
                if(!async) return draw_filled_rect_helper<Destination,PixelType,false>::do_draw(destination,rect,color,false);
                typename Destination::pixel_type dpx;
                // TODO: recode to use an async read when finally implemented
                using thas_alpha=typename PixelType::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                if(has_alpha && !dhas_alpha) {
                    using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
                    const size_t chiA = tindexA::value;
                    using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
                    const auto alp = color.template channel_unchecked<chiA>();
                    if(alp!=tchA::max) {
                        if(alp==tchA::min) {
                            return gfx_result::success;
                        }
                        rect16 rr = rect.normalize();
                        for(int y=rr.y1;y<=rr.y2;++y) {
                            for(int x=rr.x1;x<=rr.x2;++x) {
                                r=point_impl(destination,spoint16(x,y),color,nullptr,true);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                            }
                        }
                        return gfx_result::success;
                    }
                }
                r=convert_palette_to(destination,color,&dpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return destination.fill_async(rect,dpx);
            }
        };
        template<typename Destination,typename PixelType>
        struct draw_filled_rect_helper<Destination,PixelType,false> {
            static gfx_result do_draw(Destination& destination,const rect16& rect,PixelType color,bool async) {
                gfx_result r;
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
                typename Destination::pixel_type dpx;
                // TODO: recode to use an async read when finally implemented
                using thas_alpha=typename PixelType::template has_channel_names<channel_name::A>;
                using tdhas_alpha=typename Destination::pixel_type::template has_channel_names<channel_name::A>;
                const bool has_alpha = thas_alpha::value;
                const bool dhas_alpha = tdhas_alpha::value;
                if(has_alpha && !dhas_alpha) {
                    using tindexA = typename PixelType::template channel_index_by_name<channel_name::A>;
                    const size_t chiA = tindexA::value;
                    using tchA = typename PixelType::template channel_by_index_unchecked<chiA>;
                    auto alp = color.template channel_unchecked<chiA>();
                    if(alp!=tchA::max) {
                        if(alp==tchA::min) {
                            return gfx_result::success;
                        }
                        rect16 rr = rect.normalize();
                        for(int y=rr.y1;y<=rr.y2;++y) {
                            for(int x=rr.x1;x<=rr.x2;++x) {
                                r = point_impl(destination,spoint16(x,y),color,nullptr,false);
                                if(gfx_result::success!=r) {
                                    return r;
                                }
                            }
                        }
                        return gfx_result::success;
                    }
                    
                }
                r=convert_palette_from(destination,color,&dpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return destination.fill(rect,dpx);
            }
        };
        template<typename Destination,bool Async>
        struct async_wait_helper {
            inline static gfx_result wait_all(Destination& destination) {return gfx_result::success;}
        };
        template<typename Destination>
        struct async_wait_helper<Destination,true> {
            inline static gfx_result wait_all(Destination& destination) {return destination.wait_all_async();}
        };
        
        template<typename Destination,typename PixelType>
        static gfx_result filled_rectangle_impl(Destination& destination, const srect16& rect,PixelType color,const srect16* clip,bool async) {
            srect16 sr=rect;
            if(nullptr!=clip)
                sr=sr.crop(*clip);
            rect16 r;
            if(!translate_adjust(sr,&r))
                return gfx_result::success;
            r=r.crop(destination.bounds());
            if(async)
                return draw_filled_rect_helper<Destination,PixelType,Destination::caps::async>::do_draw(destination,r,color,true);
            return draw_filled_rect_helper<Destination,PixelType,false>::do_draw(destination,r,color,false);
        }
        template<typename Destination,typename PixelType, bool Batch>
        struct draw_font_batch_helper {
            static gfx_result do_draw(Destination& destination,const font& font,const font_char& fc,const srect16& chr,PixelType color,PixelType backcolor,bool transparent_background,const srect16* clip,bool async) {
                gfx_result r = gfx_result::success;
                // draw the character
                size_t wb = (fc.width()+7)/8;
                const uint8_t* p = fc.data();
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
                for(size_t j=0;j<font.height();++j) {
                    bits::int_max m = 1 << (fc.width()-1);
                    bits::int_max accum=0;
                    memcpy(&accum,p,wb);
                    p+=wb;
                    int run_start_fg = -1;
                    int run_start_bg = -1;
                    for(size_t n=0;n<fc.width();++n) {
                        if(accum&m) {
                            if(!transparent_background&&-1!=run_start_bg) {
                                r=line_impl(destination,srect16(run_start_bg+chr.left(),chr.top()+j,n-1+chr.left(),chr.top()+j),backcolor,clip,async);
                        
                                run_start_bg=-1;
                            }
                            if(-1==run_start_fg)
                                run_start_fg=n;
                        } else {
                            if(-1!=run_start_fg) {
                                r=line_impl(destination,srect16(run_start_fg+chr.left(),chr.top()+j,n-1+chr.left(),chr.top()+j),color,clip,async);
                        
                                run_start_fg=-1;
                            }
                            if(!transparent_background) {
                                if(-1==run_start_bg)
                                    run_start_bg=n;
                            }
                        }

                        accum<<=1;
                    }
                    if(-1!=run_start_fg) {
                        r=line_impl(destination,srect16(run_start_fg+chr.left(),chr.top()+j,fc.width()-1+chr.left(),chr.top()+j),color,clip,async);
                        
                    }
                    if(!transparent_background&&-1!=run_start_bg) {
                        r=line_impl(destination,srect16(run_start_bg+chr.left(),chr.top()+j,fc.width()-1+chr.left(),chr.top()+j),backcolor,clip,async);
                
                        
                    }
                }
                return r;
            }
        };
        template<typename Destination,typename PixelType>
        struct draw_font_batch_helper<Destination,PixelType,true> {
            static gfx_result do_draw(Destination& destination,const font& font,const font_char& fc,const srect16& chr,PixelType color,PixelType backcolor,bool transparent_background,const srect16* clip,bool async) {
                using batch = helpers::batcher<Destination,true,Destination::caps::async>;
                // transparent_background is ignored for this routine
                srect16 sr = srect16(chr);
                if(nullptr!=clip)
                    sr=sr.crop(*clip);
                if(!sr.intersects((srect16)destination.bounds()))
                    return gfx_result::success;
                rect16 dr = (rect16)sr.crop((srect16)destination.bounds());
                // suspend if we can
                helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
                gfx_result r = batch::begin_batch(destination,dr,async);
                if(gfx_result::success!=r)
                    return r;
                // draw the character
                size_t wb = (fc.width()+7)/8;
                const uint8_t* p = fc.data();
                for(size_t j=0;j<font.height();++j) {
                    bits::int_max m = 1 << (fc.width()-1);
                    bits::int_max accum=0;
                    memcpy(&accum,p,wb);
                    p+=wb;
                    for(size_t n=0;n<=fc.width();++n) {
                        point16 pt(n+chr.left(),j+chr.top());
                        if(dr.intersects(pt)) {
                            if(accum&m) {
                                r=batch::write_batch(destination,pt,color,async);
                                if(gfx_result::success!=r)
                                    return r;
                            } else {
                                r=batch::write_batch(destination,pt,backcolor,async);
                                if(gfx_result::success!=r)
                                    return r;
                            }
                        }
                        accum<<=1;
                    }
                }
                r=batch::commit_batch(destination,async);
                return r;
            }
        };
        
        template<typename Destination,typename PixelType>
        static gfx_result line_impl(Destination& destination, const srect16& rect,PixelType color,const srect16* clip,bool async) {
            if(rect.x1==rect.x2||rect.y1==rect.y2) {
                return filled_rectangle_impl(destination,rect,color,clip,async);
            }
            srect16 c = (nullptr!=clip)?*clip:rect;
            ssize16 ss;
            translate(destination.dimensions(),&ss);
            c = c.crop(srect16(spoint16(0,0),ss));
            srect16 r = rect;
            if(!c.contains(r)) {
                 line_clip(&r,&c);
            }
            
            float xinc,yinc,x,y,ox,oy;
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
            x=ox=r.x1;
            y=oy=r.y1;
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
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
                        oy=y;
                        y+=yinc;
                    }
                    
                    x+=xinc;
                    if(oy!=y || y==r.y1) {
                        line_impl(destination,srect16(x,oy,x,oy),color,nullptr,async);
                        ox=x;
                    }
                    
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
                        ox=x;
                        x+=xinc;
                    }
                    y+=yinc;
                    if(ox!=x || x==r.x1) {
                        line_impl(destination,srect16(ox,y,ox,y),color,nullptr,async);
                        oy=y;
                    }
                }
            }
            return gfx_result::success;
        }
        template<typename Destination,typename PixelType>
        static gfx_result rectangle_impl(Destination& destination, const srect16& rect,PixelType color,const srect16* clip,bool async) {
            gfx_result r;
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            // top or bottom
            r=line_impl(destination,srect16(rect.x1,rect.y1,rect.x2,rect.y1),color,clip,async);
            if(r!=gfx_result::success)
                return r;
            // left or right
            r=line_impl(destination,srect16(rect.x1,rect.y1,rect.x1,rect.y2),color,clip,async);
            if(r!=gfx_result::success)
                return r;
            // right or left
            r=line_impl(destination,srect16(rect.x2,rect.y1,rect.x2,rect.y2),color,clip,async);
            if(r!=gfx_result::success)
                return r;
            // bottom or top
            return line_impl(destination,srect16(rect.x1,rect.y2,rect.x2,rect.y2),color,clip,async);
        }
        static bool get_poly_rect(const spoint16* path,size_t path_size,srect16* result) {
            if(0==path_size) return false;
            *result = srect16(*path,*path);
            ++path;
            while(--path_size) {
                if(path->x<result->x1) {
                    result->x1=path->x;
                } else if(path->x>result->x2) {
                    result->x2=path->x;
                }
                if(path->y<result->y1) {
                    result->y1=path->y;
                } else if(path->y>result->y2) {
                    result->y2=path->y;
                }
                ++path;
            }
            return true;
        }
        static bool intersects_poly(spoint16 pt,const spoint16* path,size_t path_size) {
            size_t j = path_size - 1;
            bool c = false;
            for(size_t i =0; i<path_size;++i) {
                if((pt.x == path[i].x) && (pt.y == path[i].y)) {
                    // point is a corner
                    return true;
                }
                if ((path[i].y > pt.y) != (path[j].y > pt.y)) {
                    const int slope = (pt.x-path[i].x)*(path[j].y-path[i].y)-(path[j].x-path[i].x)*(pt.y-path[i].y);
                    if(0==slope) {
                        // point is on boundary
                        return true;
                    }
                    if ((slope < 0) != (path[j].y < path[i].y)) {
                        c = !c;
                    }
                }
                j = i;
            }
            return c;
        }
            
        template<typename Destination,typename PixelType>
        static gfx_result polygon_impl(Destination& destination, const spath16& path, PixelType color,const srect16* clip,bool async) {
            gfx_result r;
            const spoint16* p = path.begin();
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            size_t path_size = path.size();
            while(path_size--) {
                const spoint16 p1 = *p;
                ++p;
                const spoint16 p2  = *p;
                const srect16 sr(p1,p2);
                r=line_impl(destination,sr,color,clip,async);
                if(gfx_result::success!=r) {
                    return r;
                }
            }
            return line_impl(destination,srect16((*p),(*(path.begin()))),color,clip,async);
        }
        template<typename Destination,typename PixelType>
        static gfx_result filled_polygon_impl(Destination& destination, const spath16& path, PixelType color,const srect16* clip,bool async) {
            gfx_result r;
            if(0==path.size()) { 
                return gfx_result::success;
            }
            srect16 pr=path.bounds();
            if(nullptr!=clip) {
                pr=pr.crop(*clip);
            }
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            for(int y = pr.y1;y<=pr.y2;++y) {
                int run_start = -1;
                for(int x = pr.x1;x<=pr.x2;++x) {
                    const spoint16 pt(x,y);
                    if(path.intersects(pt,true)) {
                        if(-1==run_start) {
                            run_start = x;
                        }
                    } else {
                        if(-1!=run_start) {
                            r=line_impl(destination,srect16(run_start,y,x-1,y),color,clip,async);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                            run_start = -1;
                        }
                    }
                }
                if(-1!=run_start) {
                    r=line_impl(destination,srect16(run_start,y,pr.x2,y),color,clip,async);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                }
            }
            return gfx_result::success;
        }
        template<typename Destination,typename PixelType>
        static gfx_result rounded_rectangle_impl(Destination& destination,const srect16& rect,float ratio, PixelType color,const srect16* clip,bool async) {
            // TODO: This can be sped up by copying the ellipse algorithm and modifying it slightly.
            gfx_result r;
            srect16 sr=rect.normalize();
            float fx = .025>ratio?.025:ratio>.5?.5:ratio;
            float fy = .025>ratio?.025:ratio>.5?.5:ratio;
            int rw = (sr.width()*fx+.5);
            int rh = (sr.height()*fy+.5);
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            // top
            r=line_impl(destination,srect16(sr.x1+rw,sr.y1,sr.x2-rw,sr.y1),color,clip,async);
            if(gfx_result::success!=r)
                return r;
            // left
            r=line_impl(destination,srect16(sr.x1,sr.y1+rh,sr.x1,sr.y2-rh),color,clip,async);
            if(gfx_result::success!=r)
                return r;
            // right
            r=line_impl(destination,srect16(sr.x2,sr.y1+rh,sr.x2,sr.y2-rh),color,clip,async);
            if(gfx_result::success!=r)
                return r;
            // bottom
            r=line_impl(destination,srect16(sr.x1+rw,sr.y2,sr.x2-rw,sr.y2),color,clip,async);
            if(gfx_result::success!=r)
                return r;

            r=arc_impl(destination,srect16(sr.top_left(),ssize16(rw+1,rh+1)),color,clip,false,async);
            if(gfx_result::success!=r)
                return r;
            r=arc_impl(destination,srect16(spoint16(sr.x2-rw,sr.y1),ssize16(rw+1,rh+1)).flip_horizontal(),color,clip,false,async);
            if(gfx_result::success!=r)
                return r;
            r=arc_impl(destination,srect16(spoint16(sr.x1,sr.y2-rh),ssize16(rw,rh)).flip_vertical(),color,clip,false,async);
            if(gfx_result::success!=r)
                return r;
            return arc_impl(destination,srect16(spoint16(sr.x2-rw,sr.y2-rh),ssize16(rw+1,rh)).flip_all(),color,clip,false,async);
        }
        template<typename Destination,typename PixelType>
        static gfx_result filled_rounded_rectangle_impl(Destination& destination,const srect16& rect,float ratio, PixelType color,const srect16* clip,bool async) {
            gfx_result r;
            srect16 sr=rect.normalize();
            float fx = .025>ratio?.025:ratio>.5?.5:ratio;
            float fy = .025>ratio?.025:ratio>.5?.5:ratio;
            int rw = (sr.width()*fx+.5);
            int rh = (sr.height()*fy+.5);
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
            // top
            r=filled_rectangle_impl(destination,srect16(sr.x1+rw,sr.y1,sr.x2-rw,sr.y1+rh-1),color,clip,async);
            if(gfx_result::success!=r) 
                return r;
            // middle
            r=filled_rectangle_impl(destination,srect16(sr.x1,sr.y1+rh,sr.x2,sr.y2-rh-1),color,clip,async);
            if(gfx_result::success!=r)
                return r;
            // bottom
            r=filled_rectangle_impl(destination,srect16(sr.x1+rw,sr.y2-rh,sr.x2-rw,sr.y2),color,clip,async);
            if(gfx_result::success!=r)
                return r;

            r=arc_impl(destination,srect16(sr.top_left(),ssize16(rw+1,rh+1)),color,clip,true,async);
            if(gfx_result::success!=r)
                return r;
            r=arc_impl(destination,srect16(spoint16(sr.x2-rw,sr.y1),ssize16(rw+1,rh+1)).flip_horizontal(),color,clip,true,async);
            if(gfx_result::success!=r)
                return r;
            r=arc_impl(destination,srect16(spoint16(sr.x1,sr.y2-rh),ssize16(rw,rh)).flip_vertical(),color,clip,true,async);
            if(gfx_result::success!=r)
                return r;
            return arc_impl(destination,srect16(spoint16(sr.x2-rw,sr.y2-rh),ssize16(rw+1,rh)).flip_all(),color,clip,true,async);
        }
        template<typename Destination>
        static gfx_result image_impl(Destination& destination, const srect16& destination_rect, stream* source_stream, const rect16& source_rect,bitmap_resize resize_type, srect16* clip,bool async) {
                struct load_context {
                    Destination& destination;
                    srect16 dst_rect;
                    rect16 src_rect;
                    bitmap_resize resize_type;
                    const srect16* clip;
                    rect_orientation orient;
                    pointx<float> scale;
                    bool async;
                };
                load_context context = { 
                destination,
                destination_rect,
                source_rect,
                resize_type,
                clip,
                rect_orientation::denormalized,
                pointx<float>(NAN,NAN),
                async
            };
            gfx_result r = jpeg_image::load(source_stream,[](size16 dimensions,jpeg_image::region_type& region,point16 location,void* state) {
                gfx_result r=gfx_result::success;
                load_context& ctx = 
                    *(load_context*)state;
                if(isnan(ctx.scale.x)) {
                    if(ctx.src_rect.x2==uint16_t(0xFFFF) && ctx.src_rect.y2==uint16_t(0xFFFF)) {
                        ctx.src_rect.x2=dimensions.width+ctx.src_rect.x1-1;
                        ctx.src_rect.y2=dimensions.height+ctx.src_rect.y1-1;
                    }
                    ctx.src_rect=ctx.src_rect.crop(dimensions.bounds());
                    ctx.orient = ctx.dst_rect.orientation();
                    ctx.src_rect.normalize_inplace();
                    ctx.dst_rect.normalize_inplace();
                    if(dimensions==(size16)ctx.dst_rect.dimensions()) {
                        ctx.resize_type = bitmap_resize::crop;
                    }
                    if(bitmap_resize::crop==ctx.resize_type) {
                        ctx.scale.x=ctx.scale.y=1.0;
                    } else {
                        ctx.scale.x = float(dimensions.width)/ctx.dst_rect.dimensions().width;
                        ctx.scale.y = float(dimensions.height)/ctx.dst_rect.dimensions().height;
                        //printf("ctx scale: (%f, %f)\r\n",ctx.scale.x,ctx.scale.y);
                    }
                    switch(ctx.resize_type) {
                        case bitmap_resize::crop:
                        case bitmap_resize::resize_fast:
                            break;
                        case bitmap_resize::resize_bilinear:
                            return gfx_result::not_supported;
                        case bitmap_resize::resize_bicubic:
                            return gfx_result::not_supported;
                    }   
                }
                if((location.x+region.dimensions().width-1>=ctx.src_rect.x1) && 
                    (location.y+region.dimensions().height-1>=ctx.src_rect.y1)) {
                    const spoint16 dst_offset = {
                        int16_t(((int)ctx.dst_rect.x1)-(int)ctx.src_rect.x1),
                        int16_t(((int)ctx.dst_rect.y1)-(int)ctx.src_rect.y1)
                    };
                    bool early_exit = false;
                    srect16 region_rect = (srect16)region.bounds();
                    region_rect.offset_inplace(location.x,location.y);
                    switch(ctx.resize_type) {
                        case bitmap_resize::crop: {
                            if(region_rect.y1<=((int)ctx.src_rect.y2) && region_rect.x1<=((int)ctx.src_rect.x2)) {
                                region_rect = region_rect.crop((srect16)ctx.src_rect);
                                // in case we need to debug:
                                //printf("region rect = (%d, %d)-(%d, %d) - size = (%d, %d)\r\n",region_rect.x1,region_rect.y1,region_rect.x2,region_rect.y2,region_rect.width(),region_rect.height());
                                rect16 region_rect_unoffset = (rect16)region_rect.offset(-location.x,-location.y);
                                srect16 target_rect = ((srect16)region_rect).offset(dst_offset.x,dst_offset.y);
                                if(ctx.dst_rect.intersects(target_rect)) {
                                    target_rect = target_rect.crop(ctx.dst_rect);
                                    region_rect.x2=ctx.src_rect.x1+target_rect.width()-1;
                                    region_rect.y2=ctx.src_rect.y1+target_rect.height()-1;
                                    if(ctx.async) {
                                        r=draw::bitmap_async(ctx.destination,target_rect,region,region_rect_unoffset,ctx.resize_type,nullptr,ctx.clip);    
                                    } else {
                                        r=draw::bitmap(ctx.destination,target_rect,region,region_rect_unoffset,ctx.resize_type,nullptr,ctx.clip);
                                    }
                                    if(gfx_result::success!=r) {
                                        return r;
                                    }
                                } else if(target_rect.y1>ctx.dst_rect.y2 || (target_rect.y1==ctx.dst_rect.y2 && target_rect.x1>ctx.dst_rect.x2)) {
                                    early_exit= true;
                                }
                            }
                            // early out if we don't need the rest:
                            if(early_exit) {
                                return gfx_result::canceled;
                            }
                        } break;
                        // TODO: Implement these - not easy or cheap
                        case bitmap_resize::resize_fast: {
                        case bitmap_resize::resize_bilinear:
                        case bitmap_resize::resize_bicubic:
                            return gfx_result::not_supported;
                        }
                    }
                }
            
                return r;
            },&context);
            if(r!=gfx_result::canceled&&r!=gfx_result::success) {
                return r;
            }
            return gfx_result::success;
        }
        // draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
        template<typename Destination,typename PixelType>
        static gfx_result text_impl(
            Destination& destination,
            const srect16& dest_rect,
            const char* text,
            const font& font,
            PixelType color,
            PixelType backcolor,
            bool transparent_background,
            unsigned int tab_width,
            srect16* clip,
            bool async) {
            gfx_result r=gfx_result::success;
            if(nullptr==text)
                return gfx_result::invalid_argument;
            const char*sz=text;
            int cw;
            uint16_t rw;
            srect16 chr(dest_rect.top_left(),ssize16(font.width(*sz),font.height()));
            if(0==*sz) return gfx_result::success;
            font_char fc = font[*sz];
            // suspend if we can
            helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async> stok(destination,async);
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
                        if(chr.y2>dest_rect.bottom()) {
                            return gfx_result::success;
                        }
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
                        if(chr.y2>dest_rect.bottom()) {
                            return gfx_result::success;
                        }
                        
                        break;
                    default:
                        if(nullptr==clip || clip->intersects(chr)) {
                            if(chr.intersects(dest_rect)) {
                                if(transparent_background)
                                    r=draw_font_batch_helper<Destination,PixelType,false>::do_draw(destination,font,fc,chr,color,backcolor,transparent_background,clip,async);
                                else
                                    r=draw_font_batch_helper<Destination,PixelType,Destination::caps::batch>::do_draw(destination,font,fc,chr,color,backcolor,transparent_background,clip,async);
                                if(gfx_result::success!=r) {
                                    return r;
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
                                    if(chr.y2>dest_rect.bottom()) {
                                        return gfx_result::success;
                                    }
                                    fc=nfc;
                                }
                            }
                        }
                        break;
                }
            }
            return gfx_result::success;
        }
        
    public:
        // draws a point at the specified location and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result point(Destination& destination, spoint16 location,PixelType color,const srect16* clip=nullptr) {
            return point_impl(destination,location,color,clip,false);
        }
        // asynchronously draws a point at the specified location and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result point_async(Destination& destination, spoint16 location,PixelType color,const srect16* clip=nullptr) {
            return point_impl(destination,location,color,clip,true);
        }
        // draws a filled rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_rectangle(Destination& destination, const srect16& rect,PixelType color,const srect16* clip=nullptr) {
            return filled_rectangle_impl(destination,rect,color,clip,false);
        }
        // asynchronously draws a filled rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_rectangle_async(Destination& destination, const srect16& rect,PixelType color,const srect16* clip=nullptr) {
            return filled_rectangle_impl(destination,rect,color,clip,true);
        }
        // draws a rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result rectangle(Destination& destination, const srect16& rect,PixelType color,const srect16* clip=nullptr) {
            return rectangle_impl(destination,rect,color,clip,false);
        }
        // asynchronously draws a rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result rectangle_async(Destination& destination, const srect16& rect, PixelType color,const srect16* clip=nullptr) {
            return rectangle_impl(destination,rect,color,clip,true);
        }
        // draws a line with the specified start and end point and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result line(Destination& destination, const srect16& rect, PixelType color,const srect16* clip=nullptr) {
            return line_impl(destination,rect,color,clip,false);
        }
        // asynchronously draws a line with the specified start and end point and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result line_async(Destination& destination, const srect16& rect,PixelType color,const srect16* clip=nullptr) {
            return line_impl(destination,rect,color,clip,true);
        }
        // draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result ellipse(Destination& destination,const srect16& rect, PixelType color,const srect16* clip=nullptr)  {
            return ellipse_impl(destination,rect,color,clip,false,false);
        }
        // asynchronously draws an ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result ellipse_async(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return ellipse_impl(destination,rect,color,clip,false,true);
        }
        // draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_ellipse(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return ellipse_impl(destination,rect,color,clip,true,false);
        }
        // asynchronously draws a filled ellipse with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_ellipse_async(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return ellipse_impl(destination,rect,color,clip,true,true);
        }
        // draws an arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result arc(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return arc_impl(destination,rect,color,clip,false,false);
        }
        // asynchronously draws an arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result arc_async(Destination& destination,const srect16& rect, PixelType color,const srect16* clip=nullptr)  {
            return arc_impl(destination,rect,color,clip,false,true);
        }
        // draws a arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_arc(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return arc_impl(destination,rect,color,clip,true,false);
        }
        // draws a arc with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_arc_async(Destination& destination,const srect16& rect,PixelType color,const srect16* clip=nullptr)  {
            return arc_impl(destination,rect,color,clip,true,true);
        }
        // draws a rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result rounded_rectangle(Destination& destination,const srect16& rect,float ratio, PixelType color,const srect16* clip=nullptr)  {
            return rounded_rectangle_impl(destination,rect,ratio,color,clip,false);
        }
        // asynchronously draws a rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result rounded_rectangle_async(Destination& destination,const srect16& rect,float ratio, PixelType color,const srect16* clip=nullptr)  {
            return rounded_rectangle_impl(destination,rect,ratio,color,clip,true);
        }
        // draws a filled rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_rounded_rectangle(Destination& destination,const srect16& rect,const float ratio, PixelType color,const srect16* clip=nullptr)  {
            return filled_rounded_rectangle_impl(destination,rect,ratio,color,clip,false);
        }
        // asynchronously draws a filled rounded rectangle with the specified dimensions and of the specified color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_rounded_rectangle_async(Destination& destination,const srect16& rect,const float ratio, PixelType color,const srect16* clip=nullptr)  {
            return filled_rounded_rectangle_impl(destination,rect,ratio,color,clip,true);
        }
        // draws a polygon with the specified path and color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result polygon(Destination& destination,const spath16& path, PixelType color,const srect16* clip=nullptr)  {
            return polygon_impl(destination,path,color,clip,false);
        }
        // draws a polygon with the specified path and color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result polygon_async(Destination& destination,const spath16& path, PixelType color,const srect16* clip=nullptr)  {
            return polygon_impl(destination,path,color,clip,true);
        }
        // draws a filled polygon with the specified path and color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_polygon(Destination& destination,const spath16& path, PixelType color,const srect16* clip=nullptr)  {
            return filled_polygon_impl(destination,path,color,clip,false);
        }
        // draws a filled polygon with the specified path and color, with an optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result filled_polygon_async(Destination& destination,const spath16& path,size_t path_size, PixelType color,const srect16* clip=nullptr)  {
            return filled_polygon_impl(destination,path,color,clip,true);
        }
        // draws a portion of a bitmap or display buffer to the specified rectangle with an optional clipping rentangle
        template<typename Destination,typename Source>
        static inline gfx_result bitmap(Destination& destination,const srect16& dest_rect,Source& source,const rect16& source_rect,bitmap_resize resize_type=bitmap_resize::crop,const typename Source::pixel_type* transparent_color=nullptr, const srect16* clip=nullptr) {
            return bmp_helper<Destination,Source,typename Destination::pixel_type,typename Source::pixel_type>
                ::draw_bitmap(destination,dest_rect,source,source_rect,resize_type,transparent_color,clip,false);
        }        
        // asynchronously draws a portion of a bitmap or display buffer to the specified rectangle with an optional clipping rentangle
        template<typename Destination,typename Source>
        static inline gfx_result bitmap_async(Destination& destination,const srect16& dest_rect,Source& source,const rect16& source_rect,bitmap_resize resize_type=bitmap_resize::crop, const typename Source::pixel_type* transparent_color=nullptr,const srect16* clip=nullptr) {
            return bmp_helper<Destination,Source,typename Destination::pixel_type,typename Source::pixel_type>
                ::draw_bitmap(destination,dest_rect,source,source_rect,resize_type,transparent_color,clip,true);
        }        
        // draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result text(
            Destination& destination,
            const srect16& dest_rect,
            const char* text,
            const font& font,
            PixelType color,
            PixelType backcolor=convert<::gfx::rgb_pixel<3>,PixelType>(::gfx::rgb_pixel<3>(0,0,0)),
            bool transparent_background = true,
            unsigned int tab_width=4,
            srect16* clip=nullptr) {
            return text_impl(destination,dest_rect,text,font,color,backcolor,transparent_background,tab_width,clip,false);
        }
        // asynchronously draws text to the specified destination rectangle with the specified font and colors and optional clipping rectangle
        template<typename Destination,typename PixelType>
        inline static gfx_result text_async(
            Destination& destination,
            const srect16& dest_rect,
            const char* text,
            const font& font,
            PixelType color,
            PixelType backcolor=convert<::gfx::rgb_pixel<3>,PixelType>(::gfx::rgb_pixel<3>(0,0,0)),
            bool transparent_background = true,
            unsigned int tab_width=4,
            srect16* clip=nullptr) {
            return text_impl(destination,dest_rect,text,font,color,backcolor,transparent_background,tab_width,clip,true);
        }
        // draws an image from the specified stream to the specified destination rectangle with the an optional clipping rectangle
        template<typename Destination>
        static inline gfx_result image(Destination& destination, const srect16& destination_rect, stream* source_stream, const rect16& source_rect,bitmap_resize resize_type=bitmap_resize::crop, srect16* clip=nullptr) {
            return image_impl(destination,destination_rect,source_stream,source_rect,resize_type,clip,false);
        }
        // asynchronously draws an image from the specified stream to the specified destination rectangle with the an optional clipping rectangle
        template<typename Destination>
        static inline gfx_result image_async(Destination& destination, const srect16& destination_rect, stream* source_stream, const rect16& source_rect,bitmap_resize resize_type=bitmap_resize::crop, srect16* clip=nullptr) {
            return image_impl(destination,destination_rect,source_stream,source_rect,resize_type,clip,true);
        }
        // waits for all asynchronous operations on the destination to complete
        template<typename Destination>
        static gfx_result wait_all_async(Destination& destination) {
            return async_wait_helper<Destination,Destination::caps::async>::wait_all(destination);
        }
        // suspends drawing for a destination that supports it
        template<typename Destination>
        static gfx_result suspend(Destination& destination) {
            return helpers::suspender<Destination,Destination::caps::suspend,false>::suspend(destination);
        }
        // resumes a suspended destination
        template<typename Destination>
        static gfx_result resume(Destination& destination,bool force=false) {
            return helpers::suspender<Destination,Destination::caps::suspend,false>::resume(destination,force);
        }
        // asynchronously suspends drawing for a destination that supports it
        template<typename Destination>
        static gfx_result suspend_async(Destination& destination) {
            return helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async>::suspend_async(destination);
        }
        // asynchronously resumes a suspended destination
        template<typename Destination>
        static gfx_result resume_async(Destination& destination,bool force=false) {
            return helpers::suspender<Destination,Destination::caps::suspend,Destination::caps::async>::resume_async(destination);
        }  
    };
    template<typename Destination>
    struct suspend_token final {
        Destination* destination;
        bool async;
        inline suspend_token(Destination& dest,bool async=false) : destination(&dest),async(async) {
            if(async) {
                draw::suspend_async(*destination);
                return;
            }
            draw::suspend(*destination);
        }
        inline suspend_token(const suspend_token& rhs) {
            destination = rhs.destination;
            if(async) {
                draw::suspend_async(*destination);
                return;
            }
            draw::suspend(*destination);
        }
        inline suspend_token& operator=(const suspend_token& rhs) {
            destination = rhs.destination;
            if(nullptr!=destination) {
                if(async) {
                    draw::suspend_async(*destination);
                    return *this;
                }
                draw::suspend(*destination);
            }
            return *this;
        }
        inline suspend_token(const suspend_token&& rhs) : destination(rhs.destination),async(rhs.async) {
            rhs.destination = nullptr;
        }
        inline suspend_token& operator=(suspend_token&& rhs) {
            destination = rhs.destination;
            async = rhs.async;
            rhs.destination = nullptr;
            return *this;
        }
        inline ~suspend_token() {
            if(nullptr!=destination) {
                if(async) {
                    draw::resume_async(*destination);
                    return;
                }
                draw::resume(*destination);
            }
        }    
    };
    // changes the target's pixel type to an intermediary pixel type and back again. Useful for downsampling or converting to grayscale
    template<typename TargetType,typename PixelType> 
    gfx_result resample(TargetType& target) {
        // suspend if supported
        suspend_token<TargetType> sustok(target);
        size16 dim = target.dimensions();
        point16 pt;
        gfx_result r;
        for(pt.y = 0;pt.y<dim.width;++pt.y) {
            for(pt.x=0;pt.x<dim.height;++pt.x) {
                typename TargetType::pixel_type px;
                r=target.point(pt,&px);
                if(gfx_result::success!=r) {
                    return r;
                }
                PixelType dpx;
                r= convert_palette_to(target,px,&dpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert_palette_to(target,dpx,&px);
                if(gfx_result::success!=r) {
                    return r;
                }
                r = target.point(pt,px);
                if(r!=gfx_result::success) {
                    return r;
                }
            }
        }
        return gfx_result::success;
    }
}
#endif