#ifndef HTCW_GFX_ENCODING_HPP
#define HTCW_GFX_ENCODING_HPP
#include "gfx_core.hpp"
#include <stdint.h>
namespace gfx {
    extern gfx_result to_utf32(const char* in,uint32_t* out_codepoint, size_t* in_out_length, gfx_encoding encoding=gfx_encoding::utf8);
}
#endif // HTCW_GFX_ENCODING_HPP