#include <stdint.h>
#include <gfx_encoding.hpp>
namespace gfx {
typedef uint32_t UTF32; /* at least 32 bits */
typedef uint16_t UTF16; /* at least 16 bits */
typedef uint8_t UTF8;   /* typically 8 bits */
/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

static const int halfShift = 10; /* used for shifting by 10 bits */
static const UTF32 halfBase = 0x0010000UL;
static const UTF32 halfMask = 0x3FFUL;
#define UNI_SUR_HIGH_START (UTF32)0xD800
#define UNI_SUR_HIGH_END (UTF32)0xDBFF
#define UNI_SUR_LOW_START (UTF32)0xDC00
#define UNI_SUR_LOW_END (UTF32)0xDFFF
static const char trailingBytesForUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
static const UTF32 offsetsFromUTF8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                                         0x03C82080UL, 0xFA082080UL, 0x82082080UL};
static const UTF8 firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
static bool is_legal_utf8(const UTF8* source, int length) {
    UTF8 a;
    const UTF8* srcptr = source + length;
    switch (length) {
        default:
            return false;
            /* Everything else falls through when "true"... */
        case 4:
            if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
            [[fallthrough]];
        case 3:
            if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
            [[fallthrough]];
        case 2:
            if ((a = (*--srcptr)) > 0xBF) return false;
            switch (*source) {
                /* no fall-through in this inner switch */
                case 0xE0:
                    if (a < 0xA0) return false;
                    break;
                case 0xED:
                    if (a > 0x9F) return false;
                    break;
                case 0xF0:
                    if (a < 0x90) return false;
                    break;
                case 0xF4:
                    if (a > 0x8F) return false;
                    break;
                default:
                    if (a < 0x80) return false;
                    break;
            }
            [[fallthrough]];
        case 1:
            if (*source >= 0x80 && *source < 0xC2) return false;
            break;
    }
    if (*source > 0xF4) return false;
    return true;
}
static gfx_result utf8_to_utf32(const char* utf8, int32_t* out_codepoint, size_t* in_out_length) {
    gfx_result result = gfx_result::success;
    const UTF8* source = (UTF8*)utf8;
    UTF32* target = (UTF32*)out_codepoint;
    size_t in_len = *in_out_length;
    *in_out_length = 0;
    while (*in_out_length < in_len) {
        UTF32 ch = 0;
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        if (extraBytesToRead >= in_len) {
            result = gfx_result::invalid_format;
            break;
        }
        /* Do this check whether lenient or strict */
        if (!is_legal_utf8(source, extraBytesToRead + 1)) {
            //Serial.printf("Illegal UTF-8. in_len = %d\n",(int)in_len);
            result = gfx_result::invalid_format;
            break;
        }
        /*
         * The cases all fall through. See "Note A" below.
         */
        switch (extraBytesToRead) {
            case 5:
                ch += *source++;
                ch <<= 6;
                ++*in_out_length;
                --in_len;
                [[fallthrough]];
            case 4:
                ch += *source++;
                ch <<= 6;
                ++*in_out_length;
                --in_len;
                [[fallthrough]];
            case 3:
                ch += *source++;
                ch <<= 6;
                ++*in_out_length;
                --in_len;
                [[fallthrough]];
            case 2:
                ch += *source++;
                ch <<= 6;
                ++*in_out_length;
                --in_len;
                [[fallthrough]];
            case 1:
                ch += *source++;
                ch <<= 6;
                ++*in_out_length;
                --in_len;
                [[fallthrough]];
            case 0:
                ch += *source++;
                ++*in_out_length;
                --in_len;
                break;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];
        if (ch <= UNI_MAX_LEGAL_UTF32) {
            /*
             * UTF-16 surrogate values are illegal in UTF-32, and anything
             * over Plane 17 (> 0x10FFFF) is illegal.
             */
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
                result = gfx_result::invalid_format;
                break;
            } else {
                *target = ch;
                return gfx_result::success;
            }
        } else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
            result = gfx_result::invalid_format;
            *target = UNI_REPLACEMENT_CHAR;
        }
    }
    return result;
}

gfx_result static latin1_to_utf8(const char* in,  size_t* in_out_in_length, char* utf8_out, size_t* in_out_utf8_out_length) {
    char* outstart = utf8_out;
    char* outend = utf8_out + (*in_out_utf8_out_length);
    const char* inend = in + *in_out_in_length;
    *in_out_in_length = 0;
    *in_out_utf8_out_length = 0;
    char c;
    while (in < inend) {
        c = *in++;
        ++*in_out_in_length;
        if (c < 0x80) {
            if (utf8_out >= outend) return gfx_result::out_of_memory;
            *utf8_out++ = c;
            *in_out_utf8_out_length=1;
            return gfx_result::success;
        } else {
            if (utf8_out >= outend) return gfx_result::out_of_memory;
            *utf8_out++ = 0xC0 | (c >> 6);
            if (utf8_out >= outend) return gfx_result::out_of_memory;
            *utf8_out++ = 0x80 | (0x3F & c);
            *in_out_utf8_out_length=2;
            return gfx_result::success;
        }
    }
    *in_out_utf8_out_length = utf8_out - outstart;
    return gfx_result::success;
}

gfx_result text_encoding::utf8_encoder::to_utf32(const text_handle in, int32_t* out_codepoint, size_t* in_out_length) const {
    const char* pin = (const char *)in;
    if(*in_out_length==0) {
        *in_out_length = trailingBytesForUTF8[(size_t)*pin]+1;
    }
    return utf8_to_utf32(pin, out_codepoint, in_out_length);
}

gfx_result text_encoding::latin1_encoder::to_utf32(const text_handle in, int32_t* out_codepoint, size_t* in_out_length) const {
    const char* pin = (const char *)in;
    if(*in_out_length==0) {
        *in_out_length = trailingBytesForUTF8[(size_t)*pin]+1;
    }
    char out_tmp1[4];
    size_t outlen = sizeof(out_tmp1);
    gfx_result r= latin1_to_utf8(pin, in_out_length,out_tmp1, &outlen);
    if (r!=gfx_result::success) {
        *out_codepoint = 0;
        return r;
    }
    r=utf8_to_utf32(out_tmp1, out_codepoint, &outlen);
    if (r!=gfx_result::success) {
        *out_codepoint = 0;
        return r;
    }
    return gfx_result::success;
}
}  // namespace gfx