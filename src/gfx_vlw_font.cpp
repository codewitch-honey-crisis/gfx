#include <gfx_vlw_font.hpp>
#include <gfx_bitmap.hpp>
#include <gfx_encoding.hpp>
using namespace gfx;

static uint8_t* vlw_font_bitmap_buffer;
static size16 vlw_font_bitmap_buffer_size;

void vlw_font::do_move(vlw_font& rhs) {
    m_stream = rhs.m_stream;
    rhs.m_stream = nullptr;

    m_glyph_count = rhs.m_glyph_count;
    rhs.m_glyph_count = 0;
    m_y_advance = rhs.m_y_advance;
    m_space_width = rhs.m_space_width;
    m_ascent = rhs.m_ascent;
    m_descent = rhs.m_descent;
    m_max_ascent = rhs.m_max_ascent;
    m_max_descent = rhs.m_max_descent;
    m_bmp_size_max = rhs.m_bmp_size_max;
    m_allocator = rhs.m_allocator;
    m_deallocator = rhs.m_deallocator;
}
vlw_font::vlw_font() : m_stream(nullptr),m_allocator(nullptr),m_deallocator(nullptr),m_glyph_count(0),m_y_advance(0),m_space_width(0),m_ascent(0),m_descent(0),m_max_ascent(0),m_max_descent(0),m_bmp_size_max(0,0) {

}
vlw_font::vlw_font(stream* stream, void*(allocator)(size_t), void(deallocator)(void*)) : m_stream(stream),m_allocator(allocator),m_deallocator(deallocator),m_glyph_count(0),m_y_advance(0),m_space_width(0),m_ascent(0),m_descent(0),m_max_ascent(0),m_max_descent(0),m_bmp_size_max(0,0) {
    open(stream,this,allocator,deallocator);
}
vlw_font::vlw_font(vlw_font&& rhs) {
    do_move(rhs);
}
vlw_font& vlw_font::operator=(vlw_font&& rhs) {
    do_move(rhs);
    return *this;
}
size_t vlw_font::glyph_count() const {
    return m_glyph_count;
}
uint16_t vlw_font::y_advance() const {
    return m_y_advance;
}
uint16_t vlw_font::space_width() const {
    return m_space_width;
}
int16_t vlw_font::max_ascent() const {
    return m_max_ascent;
}
int16_t vlw_font::max_descent() const {
    return m_max_descent;
}
gfx_result vlw_font::read_uint32(stream* stm,uint32_t* out_result) {
    uint8_t tmp;
    if(1!=stm->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out_result = tmp << 24;
    if(1!=stm->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out_result |= tmp << 16;
    if(1!=stm->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out_result |= tmp << 8;
    if(1!=stm->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out_result |= tmp;
    return gfx_result::success;
}
gfx_result vlw_font::read_data(stream* stm,uint8_t* buffer, size_t* in_out_size) {
    *in_out_size=stm->read(buffer,*in_out_size);
    return gfx_result::success;
}
gfx_result vlw_font::seek_data(stream* stm, unsigned long long position, seek_origin origin) {
    stm->seek(position, origin);
    return gfx_result::success;
}
gfx_result vlw_font::read_glyph(stream* stm,glyph* out_glyph) {
    uint32_t tmp;
    gfx_result result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->codepoint = tmp;
    result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->size.height = (uint16_t)tmp;   
    result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->size.width = (uint16_t)tmp;   
    result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->x_advance = (uint16_t)tmp;
    result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->y_delta = tmp;
    result = read_uint32(stm,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    out_glyph->x_delta = tmp;
    return gfx_result::success;
}
gfx_result vlw_font::read_metrics() {
    size_t i = 0;
    seek_data(m_stream,24);
    while(i++<m_glyph_count) {
        uint32_t cp_cmp;
        gfx_result result = read_uint32(m_stream,&cp_cmp);
        if(result!=gfx_result::success) {
            return result;
        }
        if (((cp_cmp > 0x20) && (cp_cmp < 0xA0) && (cp_cmp != 0x7F)) || (cp_cmp > 0xFF)) {
            uint32_t h,w;
            result = read_uint32(m_stream,&h);
            if(result!=gfx_result::success) {
                return result;
            }
            result = read_uint32(m_stream,&w);
            if(result!=gfx_result::success) {
                return result;
            }
            m_stream->seek(4,seek_origin::current);
            uint32_t dy;
            result = read_uint32(m_stream,&dy);
            if(result!=gfx_result::success) {
                return result;
            }
            uint32_t md = h-dy;
            if(md>m_max_descent) {
                m_max_descent = md;
            }
            if(m_bmp_size_max.area()<(w*h)) {
                m_bmp_size_max = size16(w,h);
            }
            m_stream->seek(8,seek_origin::current);
        } else {
            m_stream->seek(24,seek_origin::current);
        }
    }
    m_y_advance = m_max_ascent + m_max_descent;
    m_space_width = (m_ascent + m_descent) * 2/7;
    return gfx_result::success;
}
gfx_result vlw_font::read_stream() {
    if(m_stream==nullptr) {
        return gfx_result::invalid_state;
    }
    if(!m_stream->caps().read || !m_stream->caps().seek) {
        return gfx_result::not_supported;
    }
    uint32_t tmp;
    gfx_result result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    m_glyph_count = (size_t)tmp;
    result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    m_y_advance = (uint16_t)tmp;
    result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    m_ascent = (uint16_t)tmp;
    result = read_uint32(m_stream,&tmp);
    if(result!=gfx_result::success) {
        return result;
    }
    m_descent = (uint16_t)tmp;
    m_stream->seek(4,seek_origin::current);
    m_max_ascent = m_ascent;
    m_max_descent = m_descent;
    m_y_advance = m_ascent+m_descent;
    m_space_width = m_y_advance/4;  
    gfx_result res =  read_metrics();
    if(res!=gfx_result::success) {
        return res;
    }
    return gfx_result::success;
    
}

gfx_result vlw_font::get_char_data(uint32_t codepoint, glyph* out_glyph, uint32_t *out_bitmap_offset) const {
    uint32_t bmp_ofs = 24 + (m_glyph_count*28);
    size_t i = 0;
    seek_data(m_stream,24);
    while(i<m_glyph_count) {
        uint32_t cp_cmp;
        gfx_result result = read_uint32(m_stream,&cp_cmp);
        if(result!=gfx_result::success) {
            return result;
        }
        if(cp_cmp==codepoint) {
            uint32_t glyph_offset = 24+(i*28);
            seek_data(m_stream,glyph_offset);
            read_glyph(m_stream,out_glyph);
            *out_bitmap_offset = bmp_ofs;
            return gfx_result::success;
        }
        uint32_t h,w;
        result = read_uint32(m_stream,&h);
        if(result!=gfx_result::success) {
            return result;
        }
        result = read_uint32(m_stream,&w);
        if(result!=gfx_result::success) {
            return result;
        }
        seek_data(m_stream,16,seek_origin::current);
        bmp_ofs += (w*h);
        ++i;
    }
    return gfx_result::invalid_argument;
}
gfx_result vlw_font::find_glyph(stream* stm,size_t glyph_count,uint32_t codepoint) {
    size_t i = 0;
    stm->seek(24);
    while(i<glyph_count) {
        uint32_t cp_cmp;
        gfx_result result = read_uint32(stm,&cp_cmp);
        if(result!=gfx_result::success) {
            return result;
        }
        if(cp_cmp==codepoint) {
            stm->seek(-4,seek_origin::current);
            return gfx_result::success;
        }
    }
    return gfx_result::invalid_argument;
    /*
    // binary search - doesn't work for some reason
    uint32_t left,tmp,right,ofs;
    left = 0;
    right = glyph_count - 1;
    while(left<right) {
        uint32_t middle = left + (right-left)/2;
        // seek to the middle glyph
        ofs = 24+(middle*28);
        seek_data(stm,ofs);
        gfx_result result = read_uint32(stm,&tmp);
        //Serial.print((char)tmp);
        if(result!=gfx_result::success) {
            stm->seek(-4,seek_origin::current);
            return result;
        }
        if(tmp==codepoint) {
            return gfx_result::success;
        }
        if(tmp<codepoint) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    return gfx_result::invalid_argument;
    */
}
ssize16 vlw_font::measure_text(
            ssize16 dest_size,
            const char* text,
            unsigned int tab_width, gfx_encoding encoding) const {
    if(nullptr==text || 0==*text || nullptr==m_stream) {
        return ssize16(0,0);
    }   
        uint16_t x= 0, y=0,x_ext=0,y_ext=0;
        const char* sz = text;
        size_t szlen = strlen(sz);
        while(*sz) {
            uint32_t cp;
            size_t sz_adv =szlen;// to_utf32_codepoint(sz,szlen,&cp,encoding);
            gfx_result res = to_utf32(sz,&cp,&sz_adv,encoding);
            if(res!=gfx_result::success) {
                return ssize16(0,0);
            }
            int x_adv;
            bool nl = false,cr = false;
            if(cp==0xA0 || cp == ' ') {
                x_adv=m_space_width;
                 int ext = x+x_adv;
                 x_ext = x_ext<ext?ext:x_ext;
                y_ext = y+m_y_advance+1;
            } else if(cp=='\n') {
                x_adv = 0;
                nl = true;
                cr = true;
                y_ext = y+m_y_advance+m_max_descent;
                int ext = x+x_adv;
                x_ext = x_ext<ext?ext:x_ext;
            } else if(cp=='\r') {
                x_adv = 0;
                y_ext = y+m_y_advance+m_max_descent;
                int ext = x+x_adv;
                x_ext = x_ext<ext?ext:x_ext;
                cr = true;
            } else if(cp=='\t') {
                int ti = x/(tab_width*m_space_width);
                x_adv=((ti+1)*(tab_width*m_space_width))-x;
                y_ext = y+m_y_advance+m_max_descent;
                int ext = x+x_adv;
                x_ext = x_ext<ext?ext:x_ext;
            } else if(cp>0x20) {
                vlw_font::glyph g;
                gfx_result r=find_glyph(m_stream,m_glyph_count, cp);
                y_ext = y+m_y_advance+m_max_descent+g.y_delta;
                if(gfx_result::success!=r) {
                    x_adv = m_space_width;
                    x_ext = x+x_adv;
                    int ext = x+x_adv;
                    x_ext = x_ext<ext?ext:x_ext;
                    //Serial.print((char)cp);
                } else {
                    r=read_glyph(m_stream,&g);
                    if(gfx_result::success!=r) {
                        return ssize16(0,0);
                    }
                    int y_adj = m_max_ascent-g.y_delta;
                    y_ext = y+m_y_advance+y_adj;
                    x_adv = g.x_advance;
                    int ext = x+x_adv+g.size.width;
                    x_ext = x_ext<ext?ext:x_ext;
                    //Serial.print((char)cp);
                }
            } else {
                x_adv = 0;
            }
            int y_adv = 0;
            if(!cr) {
                if(x_adv+x>=dest_size.width) {
                    cr = true;
                    nl = true;
                }
            }
            if(nl) {
                y_adv = m_y_advance+1;
            }
            if(cr) {
                x=0;
            } else {
                x+=x_adv;
            }
            y+=y_adv;
            if(y>=dest_size.height) {
                break;
            }
            if(x_ext<x) {
                x_ext = x;
            }
            if(y_ext<y) {
                y_ext = y;
            }
            szlen-=sz_adv;
            sz+=sz_adv;   
        }
        return ssize16(x_ext,y_ext);
}
gfx_result vlw_font::draw(draw_callback draw_cb, const glyph& glyph, uint32_t bitmap_offset, void*state) const {
    if(draw_cb==nullptr || bitmap_offset<24+(m_glyph_count*28)) {
        return gfx_result::invalid_argument;
    }
    if(m_stream==nullptr || !m_stream->caps().read || !m_stream->caps().seek) {
        return gfx_result::invalid_state;
    }
    int y_adj = m_max_ascent-glyph.y_delta;
    using bmp_t = bitmap<alpha_pixel<8>>;
    if(vlw_font_bitmap_buffer==nullptr || vlw_font_bitmap_buffer_size.area()<glyph.size.area()) {
        for(int y = 0;y<glyph.size.height;++y) {
            for(int x = 0;x<glyph.size.width;++x) {
                uint8_t tmp;
                size_t s=1;
                // without the following two lines
                // the data is out of order
                uint32_t ofs = bitmap_offset + (x+y*glyph.size.width);
                seek_data(m_stream,ofs);
                read_data(m_stream,&tmp,&s);
                if(s!=1) return gfx_result::io_error;
                draw_cb(x+glyph.x_delta,y+y_adj,tmp,state);
            }
        }
        return gfx_result::success;
    }
    seek_data(m_stream,bitmap_offset);
    size_t size = bmp_t::sizeof_buffer(glyph.size);
    size_t size_cmp = size;
    gfx_result res = read_data(m_stream,vlw_font_bitmap_buffer,&size);
    if(res!=gfx_result::success) {
        return res;
    }
    if(size!=size_cmp) {
        return gfx_result::io_error;
    }
    for(int y = 0;y<glyph.size.height;++y) {
        for(int x = 0;x<glyph.size.width;++x) {
            uint32_t ofs = (x+y*glyph.size.width);
            draw_cb(x+glyph.x_delta,y+y_adj,*(vlw_font_bitmap_buffer+ofs),state);
        }
    }
    return gfx_result::success;
}
gfx_result vlw_font::open(stream* stream, vlw_font* out_font,void*(allocator)(size_t), void(deallocator)(void*)) {
    if(out_font == nullptr || stream==nullptr || allocator==nullptr || deallocator == nullptr || !stream->caps().read || !stream->caps().seek) {
        return gfx_result::invalid_argument;
    }
    out_font->m_stream = stream;
    out_font->m_allocator = allocator;
    out_font->m_deallocator = deallocator;
    return out_font->read_stream();
}
gfx_result vlw_font::start_draw() const {
    if(vlw_font_bitmap_buffer==nullptr) {
        vlw_font_bitmap_buffer = (uint8_t*)m_allocator(bitmap<alpha_pixel<8>>::sizeof_buffer(m_bmp_size_max));
        vlw_font_bitmap_buffer_size = m_bmp_size_max;
        // don't care if out of memory
    }
    return gfx_result::success;
}
gfx_result vlw_font::end_draw() const {
    if(vlw_font_bitmap_buffer!=nullptr) {
        m_deallocator(vlw_font_bitmap_buffer);
        vlw_font_bitmap_buffer = nullptr;
    }
    return gfx_result::success;
}
