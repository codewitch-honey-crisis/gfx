#ifndef HTCW_GFX_VLW_FONT
#define HTCW_GFX_VLW_FONT
#include <stdlib.h>
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
namespace gfx {   
    class vlw_font final {
    public:
        struct glyph final {
            uint32_t codepoint;
            size16 size;
            uint16_t x_advance;
            int16_t y_delta;
            int8_t x_delta;
        };
        typedef void(*draw_callback)(int x,int y,int c,void* state);
    private:
        stream* m_stream;
        void*(*m_allocator)(size_t);
        void(*m_deallocator)(void*);
        size_t m_glyph_count;
        uint16_t m_y_advance;
        uint16_t m_space_width;
        int16_t m_ascent, m_descent;
        uint16_t m_max_ascent, m_max_descent;
        size16 m_bmp_size_max;
    private:
        vlw_font(const vlw_font& rhs)=delete;
        vlw_font& operator=(const vlw_font& rhs)=delete;
        void do_move(vlw_font& rhs);
        static gfx_result read_uint32(stream* stm,uint32_t *out_result);
        static gfx_result read_data(stream* stm,uint8_t* buffer, size_t* in_out_size);
        static gfx_result seek_data(stream* stm, unsigned long long position, seek_origin origin = seek_origin::start);
        gfx_result read_stream();
        gfx_result read_metrics();
        static gfx_result read_glyph(stream* stm,glyph* out_glyph);
        static gfx_result find_glyph(stream* stm, size_t glyph_count, uint32_t codepoint);
    public:
        vlw_font();
        vlw_font(stream* stream, void*(allocator)(size_t)=::malloc, void(deallocator)(void*)=::free);
        vlw_font(vlw_font&& rhs);
        vlw_font& operator=(vlw_font&& rhs);
        size_t glyph_count() const;
        uint16_t y_advance() const;
        uint16_t space_width() const;
        int16_t max_ascent() const;
        int16_t max_descent() const;
        gfx_result get_char_data(uint32_t codepoint, glyph *out_glyph, uint32_t *out_bitmap_offset) const;
        gfx_result start_draw() const;
        gfx_result end_draw() const;
        gfx_result draw(draw_callback draw_cb, const glyph& glyph, uint32_t bitmap_offset, void*state) const;
        ssize16 measure_text(
            ssize16 dest_size,
            const char* text,
            unsigned int tab_width=4,
            gfx_encoding encoding = gfx_encoding::utf8) const;
        static gfx_result open(stream* stream, vlw_font* out_font, void*(allocator)(size_t)=::malloc, void(deallocator)(void*)=::free);
    };
}
#endif