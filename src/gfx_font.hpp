#ifndef HTCW_GFX_FONT_HPP
#define HTCW_GFX_FONT_HPP
#include <stdlib.h>
#include "bits.hpp"
#include "stream.hpp"
#include "gfx_positioning.hpp"
namespace gfx {
    struct font_style {
        int italic : 1;
        int underline :1;
        int strikeout :1;
    };
    struct font;
    // represents a character entry in a font
    class font_char final {
        friend struct font;
        uint16_t m_width;
        const uint8_t* m_data;
    public:
        // retrieves the width
        inline uint16_t width() const {
            return m_width;
        }
        // retrieves the height
        inline const uint8_t* data() const {
            return m_data;
        }
    };
    // represents a font
    struct font final { 
        // the result of an operation  
        enum struct result {
            // completed successfully
            success = 0,
            // io error while reading or seeking
            io_error = 1,
            // the stream is not a font format
            invalid_format = 2,
            // the stream does not contain the MZ signature
            no_mz_signature = 3,
            // the stream does not contain an exe signature
            no_exe_signature = 4,
            // the stream cannot be seeked
            non_seekable_stream=5,
            // the stream cannot be read
            non_readable_stream=6,
            // more data was expected
            unexpected_end_of_stream=7,
            // the specified font index does not exist
            font_index_out_of_range=8,
            // an invalid argument was passed
            invalid_argument=9,
            // not enough memory to complete the operation
            out_of_memory=10,
            // vector fonts are not supported
            vector_font_not_supported=11
        };
    private:
        uint16_t m_height;
        uint16_t m_average_width;
        uint16_t m_point_size;
        uint16_t m_ascent;
        point16 m_resolution;
        char m_first_char;
        char m_last_char;
        char m_default_char;
        char m_break_char;
        font_style m_style;
        uint16_t m_weight;
        uint8_t m_charset;
        uint16_t m_internal_leading;
        uint16_t m_external_leading;
        // char data is, for each character from
        // m_first_char to m_last_char, one
        // uint16_t width, followed by encoded
        // font data
        const uint8_t* m_char_data;
        // if set, data will be freed on class destruction
        uint8_t* m_owned_data;
        
