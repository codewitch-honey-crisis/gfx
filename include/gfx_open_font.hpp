#ifndef GFX_OPEN_FONT_HPP
#define GFX_OPEN_FONT_HPP
#include <stdlib.h>
#include <htcw_data.hpp>
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
namespace gfx {
    struct draw;
    struct open_font;
    class open_font_cache final {
        friend open_font;
        using cache_type = data::simple_fixed_map<int,int,25>;
        void*(*m_allocator)(size_t);
        void*(*m_reallocator)(void*,size_t);
        void(*m_deallocator)(void*);
        cache_type* m_cache;
        void deinitialize();
        bool initialize();
    public:
        open_font_cache(void*(allocator)(size_t) = ::malloc,void*(reallocator)(void*,size_t) = ::realloc,void(deallocator)(void*) = ::free);
        ~open_font_cache();
        void clear();
    };
    class open_font final {
        friend class draw;
        void*(*m_allocator)(size_t);
        void(*m_deallocator)(void*);
        void* m_info_data;
        void free();
        void draw(int(*render_cb)(int x,int y,int c,void* state),void* state, int width,int height,int out_stride,float scale_x,float scale_y,float shift_x, float shift_y,int glyph) const;
        void bounding_box(int* x1, int* y1, int* x2, int* y2) const;
        void glyph_hmetrics(int glyph_index, int* advance_width, int* left_side_bearing) const;
        void glyph_bitmap_bounding_box(int glyph_index,float scale_x,float scale_y,float shift_x,float shift_y,int* x1, int* y1, int* x2, int* y2) const;
        int glyph_index(const char* sz, size_t* out_advance, gfx_encoding encoding = gfx_encoding::utf8,open_font_cache* cache = nullptr) const;
        void font_vmetrics(int* ascent, int* descent, int* line_gap) const;
        static int latin1_to_utf8(unsigned char* out, size_t outlen, const unsigned char* in, size_t inlen);
        static int utf8_to_utf16(uint16_t* out, size_t outlen, const unsigned char* in, size_t inlen);
        static int to_utf32_codepoint(const char* in,size_t in_length, int* codepoint, gfx_encoding encoding=gfx_encoding::utf8);
        open_font(const open_font& rhs)=delete;
        open_font& operator=(const open_font& rhs)=delete;
    public:
        open_font();
        // not the ideal way to load, but used for embedded fonts
        open_font(stream* stream,void*(allocator)(size_t)=::malloc, void(deallocator)(void*)=::free);
        open_font(open_font&& rhs);
        virtual ~open_font();
        inline open_font& operator=(open_font&& rhs);
        uint16_t ascent() const;
        uint16_t descent() const;
        uint16_t line_gap() const;
        float scale(float pixel_height) const;
        uint16_t advance_width(int codepoint) const;
        inline float advance_width(int codepoint,float scale) const;
        uint16_t left_bearing(int codepoint) const;
        inline float left_bearing(int codepoint,float scale) const;
        int16_t kern_advance_width(int codepoint1,int codepoint2) const;
        inline float kern_advance_width(int codepoint1,int codepoint2,float scale) const;
        rect16 bounds(int codepoint,float scale,float x_shift=0.0,float y_shift=0.0) const;
        void cache(open_font_cache* cache,const char* text,gfx_encoding encoding=gfx_encoding::utf8);
        // measures the size of the text within the destination area
        ssize16 measure_text(
            ssize16 dest_size,
            spoint16 offset,
            const char* text,
            float scale,
            float scaled_tab_width=0.0,
            gfx_encoding encoding=gfx_encoding::utf8,
            open_font_cache* cache = nullptr) const;
        // opens a font stream. Note that the stream must remain open as long as the font is being used. The stream must be readable and seekable. This class does not close the stream
        // This always chooses the first font out of a font collection.
        static gfx_result open(stream* stream, open_font* out_result, void*(*allocatpr)(size_t)=::malloc, void(*deallocator)(void*)=::free);
    };
}
#endif