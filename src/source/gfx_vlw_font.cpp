#include <gfx_vlw_font.hpp>
namespace gfx {
 
gfx_result vlw_font::read_uint32(uint32_t* out) const {
    uint8_t tmp;
    if(1!=m_stream->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out = uint32_t(tmp) << 24;
    if(1!=m_stream->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out |= uint32_t(tmp) << 16;
    if(1!=m_stream->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out |= uint32_t(tmp) << 8;
    if(1!=m_stream->read(&tmp,1)) {
        return gfx_result::io_error;
    }
    *out |= uint32_t(tmp);
    return gfx_result::success;
}
gfx_result vlw_font::seek_codepoint(int32_t codepoint, int32_t* out_glyph_index) const {
    size_t i = 0;
    m_stream->seek(24);
    while(i<m_glyph_count) {
        uint32_t cp_cmp;
        gfx_result res = read_uint32(&cp_cmp);
        if(res!=gfx_result::success) {
            return res;
        }
        if(((int32_t)cp_cmp)==codepoint) {
            m_stream->seek(-4,seek_origin::current);
            if(out_glyph_index) {
                *out_glyph_index = i;
            }
            return gfx_result::success;
        }
        m_stream->seek(24,seek_origin::current);
        ++i;
    }
    return gfx_result::invalid_argument;
}

vlw_font::vlw_font(gfx::stream& stream, bool initialize) : m_stream(&stream), m_glyph_count(0){
    if(initialize) {
        this->initialize();
    }
}
vlw_font::vlw_font() : m_stream(nullptr),m_glyph_count(0) {

}
vlw_font::~vlw_font() {
    deinitialize();
}
vlw_font::vlw_font(vlw_font&& rhs) {
    m_stream=rhs.m_stream;
    m_glyph_count=rhs.m_glyph_count;
    m_line_advance=rhs.m_line_advance;
    m_space_width=rhs.m_space_width;
    m_ascent=rhs.m_ascent;
    m_descent=rhs.m_descent;
    m_max_ascent=rhs.m_max_ascent;
    m_max_descent=rhs.m_max_descent;
    m_bmp_size_max=rhs.m_bmp_size_max;
    rhs.m_stream = nullptr;
    rhs.m_glyph_count = 0;
}
vlw_font& vlw_font::operator=(vlw_font&& rhs) {
    m_stream=rhs.m_stream;
    m_glyph_count=rhs.m_glyph_count;
    m_line_advance=rhs.m_line_advance;
    m_space_width=rhs.m_space_width;
    m_ascent=rhs.m_ascent;
    m_descent=rhs.m_descent;
    m_max_ascent=rhs.m_max_ascent;
    m_max_descent=rhs.m_max_descent;
    m_bmp_size_max=rhs.m_bmp_size_max;
    rhs.m_stream = nullptr;
    rhs.m_glyph_count = 0;
    return *this;
}
gfx_result vlw_font::initialize() {
    if(m_stream==nullptr) {
        return gfx_result::invalid_state;
    }
    if(!m_stream->caps().read || !m_stream->caps().seek) {
        return gfx_result::invalid_argument;
    }
    size_t i = 0;
    m_stream->seek(0);
    //uint8_t tmp;
    uint32_t tmp32;
    gfx_result res = read_uint32(&tmp32);
    if(res!=gfx_result::success) {
        return res;
    }
    m_bmp_size_max = {0,0};
    m_glyph_count = tmp32;
    m_stream->seek(4,seek_origin::current);
    read_uint32(&tmp32);
    m_line_advance = tmp32;
    m_stream->seek(4,seek_origin::current);
    read_uint32(&tmp32);
    m_ascent = tmp32;
    read_uint32(&tmp32);
    m_descent = tmp32;
    m_max_ascent = m_ascent;
    m_max_descent = m_descent;
    
    m_stream->seek(24);
    while(i++<m_glyph_count) {
        uint32_t cp_cmp;
        res = read_uint32(&cp_cmp);
        if(res!=gfx_result::success) {
            return res;
        }
        if (((cp_cmp > 0x20) && (cp_cmp < 0xA0) && (cp_cmp != 0x7F)) || (cp_cmp > 0xFF)) {
            uint32_t h,w;
            res = read_uint32(&h);
            if(res!=gfx_result::success) {
                return res;
            }
            res = read_uint32(&w);
            if(res!=gfx_result::success) {
                return res;
            }
            m_stream->seek(4,seek_origin::current);
            int dy;
            res = read_uint32(&tmp32);
            dy=tmp32;
            if(res!=gfx_result::success) {
                return res;
            }
            int md = h-dy;
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
    m_line_advance = m_ascent + m_descent;
    //printf("line_advance: %d\n",(int)m_line_advance);
    m_space_width = (m_ascent + m_descent) * 2/7;
    return gfx_result::success;
}
bool vlw_font::initialized() const {
    return m_glyph_count>0;
}
void vlw_font::deinitialize() {
    m_glyph_count = 0;
}
uint16_t vlw_font::line_height() const {
    if(!initialized()) {
        return 0;
    }
    return m_bmp_size_max.height;
}
uint16_t vlw_font::line_advance() const {
    if(!initialized()) {
        return 0;
    }
    return m_line_advance;
}
uint16_t vlw_font::base_line() const {
    if(!initialized()) {
        return 0;
    }
    return m_bmp_size_max.height;
}

gfx_result vlw_font::on_measure(int32_t codepoint1,int32_t codepoint2, font_glyph_info* out_glyph_info) const{
    if(!initialized()) {
        return gfx_result::invalid_state;
    }
    if (((codepoint1 > 0x20) && (codepoint1 < 0xA0) && (codepoint1 != 0x7F)) || (codepoint1 > 0xFF)) {
        int32_t glyph_index;
        gfx_result res = this->seek_codepoint(codepoint1,&glyph_index);
        if(res!=gfx_result::success) {
            return res;
        }
        m_stream->seek(4,seek_origin::current);
        uint32_t tmp;
        res=read_uint32(&tmp);
        if(res!=gfx_result::success) {
            return res;
        }
        out_glyph_info->dimensions.height = tmp;
        res=read_uint32(&tmp);
        if(res!=gfx_result::success) {
            return res;
        }
        out_glyph_info->dimensions.width = tmp;
        res=read_uint32(&tmp);
        if(res!=gfx_result::success) {
            return res;
        }
        out_glyph_info->advance_width = tmp;
        res=read_uint32(&tmp);
        if(res!=gfx_result::success) {
            return res;
        }
        out_glyph_info->offset.y = m_line_advance-tmp;
        res=read_uint32(&tmp);
        if(res!=gfx_result::success) {
            return res;
        }
        out_glyph_info->offset.x = tmp;
        out_glyph_info->glyph_index1 = glyph_index;
    } else {
        out_glyph_info->dimensions.width = m_space_width;
        out_glyph_info->dimensions.height = m_bmp_size_max.height;
        out_glyph_info->advance_width = m_space_width;
        out_glyph_info->offset = {0,0};
    }
    return gfx_result::success;
}
gfx_result vlw_font::on_draw(bitmap<alpha_pixel<8>>& destination,int32_t codepoint, int32_t glyph_index) const {
    if(!initialized()) {
        return gfx_result::invalid_state;
    }
    if (((codepoint > 0x20) && (codepoint < 0xA0) && (codepoint != 0x7F)) || (codepoint > 0xFF)) {
    
        if(glyph_index<0) {
            gfx_result res = this->seek_codepoint(codepoint,&glyph_index);
            if(res!=gfx_result::success) {
                return res;
            }
        }
        long long bmp_ptr = 24 + (m_glyph_count*28);
        size_t i = 0;
        m_stream->seek(24);
        long long bmp_offset=0;
        uint32_t w,h;
        while(i<m_glyph_count) {
            uint32_t cp_cmp;
            gfx_result res=read_uint32(&cp_cmp);
            if(res!=gfx_result::success) {
                return res;
            }
            long long glyph_offset = 24+(i*28);
            m_stream->seek(glyph_offset+4);
            
            res=read_uint32(&h);
            if(res!=gfx_result::success) {
                return res;
            }
            res=read_uint32(&w);
            if(res!=gfx_result::success) {
                return res;
            }
            if(((int32_t)cp_cmp)==codepoint) {
                bmp_offset = bmp_ptr;
                break;
            } else {
                bmp_offset+=(w*h);
            }
            bmp_ptr += (w*h);
            m_stream->seek(16,seek_origin::current);
            ++i;
        }
        if(i>=m_glyph_count) {
            return gfx_result::invalid_argument;
        }
        if(bmp_offset!=(long long)m_stream->seek(bmp_offset)) {
            return gfx_result::io_error;
        }
        if((w*h)!=m_stream->read(destination.begin(),w*h)) {
            return gfx_result::io_error;
        }
    } else {
        destination.fill(destination.bounds(),alpha_pixel<8>(0));
    }
    return gfx_result::success;
}
}