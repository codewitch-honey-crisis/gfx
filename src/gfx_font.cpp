#include <stdint.h>
#include <inttypes.h>
#include <gfx_font.hpp>
namespace gfx {
    font::font() : m_owned_data(nullptr) {

    }
    font::font(
        uint16_t height,
        uint16_t average_width,
        uint16_t point_size,
        uint16_t ascent,
        point16 dpi,
        char first_char,
        char last_char,
        char default_char,
        char break_char,
        font_style style,
        uint16_t weight,
        uint8_t charset,
        uint16_t internal_leading,
        uint16_t external_leading,
        // char data is, for each character from
        // first_char to last_char, one
        // uint16_t width, followed by encoded
        // font data
        const uint8_t* char_data
    ) :
        m_height(height),
        m_average_width(average_width),
        m_point_size(point_size),
        m_ascent(ascent),
        m_resolution(dpi),
        m_first_char(first_char),
        m_last_char(last_char),
        m_default_char(default_char),
        m_break_char(break_char),
        m_style(style),
        m_weight(weight),
        m_charset(charset),
        m_internal_leading(internal_leading),
        m_external_leading(external_leading),
        m_char_data(char_data),
        m_owned_data(nullptr)
    {
        
    }
    font::font(io::stream* stream,size_t index, char first_char, char last_char) {
        read(stream,this,index,first_char,last_char,nullptr);
    }
    font::font(font&& rhs) :
        m_height(rhs.m_height),
        m_average_width(rhs.m_average_width),
        m_point_size(rhs.m_point_size),
        m_ascent(rhs.m_ascent),
        m_resolution(rhs.m_resolution),
        m_first_char(rhs.m_first_char),
        m_last_char(rhs.m_last_char),
        m_default_char(rhs.m_default_char),
        m_break_char(rhs.m_break_char),
        m_style(rhs.m_style),
        m_weight(rhs.m_weight),
        m_charset(rhs.m_charset),
        m_internal_leading(rhs.m_internal_leading),
        m_external_leading(rhs.m_external_leading),
        m_char_data(rhs.m_char_data),
        m_owned_data(rhs.m_owned_data)
    {
        rhs.m_owned_data=nullptr;
    }
    font& font::operator=(font&& rhs) {
        if(nullptr!=m_owned_data)
            free(m_owned_data);
        m_height=rhs.m_height;
        m_average_width=rhs.m_average_width;
        m_point_size=rhs.m_point_size;
        m_ascent=rhs.m_ascent;
        m_resolution=rhs.m_resolution;
        m_first_char=rhs.m_first_char;
        m_last_char=rhs.m_last_char;
        m_default_char=rhs.m_default_char;
        m_break_char=rhs.m_break_char;
        m_style=rhs.m_style;
        m_weight=rhs.m_weight;
        m_charset=rhs.m_charset;
        m_internal_leading=rhs.m_internal_leading;
        m_external_leading=rhs.m_external_leading;
        m_char_data=rhs.m_char_data;
        m_owned_data=rhs.m_owned_data;
        rhs.m_owned_data=nullptr;
        return *this;
    }
    // destroys any memory created by the font
    font::~font() {
        if(nullptr!=m_owned_data)
            free(m_owned_data);
    }
    font::result font::read_font_init(io::stream* stream, long long int* pos,uint16_t* ctstart,uint16_t* ctsize) {
        *pos = stream->seek(0,io::seek_origin::current);
        uint16_t version = order_guard(stream->template read<uint16_t>());
        if(0x200==version) {
            *ctstart = 0x76;
            *ctsize = 4;
        } else {
            *ctstart = 0x94;
            *ctsize = 6;
        }    
        return result::success;
    }
    font::result font::read_font(io::stream* stream,char first_char, char last_char, uint8_t* buffer,font* out_font,size_t* out_size) {
        long long int pos;
        uint16_t ctstart;
        uint16_t ctsize;
        if(nullptr==out_font) {
            if(nullptr==out_size) {
                // nothing to do
                return font::result::success;
            }
            read_font_init(stream,&pos,&ctstart,&ctsize);
            stream->seek(pos+0x58);
            uint16_t pxheight = order_guard(stream->template read<uint16_t>());
            stream->seek(pos+0x5F);
            uint8_t fc = (uint8_t)stream->getch();
            uint8_t lc = (uint8_t)stream->getch();
            uint8_t dc = (uint8_t)stream->getch()+fc;
            uint8_t bc = (uint8_t)stream->getch()+fc;
            if((uint8_t)first_char<fc)
                first_char = fc;
            if(dc<(uint8_t)first_char) {
                first_char = dc;
            }
            if(bc<(uint8_t)first_char) {
                first_char = bc;
            }
            
            if((uint8_t)last_char>lc)
                last_char = lc;
            if(dc>(uint8_t)last_char) {
                last_char = dc;
            }
            if(bc>(uint8_t)last_char) {
                last_char = bc;
            }
            size_t size = 0;
            for(size_t i = (uint8_t)first_char;i<=(uint8_t)last_char;++i) {
                size_t entry =ctstart+ctsize*(i-fc);
                stream->seek(pos+entry);
                uint16_t w = order_guard(stream->template read<uint16_t>());
                size+=2; // 2 bytes for width
                size_t wb = (w+7)/8;
                // size of char table
                size += (wb*pxheight);
            }
            *out_size=size;
            return font::result::success;
        }
        out_font->m_char_data = buffer;
        read_font_init(stream,&pos,&ctstart,&ctsize);
        stream->seek(pos+66);
        uint16_t ftype = order_guard(stream->template read<uint16_t>());
        if(ftype&1) {
            return result::vector_font_not_supported;
        }
        out_font->m_point_size = order_guard(stream->template read<uint16_t>());
        out_font->m_resolution.y =  order_guard(stream->template read<uint16_t>());
        out_font->m_resolution.x =  order_guard(stream->template read<uint16_t>());
        out_font->m_ascent =  order_guard(stream->template read<uint16_t>());
        out_font->m_internal_leading=order_guard(stream->template read<uint16_t>());
        out_font->m_external_leading=order_guard(stream->template read<uint16_t>());
        out_font->m_style = {0,0,0};
        if(0<stream->getch()) {
            out_font->m_style.italic = 1;
        }
        if(0<stream->getch()) {
            out_font->m_style.underline = 1;
        }
        if(0<stream->getch()) {
            out_font->m_style.strikeout = 1;
        }
        out_font->m_weight = order_guard(stream->template read<uint16_t>());
        int gc = stream->getch();
        if(0>gc)
            return result::unexpected_end_of_stream;
        out_font->m_charset = (uint8_t)gc;
        out_font->m_average_width=order_guard(stream->template read<uint16_t>());
        out_font->m_height = order_guard(stream->template read<uint16_t>());
        // skip pitch and family
        stream->seek(1,io::seek_origin::current);
        if(0==out_font->m_average_width)
            out_font->m_average_width=order_guard(stream->template read<uint16_t>());
        else
            stream->seek(2,io::seek_origin::current);    
        // skip max_width
        stream->seek(2,io::seek_origin::current);
        gc = stream->getch();
        if(0>gc)
            return result::unexpected_end_of_stream;
        out_font->m_first_char = (char)gc;
        gc = stream->getch();
        if(0>gc)
            return result::unexpected_end_of_stream;
        out_font->m_last_char = (char)gc;
        gc = stream->getch();
        if(0>gc)
            return result::unexpected_end_of_stream;
        out_font->m_default_char = (char)gc+out_font->m_first_char;
        gc = stream->getch();
        if(0>gc)
            return result::unexpected_end_of_stream;
        out_font->m_break_char = (char)gc+out_font->m_first_char;
        size_t size = 0;
        
        if((uint8_t)first_char<(uint8_t)out_font->m_first_char)
            first_char = out_font->m_first_char;
        if((uint8_t)out_font->m_default_char<(uint8_t)first_char) {
            first_char = out_font->m_default_char;
        }
        if((uint8_t)out_font->m_break_char<(uint8_t)first_char) {
            first_char = out_font->m_break_char;
        }
        
        if((uint8_t)last_char>(uint8_t)out_font->m_last_char)
            last_char = out_font->m_last_char;
        if((uint8_t)out_font->m_default_char>(uint8_t)last_char) {
            last_char = out_font->m_default_char;
        }
        if((uint8_t)out_font->m_break_char>(uint8_t)last_char) {
            last_char = out_font->m_break_char;
        }
        for(size_t i = (uint8_t)first_char;i<=(uint8_t)last_char;++i) {
            size_t entry =ctstart+ctsize*(i-out_font->m_first_char);
            stream->seek(pos+entry);
            uint16_t w = order_guard(stream->template read<uint16_t>());
            size+=2; // 2 bytes for width
            size_t wb = (w+7)/8;
            // size of char table
            size += (wb*out_font->m_height);
            uint32_t off;
            if(ctsize==4) {
                off = order_guard(stream->template read<uint16_t>());
            } else {
                off = order_guard(stream->template read<uint32_t>());
            }
            stream->seek(off+pos);
            *((uint16_t*)buffer)=w;
            buffer+=sizeof(uint16_t);
            for(size_t j=0;j<out_font->m_height;++j) {
                bits::uint_max accum = 0;
                for(size_t k=0;k<wb;++k) {
                    uint32_t bytepos = off+k*out_font->m_height+j;
                    accum<<=8;
                    stream->seek(pos+bytepos);
                    uint8_t b;
                    if(1!=stream->read(&b,1))
                        return result::unexpected_end_of_stream;
                    accum|=b;
                }
                accum>>=(8*wb-w);
                // TODO: Ensure this is correct on big endian machines
                accum = order_guard(accum);
                memcpy(buffer,((uint8_t*)&accum),wb);
                buffer+=wb;
            }
        }
        out_font->m_first_char = first_char;
        out_font->m_last_char = last_char;
        if(out_size!=nullptr)
            *out_size=size;
        
        return result::success;
    }
    font::result font::read_ne(uint32_t neoff,io::stream* stream,size_t index,char first_char,char last_char, uint8_t* buffer, font* out_font,size_t* out_size) {
        if(0x24+neoff!=stream->seek(neoff+0x24)) {
            return result::unexpected_end_of_stream;
        }
        uint16_t rtable=order_guard(stream->template read<uint16_t>());
        uint32_t rtablea=rtable+neoff;
        if(rtablea!=stream->seek(rtablea)) {
            return result::unexpected_end_of_stream;
        }
        uint16_t shift=order_guard(stream->template read<uint16_t>());
        size_t ii = 0;
        while(true) {
            uint16_t rtype=order_guard(stream->template read<uint16_t>());
            if(0==rtype)
                break; // end of resource table
            uint16_t count=order_guard(stream->template read<uint16_t>());
            // skip 4 bytes (reserved)
            stream->seek(4,io::seek_origin::current);
            for(int i=0;i<count;++i) {
                uint32_t start=order_guard(stream->template read<uint16_t>())<<shift;
                // skip the size, we don't need it.
                stream->seek(2,io::seek_origin::current);
                if(0x8008==rtype) { // is font entry
                    if(ii==index) {
                        stream->seek(start);
                        return read_font(stream,first_char,last_char,buffer,out_font,out_size);
                    }
                    ++ii;
                }
                stream->seek(8,io::seek_origin::current);
            }
        }
        return result::font_index_out_of_range;
    }
    const uint8_t* font::char_data_ptr(char ch) const {
        if(nullptr==m_char_data)
            return nullptr;
        if((uint8_t)ch<(uint8_t)m_first_char || (uint8_t)ch>(uint8_t)m_last_char)
        {
            ch=m_default_char;
        }
        const uint8_t* data = m_char_data;
        for(size_t i = (uint8_t)m_first_char;i<(uint8_t)ch;++i) {
            uint16_t w = pgm_read_byte((uint16_t*)data);
            data+=sizeof(w);
            size_t wb = (w+7)/8;
            data+=(wb*m_height);
        }
        return data;
    }
    uint16_t font::width(char ch) const {
        const uint8_t* p = char_data_ptr(ch);
        if(nullptr!=p) {
            return pgm_read_byte((uint16_t*)p);
        }
        return 0;
    }
    // retrieves information about the specified character
    const font_char font::operator[](int ch) const {
        const uint8_t* p= char_data_ptr((char)ch);
        font_char result;
        result.m_width = pgm_read_byte((uint16_t*)p);
        p+=sizeof(uint16_t);
        result.m_data = p;
        return result;
    }
    font::result font::read(io::stream* stream, font* out_font,size_t index, char first_char,char last_char,uint8_t* buffer) {
        uint8_t mz[2];
        io::stream_caps scaps=stream->caps();
        if(nullptr==out_font)
            return result::invalid_argument;
        if(!scaps.seek) 
            return result::non_seekable_stream;
        if(!scaps.read) 
            return result::non_readable_stream;
        
        if(0!=stream->seek(0)) {
            return result::io_error;
        }
        if(2!=stream->read(mz,2) || 'M'!=mz[0] || 'Z'!=mz[1]) {
            return result::no_mz_signature;
        }
        if(0x3C!=stream->seek(0x3C)) {
            return result::no_exe_signature;
        }
        uint32_t neoff;
        if(4!=stream->read((uint8_t*)&neoff,4)) {
            return result::no_exe_signature;
        }
        if(neoff!=stream->seek(neoff)) {
            return result::no_exe_signature;
        }
        uint8_t sig[2];
        if(2!=stream->read(sig,2)) {
            return result::no_exe_signature;
        }
        if('N'==sig[0] && 'E'==sig[1]) {
            bool own = false;
            if(nullptr==buffer) {
                // bookmark our position and then get the size
                size_t size;
                auto pos = stream->seek(0,io::seek_origin::current);
                result r = read_ne(neoff,stream,index,first_char,last_char,nullptr, nullptr,&size);
                if(result::success!=r) {
                    return r;
                }
                if(pos!=stream->seek(pos)) 
                    return result::io_error;
                // now alloc the buffer
                buffer = (uint8_t*)malloc(size);
                if(nullptr==buffer)
                    return result::out_of_memory;
                own = true;
            }
            result rr = read_ne(neoff,stream,index,first_char,last_char,buffer, out_font,nullptr);
            if(result::success==rr) {
                if(own) {
                    out_font->m_owned_data = buffer;
                } else
                    out_font->m_owned_data=nullptr;
            }
            return rr;
        } else if('P'==sig[0] && 'E'==sig[1]) {
            if(2!=stream->read(sig,2)) {
                return result::no_exe_signature;
            }
            if(0==sig[0] && 0==sig[1]) {
                return read_pe(neoff,stream,index,first_char,last_char,buffer,out_font,nullptr);
            }
        }
        return result::no_exe_signature;
    }
    ssize16 font::measure_text(
        ssize16 dest_size,
        const char* text,
        unsigned int tab_width) const {
        if(nullptr==text || 0==*text) {
            return {0,0};
        }
        int mw = 0;
        int w = 0;
        int h = 0;
        int cw;
        const char*sz=text;
        while(*sz) {
            if(h==0) {
                h=height();
            }
            font_char fc = (*this)[*sz];
            switch(*sz) {
                case '\n':
                    if(w>mw) {
                        mw = w;
                    }
                    h+=height();
                    w=0;
                    break;
                case '\r':
                    if(w>mw) {
                        mw = w;
                    }
                    w=0;
                    break;
                case '\t':
                    cw = average_width()*tab_width;
                    w=((w/cw)+1)*cw;
                    if(w>dest_size.width) {
                        h+=height();
                        if(w>mw) {
                            mw = w;
                        }   
                        w=0;
                    }
                    break;
                default:
                    w+=fc.width();
                    if(w>dest_size.width) {
                        h+=height();
                        w=fc.width();
                    }
                    if(w>mw) {
                        mw = w;
                    }   
                    break;
            }
            ++sz;
        }
        return gfx::ssize16(mw,h);
    }
}
