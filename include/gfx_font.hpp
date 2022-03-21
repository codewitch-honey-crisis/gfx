#ifndef HTCW_GFX_FONT_HPP
#define HTCW_GFX_FONT_HPP
#include <stdlib.h>
#include "gfx_core.hpp"
#include <htcw_bits.hpp>
#include <io_stream.hpp>
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
        static result read_font_init(io::stream* stream, long long int* pos,uint16_t* ctstart,uint16_t* ctsize);
        static result read_font(io::stream* stream,char first_char, char last_char, uint8_t* buffer,font* out_font,size_t* out_size);
        static result read_ne(uint32_t neoff,io::stream* stream,size_t index,char first_char,char last_char, uint8_t* buffer, font* out_font,size_t* out_size);
        static inline result read_pe(uint32_t neoff, io::stream* stream,size_t index,char first_char, char last_char, uint8_t* buffer,font* out_font,size_t* out_size) {
            return result::io_error;
        }
        const uint8_t* char_data_ptr(char ch) const;
        font(const font& rhs)=delete;
        font& operator=(const font& rhs)=delete;
    public:
        // constructs an empty font. Not really usable yet
        font();
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
        );
        // constructs a font with the specified stream
        font(io::stream* stream,size_t index=0, char first_char = '\0', char last_char='\xFF');
        // resource steals from another font
        font(font&& rhs);
        // resource steals from another font
        font& operator=(font&& rhs);
        // destroys any memory created by the font
        ~font();
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
        uint16_t width(char ch) const;
        // retrieves information about the specified character
        const font_char operator[](int ch) const;

        // reads a font from a stream
        static result read(io::stream* stream, font* out_font,size_t index=0, char first_char = '\0',char last_char='\xFF',uint8_t* buffer=nullptr);
        // measures the size of the text within the destination rectangle
        ssize16 measure_text(
            ssize16 dest_size,
            const char* text,
            unsigned int tab_width=4) const;
    };
}
#endif