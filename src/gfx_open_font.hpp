#ifndef GFX_OPEN_FONT_HPP
#define GFX_OPEN_FONT_HPP
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype_htcw.h"
namespace gfx {
    struct draw;
    class open_font final {
        friend class draw;
        stbtt::stbtt_fontinfo m_info;
        open_font(const open_font& rhs)=delete;
        open_font& operator=(const open_font& rhs)=delete;
    public:
        open_font() {
            m_info.stream = nullptr;
        }
        // not the ideal way to load, but used for embedded fonts
        open_font(stream* stream) {
            m_info.stream = nullptr;
            if(nullptr==stream) return;
            stbtt::stbtt_InitFont(&m_info,stream,0);
        }
        
        open_font(open_font&& rhs) : m_info(rhs.m_info) {
            rhs.m_info.stream = nullptr;
        }
        open_font& operator=(open_font&& rhs) {
            m_info = rhs.m_info;
            rhs.m_info.stream = nullptr;
            return *this;
        }
        uint16_t ascent() const {
            if(nullptr==m_info.stream) return 0;
            int result = 0;
            stbtt::stbtt_GetFontVMetrics(&m_info,&result,nullptr,nullptr);
            return (uint16_t)result;
        }
        uint16_t descent() const {
            if(nullptr==m_info.stream) return 0;
            int result = 0;
            stbtt::stbtt_GetFontVMetrics(&m_info,nullptr,&result,nullptr);
            return (uint16_t)result;
        }
        uint16_t line_gap() const {
            if(nullptr==m_info.stream) return 0;
            int result = 0;
            stbtt::stbtt_GetFontVMetrics(&m_info,nullptr,nullptr,&result);
            return (uint16_t)result;
        }
        float scale(float pixel_height) const {
            if(nullptr==m_info.stream) return NAN;
            return stbtt::stbtt_ScaleForPixelHeight(&m_info,pixel_height);
        }
        uint16_t advance_width(int codepoint) const {
            if(nullptr==m_info.stream) return 0;
            int result;
            stbtt::stbtt_GetCodepointHMetrics(&m_info,codepoint,&result,nullptr);
            return (uint16_t)result;
        }
        inline float advance_width(int codepoint,float scale) const {
            if(nullptr==m_info.stream) return NAN;
            return advance_width(codepoint)*scale+.5;
        }
        uint16_t left_bearing(int codepoint) const {
            if(nullptr==m_info.stream) return 0;
            int result;
            stbtt::stbtt_GetCodepointHMetrics(&m_info,codepoint,nullptr,&result);
            return (uint16_t)result;
        }
        inline float left_bearing(int codepoint,float scale) const {
            if(nullptr==m_info.stream) return NAN;
            return left_bearing(codepoint)*scale+.5;
        }
        uint16_t kern_advance_width(int codepoint1,int codepoint2) const {
            if(nullptr==m_info.stream) return 0;
            return stbtt::stbtt_GetCodepointKernAdvance(&m_info,codepoint1,codepoint2);
        }
        inline float kern_advance_width(int codepoint1,int codepoint2,float scale) const {
            if(nullptr==m_info.stream) return NAN;
            return kern_advance_width(codepoint1,codepoint2)*scale+.5;
        }
        rect16 bounds(int codepoint,float scale=1.0,float x_shift=0.0,float y_shift=0.0) const {
            rect16 result;
            if(nullptr==m_info.stream) return result;
            int x1,y1,x2,y2;
            stbtt::stbtt_GetCodepointBitmapBoxSubpixel(&m_info,codepoint,scale,scale,x_shift,y_shift,&x1,&y1,&x2,&y2);
            return {(uint16_t)x1,(uint16_t)y1,(uint16_t)x2,(uint16_t)y2};
        }
        // measures the size of the text within the destination area
        ssize16 measure_text(
            ssize16 dest_size,
            spoint16 offset,
            const char* text,
            float scale=1.0,
            float scaled_tab_width=0.0) const {
            ssize16 result(0,0);
            if(nullptr==text || 0==*text || nullptr==m_info.stream)
                return result;
            int asc,dsc,lgap;
            float height;
            stbtt::stbtt_GetFontVMetrics(&m_info,&asc,&dsc,&lgap);
            int x1,y1,x2,y2;
            stbtt::stbtt_GetFontBoundingBox(&m_info,&x1,&y1,&x2,&y2);
            if(scaled_tab_width<=0.0) {
                scaled_tab_width = (x2-x1+1)*scale*4;
            }
            height = asc*scale;
            int advw,lsb,gi;
            float xpos=offset.x,ypos=offset.y;
            float x_extent=0,y_extent=0;
            bool adv_line = false;
            const char*sz=text;
            while(*sz) {
                if(*sz<32) {
                    int ti;
                    switch(*sz) {
                        case '\t':
                            ti=xpos/scaled_tab_width;
                            xpos=(ti+1)*scaled_tab_width;
                            if(xpos>=dest_size.width) {
                                adv_line=true;
                            }
                            break;
                        case '\r':
                            xpos = offset.x;
                            break;
                        case '\n':
                            adv_line=true;
                            break;
                    }
                    if(adv_line) {
                        ypos+=height;
                        xpos = offset.x;
                        if(height+ypos>=dest_size.height) {
                            return {(int16_t)(x_extent+1),(int16_t)(ypos+1)};
                        }                        
                        if(ypos+height>y_extent) {
                            y_extent=ypos+height;
                        }
                        ypos+=lgap*scale;
                    }
                    ++sz;
                    continue;
                }
                gi = stbtt::stbtt_FindGlyphIndex(&m_info,*sz);
                stbtt::stbtt_GetGlyphHMetrics(&m_info,gi,&advw,&lsb);
                stbtt::stbtt_GetGlyphBitmapBoxSubpixel(&m_info,gi,scale,scale,xpos-floor(xpos),0,&x1,&y1,&x2,&y2);
                float xe = x2-x1;
                adv_line = false;
                if(xe+xpos>=dest_size.width) {
                    ypos+=height;
                    xpos=offset.x;
                    adv_line = true;
                } 
                if(adv_line && height+ypos>=dest_size.height) {
                    return {(int16_t)(x_extent+1),(int16_t)(ypos+1)};
                }
                if(xpos+xe>x_extent) {
                    x_extent=xpos+xe;
                }
                if(ypos+height>y_extent+(lgap*scale)) {
                    y_extent=ypos+height;
                }
                xpos+=(advw*scale);    
                if(*(sz+1)) {
                    xpos+=(stbtt::stbtt_GetGlyphKernAdvance(&m_info,gi,stbtt::stbtt_FindGlyphIndex(&m_info,*(sz+1)))*scale);
                }
                if(adv_line) {
                    ypos+=lgap*scale;
                }
                ++sz;   
            }
            return {(int16_t)(ceil(x_extent)),(int16_t)(ceil(y_extent+abs(dsc*scale)))};
        }
        // opens a font stream. Note that the stream must remain open as long as the font is being used. The stream must be readable and seekable. This class does not close the stream
        // This always chooses the first font out of a font collection.
        static gfx_result open(stream* stream,open_font* out_result) {
            if(nullptr==stream || 
                nullptr==out_result ||
                !stream->caps().read ||
                !stream->caps().seek) {
                    return gfx_result::invalid_argument;
                }
            if(!stbtt::stbtt_InitFont(&out_result->m_info,stream,0)) {
                return gfx_result::invalid_format;
            }    
            return gfx_result::success;
        }
        

    };
}
#endif