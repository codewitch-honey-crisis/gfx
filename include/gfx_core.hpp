#ifndef HTCW_GFX_CORE_HPP
#define HTCW_GFX_CORE_HPP
#include <string.h>
#include <stdint.h>
#ifdef GFX_BIG_ENDIAN
    #define HTCW_BIG_ENDIAN
#endif
#ifdef GFX_LITTLE_ENDIAN
    #define HTCW_LITTLE_ENDIAN
#endif
#include <htcw_bits.hpp>
#if !defined(GFX_BIG_ENDIAN) && !defined(GFX_LITTLE_ENDIAN)
    #ifdef HTCW_BIG_ENDIAN
        #define GFX_BIG_ENDIAN
    #elif defined(HTCW_LITTLE_ENDIAN)
        #define GFX_LITTLE_ENDIAN
    #endif
#endif
#include <io_stream.hpp>
#ifndef ARDUINO
    #define PROGMEM 
    #define pgm_read_byte(x) (*x)
#endif
namespace gfx {
    static_assert(bits::endianness()!=bits::endian_mode::none,"Please define GFX_LITTLE_ENDIAN or GFX_BIG_ENDIAN before including GFX to indicate the byte order of the platform.");
    using stream = io::stream;
    using seek_origin = io::seek_origin;
    using stream_caps = io::stream_caps;
#ifdef ARDUINO
    using arduino_stream = io::arduino_stream;
#endif
    using buffer_stream = io::buffer_stream;
    using const_buffer_stream = io::const_buffer_stream;
#ifndef IO_NO_FS
    using file_stream = io::file_stream;
#endif
    template<bool Blt,bool Async,bool Batch,bool CopyFrom,bool Suspend,bool Read,bool CopyTo>
    struct gfx_caps {
        constexpr const static bool blt = Blt;
        constexpr const static bool async = Async;
        constexpr const static bool batch = Batch;
        constexpr const static bool copy_from = CopyFrom;
        constexpr const static bool suspend = Suspend;
        constexpr const static bool read = Read;
        constexpr const static bool copy_to = CopyTo;
    };
    enum struct gfx_result {
        success = 0,
        canceled,
        invalid_argument,
        not_supported,
        io_error,
        device_error,
        out_of_memory,
        invalid_format,
        no_palette,
        invalid_state,
        unknown_error
    };
    enum struct gfx_encoding {
        utf8 = 0,
        latin1 = 1
    };
    namespace helpers {
        template<typename T, typename U>
        struct is_same  {
            constexpr static const bool value = false;
        };
        template<typename T>
        struct is_same<T, T> {
            constexpr static const bool value = true;
        };
        // adjusts byte order if necessary
        constexpr static inline uint16_t order_guard(uint16_t value) {
            if(bits::endianness()==bits::endian_mode::little_endian) {
                return bits::swap(value);
            }
            return value;
        }
        // adjusts byte order if necessary
        constexpr static inline uint32_t order_guard(uint32_t value) {
            if(bits::endianness()==bits::endian_mode::little_endian) {
                return bits::swap(value);
            }
            return value;
        }
#if HTCW_MAX_WORD>=64
        // adjusts byte order if necessary
        constexpr static inline uint64_t order_guard(uint64_t value) {
            if(bits::endianness()==bits::endian_mode::little_endian) {
                return bits::swap(value);
            }
            return value;
        }
#endif
        // adjusts byte order if necessary (disambiguation)
        constexpr static inline uint8_t order_guard(uint8_t value) {
            return value;
        }
        template<typename ValueType>
        constexpr static inline ValueType clamp(ValueType value,ValueType min,ValueType max) {
            return value<min?min:value>max?max:value;
        }
        
        
    }
}
#endif