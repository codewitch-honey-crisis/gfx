#ifndef HTCW_GFX_ENCODING_HPP
#define HTCW_GFX_ENCODING_HPP
#include "gfx_core.hpp"
#include <stdint.h>
namespace gfx {
    typedef void* text_handle;
    class text_encoder {
    public:
        virtual gfx_result to_utf32(const text_handle in, int32_t* out_codepoint, size_t* in_out_length) const=0;
    };
    class text_encoding {
        class utf8_encoder : public text_encoder {
        public: 
            virtual gfx_result to_utf32(const text_handle in, int32_t* out_codepoint, size_t* in_out_length) const override;
        };
        class latin1_encoder : public text_encoder {
        public: 
            virtual gfx_result to_utf32(const text_handle in, int32_t* out_codepoint, size_t* in_out_length) const override;
        };
        static const utf8_encoder s_utf8;
        static const latin1_encoder s_latin1;
    public:
        static const text_encoder& utf8;
        static const text_encoder& latin1;
    };
}
#endif // HTCW_GFX_ENCODING_HPP