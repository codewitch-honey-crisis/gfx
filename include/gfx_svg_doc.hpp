#ifndef HTCW_GFX_SVG_DOC_HPP
#define HTCW_GFX_SVG_DOC_HPP
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include <string.h>
namespace gfx {
    class svg_doc final {
        void* m_doc_data;
        void do_move(svg_doc& rhs);
        void do_free();
        svg_doc(const svg_doc& rhs)=delete;
        svg_doc& operator=(const svg_doc& rhs)=delete;
    public:
        svg_doc();
        ~svg_doc();
        svg_doc(svg_doc&& rhs);
        svg_doc(stream* svg_stream, uint16_t dpi=96,void*(allocator)(size_t)=::malloc,void*(reallocator)(void*,size_t)=::realloc,void(deallocator)(void*)=::free);
        svg_doc& operator=(svg_doc&& rhs);
        static gfx_result read(stream* input,svg_doc* out_doc, uint16_t dpi=96,void*(allocator)(size_t)=::malloc,void*(reallocator)(void*,size_t)=::realloc,void(deallocator)(void*)=::free);
        bool initialized() const;
        void draw(float scale,const srect16& rect, void(read_callback)(int x, int y, unsigned char* r,unsigned char* g,unsigned char* b,unsigned char* a,void* state),void*read_callback_state,void(write_callback)(int x, int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a,void* state),void*write_callback_state,void*(allocator)(size_t)=::malloc,void*(reallocator)(void*,size_t)=::realloc,void(deallocator)(void*)=::free) const;
        float scale(float line_height) const;
        float scale(sizef dimensions) const;
        float scale(ssize16 dimensions) const;
        float scale(size16 dimensions) const;
        sizef dimensions() const;
    };
}
#endif