#include <gfx_win_font.hpp>
namespace gfx {

win_font::win_font(gfx::stream& stream, size_t font_index, bool initialize) : m_stream(&stream), m_font_index(font_index), m_line_height(0),m_font_offset(-1) {
    if(initialize) {
        this->initialize();
    }
}
win_font::win_font() : m_stream(nullptr), m_line_height(0) {
}
win_font::~win_font() {
    this->deinitialize();
}
win_font::win_font(win_font&& rhs) : m_stream(rhs.m_stream), 
                                        m_font_index(rhs.m_font_index),
                                        m_line_height(rhs.m_line_height),
                                        m_width(rhs.m_width),
                                        m_font_offset(rhs.m_font_offset),
                                        m_char_table_offset(rhs.m_char_table_offset),
                                        m_char_table_len(rhs.m_char_table_len),
                                        m_first_char(rhs.m_first_char),
                                        m_last_char(rhs.m_last_char)
{

    rhs.m_stream = nullptr;
    rhs.m_font_offset = -1;
}
win_font& win_font::operator=(win_font&& rhs) {
    this->deinitialize();
    m_stream=rhs.m_stream;
    m_font_index=rhs.m_font_index;
    m_line_height=rhs.m_line_height;
    m_width=rhs.m_width;
    m_font_offset=rhs.m_font_offset;
    m_char_table_offset=rhs.m_char_table_offset;
    m_char_table_len=rhs.m_char_table_len;
    m_first_char=rhs.m_first_char;
    m_last_char=rhs.m_last_char;
    rhs.m_stream = nullptr;
    rhs.m_font_offset = -1;
    return *this;
}
gfx_result win_font::initialize() {
    if(m_font_offset>-1) {
        return gfx_result::success;
    }
    if(m_stream->caps().read==0) {
        return gfx_result::io_error;
    } 
    if(m_stream->caps().seek==0) {
        return gfx_result::invalid_argument;
    }
    m_stream->seek(0);
    char ch[2];
    m_stream->read((uint8_t*)ch,sizeof(ch));
    bool is_fon = false;
    if(ch[0]=='M' && ch[1]=='Z') {
        is_fon = true;
    } else {
        m_font_offset = 0;
    }
    if(is_fon) {
        m_stream->seek(0x3c);
        uint32_t neoff = bits::from_le(m_stream->read<uint32_t>());
        m_stream->seek(neoff);
        char sig[2];
        m_stream->read((uint8_t*)sig,sizeof(sig));
        if(sig[0]=='N' && sig[1]=='E') {
            if(0x24+neoff!=m_stream->seek(neoff+0x24)) {
                // Unexpected end of stream parsing NE
                return gfx_result::io_error;
            }
            uint16_t rtable=bits::from_le(m_stream->read<uint16_t>());
            long long rtablea=rtable+neoff;
            if(rtablea!=(long long)m_stream->seek(rtablea)) {
                // Unexpected end of stream parsing NE
                return gfx_result::io_error;
            }
            uint16_t shift=bits::from_le(m_stream->read<uint16_t>());
            size_t ii = 0;
            while(m_font_offset<0) {
                uint16_t rtype=bits::from_le(m_stream->read<uint16_t>());
                if(0==rtype)
                    break; // end of resource table
                uint16_t count=bits::from_le(m_stream->read<uint16_t>());
                // skip 4 bytes (reserved)
                m_stream->seek(4,seek_origin::current);
                int i;
                for(i=0;i<count;++i) {
                    uint32_t start=((uint32_t)bits::from_le(m_stream->read<uint16_t>()))<<shift;
                    // skip the size, we don't need it.
                    m_stream->seek(2,seek_origin::current);
                    if(0x8008==rtype) { // is font entry
                        if(ii==m_font_index) {
                            m_font_offset = start;
                            break;
                        }
                        ++ii;
                    }
                    if(m_font_offset>-1) {
                        break;
                    }
                    m_stream->seek(8,seek_origin::current);
                }
            }    
            if(m_font_offset<0) {
                // font index out of range
                return gfx_result::invalid_argument;
            }       
        } else if(sig[0]=='P' && sig[1]=='E') {
            // PE files don't work yet
            return gfx_result::not_supported;
        } else {
            // not an executable
            return gfx_result::invalid_argument;
        }
    }
    m_stream->seek(m_font_offset);
    uint16_t version = bits::from_le(m_stream->read<uint16_t>());
    m_stream->seek(m_font_offset+0x42);
    uint16_t ftype = bits::from_le(m_stream->read<uint16_t>());
	if(ftype & 1) {
		// Font is a vector font
		return gfx_result::not_supported;
    }
    m_stream->seek(m_font_offset+0x4A);
    //int16_t ascent = bits::from_le(m_stream->read<int16_t>());
	m_stream->seek(m_font_offset+0x58);
    m_line_height = bits::from_le(m_stream->read<uint16_t>());
    if(version==0x200) {
        m_char_table_offset = 0x76;
        m_char_table_len = 4;
    } else {
        m_char_table_offset = 0x94;
        m_char_table_len = 6;
    }
    m_stream->seek(m_font_offset+0x5f);
    uint8_t ba[2];
    m_stream->read(ba,sizeof(ba));
    m_first_char = (char)ba[0];
    m_last_char = (char)ba[1];

    return gfx_result::success;
}
gfx_result win_font::seek_char(char ch) const {
    if(m_char_table_offset==0) {
        return gfx_result::invalid_state;
    }
    uint8_t fc = (uint8_t)m_first_char;
    uint8_t lc = (uint8_t)m_last_char;
    uint8_t cmp = (uint8_t)ch;
    if(cmp<fc||cmp>lc) {
        // character not found
        return gfx_result::invalid_argument;
    }
    long long offs = m_char_table_offset + m_char_table_len * (cmp-fc);
    if(offs+m_font_offset!=(long long)m_stream->seek(offs+m_font_offset)) {
        // unexpected end of stream
        return gfx_result::io_error;
    }
    return gfx_result::success;
}
bool win_font::initialized() const {
    return m_font_offset > -1;
}
void win_font::deinitialize() {
    m_font_offset = -1;
}
uint16_t win_font::line_height() const {
    return m_line_height;
}
uint16_t win_font::line_advance() const {
    if(!initialized()) {
        return 0;
    }
    return m_line_height;
}
uint16_t win_font::base_line() const {
    if(!initialized()) {
        return 0;
    }
    return m_line_height;
}
gfx_result win_font::on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const {
    if(codepoint1<0||codepoint1>255) {
        return gfx_result::invalid_argument;
    }
    gfx_result res = seek_char((char)codepoint1);
    if(res!=gfx_result::success) {
        return res;
    }
    out_glyph_info->dimensions.width = bits::from_le(m_stream->read<uint16_t>());
    out_glyph_info->advance_width = out_glyph_info->dimensions.width;
    out_glyph_info->dimensions.height = m_line_height;
    out_glyph_info->offset = {0,0};
    return gfx_result::success;
}
gfx_result win_font::on_draw(bitmap<alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index) const {
    if(codepoint<0||codepoint>255) {
        return gfx_result::invalid_argument;
    }
    gfx_result res = seek_char((char)codepoint);
    if(res!=gfx_result::success) {
        return res;
    }
    uint16_t width = bits::from_le(m_stream->read<uint16_t>());
    size_t width_bytes = (width+7)/8;
    
    long long offs = (m_char_table_len==4)?bits::from_le(m_stream->read<uint16_t>()):bits::from_le(m_stream->read<uint32_t>());
    for(size_t j=0;j<m_line_height;++j) {
        uint32_t accum = 0;
        for (size_t i = 0; i < width_bytes; ++i) {
            unsigned long long bytepos = offs+i*m_line_height+j;
            
            m_stream->seek(bytepos+m_font_offset);
            accum<<=8;
            accum|=m_stream->read<uint8_t>();;
        }
        int shift = (width_bytes*8)-width;
        accum>>=shift;
        uint32_t m = ((uint32_t)1) << (width - 1);
        for(int i = 0;i<width;++i)    {
            if(accum & m) {
                destination.point(point16(i,j),alpha_pixel<8>(255));
            } else {
                destination.point(point16(i,j),alpha_pixel<8>(0));
            }
            accum<<=1;
        }
    }
    return gfx_result::success;
}
}