        static inline uint16_t order_guard(uint16_t value) {
            return bits::from_le(value);
        }
        static inline uint32_t order_guard(uint32_t value) {
            return bits::from_le(value);
        }
        static inline uint8_t order_guard(uint8_t value) {
            return bits::from_le(value);
        }
#if HTCW_MAX_WORD >= 64
        static inline uint64_t order_guard(uint64_t value) {
            return bits::from_le(value);
        }
#endif
        static result read_font_init(io::stream* stream, long long int* pos,uint16_t* ctstart,uint16_t* ctsize) {
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
        static result read_font(io::stream* stream,char first_char, char last_char, uint8_t* buffer,font* out_font,size_t* out_size) {
            long long int pos;
            uint16_t ctstart;
            uint16_t ctsize;
            if(nullptr==out_font) {
                if(nullptr==out_size) {
                    // nothing to do
                    return result::success;
                }
                read_font_init(stream,&pos,&ctstart,&ctsize);
                stream->seek(pos+0x58);
                uint16_t pxheight = order_guard(stream->template read<uint16_t>());
                stream->seek(pos+0x5F);
                uint8_t fc = (uint8_t)stream->getc();
                uint8_t lc = (uint8_t)stream->getc();
                uint8_t dc = (uint8_t)stream->getc()+fc;
                uint8_t bc = (uint8_t)stream->getc()+fc;
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
                return result::success;
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
            if(0<stream->getc()) {
                out_font->m_style.italic = 1;
            }
            if(0<stream->getc()) {
                out_font->m_style.underline = 1;
            }
            if(0<stream->getc()) {
                out_font->m_style.strikeout = 1;
            }
            out_font->m_weight = order_guard(stream->template read<uint16_t>());
            int gc = stream->getc();
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
            gc = stream->getc();
            if(0>gc)
                return result::unexpected_end_of_stream;
            out_font->m_first_char = (char)gc;
            gc = stream->getc();
            if(0>gc)
                return result::unexpected_end_of_stream;
            out_font->m_last_char = (char)gc;
            gc = stream->getc();
            if(0>gc)
                return result::unexpected_end_of_stream;
            out_font->m_default_char = (char)gc+out_font->m_first_char;
            gc = stream->getc();
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
        static result read_ne(uint32_t neoff,io::stream* stream,size_t index,char first_char,char last_char, uint8_t* buffer, font* out_font,size_t* out_size) {
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
        static result read_pe(uint32_t neoff, io::stream* stream,size_t index,char first_char, char last_char, uint8_t* buffer,font* out_font,size_t* out_size) {
            return result::io_error;
        }
        const uint8_t* char_data_ptr(char ch) const {
            if(nullptr==m_char_data)
                return nullptr;
            if((uint8_t)ch<(uint8_t)m_first_char || (uint8_t)ch>(uint8_t)m_last_char)
            {
                ch=m_default_char;
            }
            const uint8_t* data = m_char_data;
            for(size_t i = (uint8_t)m_first_char;i<(uint8_t)ch;++i) {
                uint16_t w = *((uint16_t*)data);
                data+=sizeof(w);
                size_t wb = (w+7)/8;
                data+=(wb*m_height);
            }
            return data;
        }
        font(const font& rhs)=delete;
        font& operator=(const font& rhs)=delete;
    public:
        // constructs an empty font. Not really usable yet
        font() : m_owned_data(nullptr) {

        }
        // constructs a font with the specified data
        font(
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
        // constructs a font with the specified stream
        font(io::stream* stream,size_t index=0, char first_char = '\0', char last_char='\xFF') {
            read(stream,this,index,first_char,last_char,nullptr);
        }
        // resource steals from another font
        font(font&& rhs) :
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
        // resource steals from another font
        font& operator=(font&& rhs) {
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
        ~font() {
            if(nullptr!=m_owned_data)
                free(m_owned_data);
        }
        // indicates the font height
        inline uint16_t height() const {
            return m_height;
        }
        // indicates the horizontal and vertical resolution in dots per inch
        inline point16 resolution() const {
            return m_resolution;
        }
        // indicates the internal leading
        inline uint16_t internal_leading() const {
            return m_internal_leading;
        }
        // indicates the external leading
        inline uint16_t external_leading() const {
            return m_external_leading;
        }
        // indicates the ascent which is the baseline of the font
        inline uint16_t ascent() const {
            return m_ascent;
        }
        // indicates the size of the font in points
        inline uint16_t point_size() const {
            return m_point_size;
        }
        // indicates the font style
        inline font_style style() const {
            return m_style;
        }
        // indicates the font weight
        inline uint16_t weight() const {
            return m_weight;
        }
        // indicates the first character represented by the font
        inline char first_char() const {
            return m_first_char;
        }
        // indicates the final character represented by the font
        inline char last_char() const {
            return m_last_char;
        }
        // indicates the character used if no other character could be mapped
        inline char default_char() const {
            return m_default_char;
        }
        // indicates the character used for word breaks (not currently used)
        inline char break_char() const {
            return m_break_char;
        }
        // indicates the character set code
        inline uint8_t charset() const {
            return m_charset;
        }
        // indicates the average width of the characters
        inline uint16_t average_width() const {
            return m_average_width;
        }
        // indicates the width of an individual character
        inline uint16_t width(char ch) const {
            const uint8_t* p = char_data_ptr(ch);
            if(nullptr!=p) {
                return *((uint16_t*)p);
            }
            return 0;
        }
        // retrieves information about the specified character
        const font_char operator[](int ch) const {
            const uint8_t* p= char_data_ptr((char)ch);
            font_char result;
            result.m_width = *((uint16_t*)p);
            p+=sizeof(uint16_t);
            result.m_data = p;
            return result;
        }
        // reads a font from a stream
        static result read(io::stream* stream, font* out_font,size_t index=0, char first_char = '\0',char last_char='\xFF',uint8_t* buffer=nullptr) {
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
        // measures the size of the text within the destination rectangle
        ssize16 measure_text(
            ssize16 dest_size,
            const char* text,
            unsigned int tab_width=4) const {
            ssize16 result(0,0);
            if(nullptr==text || 0==*text)
                return result;
            const char*sz=text;
            int cw;
            uint16_t rw;
            srect16 chr(spoint16(0,0),ssize16(width(*sz),height()));
            
            font_char fc = (*this)[*sz];
            while(*sz) {
                switch(*sz) {
                    case '\r':
                        chr.x1=0;
                        ++sz;
                        if(*sz) {
                            font_char nfc = (*this)[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            fc=nfc;
                        }  
                        if(chr.x2>=result.width)
                            result.width=chr.x2+1;
                        if(chr.y2>=result.height)
                            result.height=chr.y2+1;    
                        break;
                    case '\n':
                        chr.x1=0;
                        ++sz;
                        if(*sz) {
                            font_char nfc = (*this)[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            fc=nfc;
                        }
                        chr=chr.offset(0,height());
                        if(chr.y2>=dest_size.height) {    
                            return dest_size;
                        }
                        break;
                    case '\t':
                        ++sz;
                        if(*sz) {
                            font_char nfc = (*this)[*sz];
                            rw=chr.x1+nfc.width()-1;
                            fc=nfc;
                        } else
                            rw=chr.width();
                        cw = average_width()*tab_width;
                        chr.x1=((chr.x1/cw)+1)*cw;
                        chr.x2=chr.x1+rw-1;
                        if(chr.right()>=dest_size.width) {
                            chr.x1=0;
                            chr=chr.offset(0,height());
                        } 
                        if(chr.x2>=result.width)
                            result.width=chr.x2+1;
                        if(chr.y2>=result.height)
                            result.height=chr.y2+1;    
                        if(chr.y2>=dest_size.height)
                            return dest_size;
                        
                        break;
                    default:
                        chr=chr.offset(fc.width(),0);
                        ++sz;
                        if(*sz) {
                            font_char nfc = (*this)[*sz];
                            chr.x2=chr.x1+nfc.width()-1;
                            if(chr.right()>=dest_size.width) {
                                
                                chr.x1=0;
                                chr=chr.offset(0,height());
                            }
                            if(chr.x2>=result.width)
                                result.width=chr.x2+1;
                            if(chr.y2>=result.height)
                                result.height=chr.y2+1;    
                            if(chr.y2>dest_size.height)
                                return dest_size;
                            fc=nfc;
                        }
                        break;
                }
            }
            return result;
        }
    };
}
#endif