#ifndef HTCW_GFX_CORE_HPP
#define HTCW_GFX_CORE_HPP
#include <string.h>
#include <stdint.h>
#ifdef __has_include
#if __has_include("gfx_conf.h")
#include "gfx_conf.h"
#endif
#endif
//#define HTCW_GFX_NO_SWAP
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
    using c_file_stream = io::c_file_stream;
#endif
    namespace helpers {
        // implement std::move to limit dependencies on the STL, which may not be there
        template< class T > struct gfx_remove_reference      { typedef T type; };
        template< class T > struct gfx_remove_reference<T&>  { typedef T type; };
        template< class T > struct gfx_remove_reference<T&&> { typedef T type; };
        template <typename T>
        typename gfx_remove_reference<T>::type&& gfx_move(T&& arg) {
            return static_cast<typename gfx_remove_reference<T>::type&&>(arg);
        }
    }
    template<bool Blt,bool BltSpans,bool CopyFrom,bool CopyTo>
    struct gfx_caps {
        constexpr const static bool blt = Blt;
        constexpr const static bool blt_spans = BltSpans;
        constexpr const static bool copy_from = CopyFrom;
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
    struct gfx_span {
        uint8_t* data;
        size_t length;
    };
    struct gfx_cspan {
        const uint8_t* cdata;
        size_t length;
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
        template<typename...Types> 
        struct tuple { };
        template<
            typename Type,
            typename... Types
        >
        struct tuple<Type, Types...> {
            using type = Type;
            Type value;
            tuple<Types...> next;
            constexpr inline tuple(
                const Type& value, 
                const Types& ... next)
                : value(value)
                , next(next...) {
            }
        };
        // tuple index helper base declaration
        template<size_t Index,typename Type>
        struct tuple_index_impl {};
        // tuple index helper terminator specialization
        // on index zero (specialization #1)
        template<typename Type, typename... Types>
        struct tuple_index_impl<0,tuple<Type,Types...>> {
            // indicates the type of the tuple itself
            using tuple_type = tuple<Type, Types...>;
            // indicates the first value type in the tuple
            using value_type = Type;
            // retrieve the tuple's value
            constexpr inline static value_type value(
                                        tuple_type &t) {
                return t.value;
            }
            // set the tuple's value
            constexpr inline static void value(
                    tuple_type &t,const value_type& v) {
                t.value=v;
            }
        };
        template<
                size_t Index,
                typename Type,
                typename... Types
            >
        struct tuple_index_impl<Index, tuple<Type, Types...>> { 
            using tuple_type = tuple<Type, Types...>;
            using value_type = typename tuple_index_impl<
                    Index - 1, 
                    tuple<Types...>>::value_type;
            constexpr inline static value_type value(
                                        tuple_type &t) {
                return tuple_index_impl<
                    Index - 1, 
                    tuple<Types...>>::value(t.next);
            }
            constexpr inline static void value(
                    tuple_type &t,const value_type& v) {
                tuple_index_impl<
                    Index - 1, 
                    tuple<Types...>>::value(t.next,v);
            }
        };
        // static tuple by index getter method template
        template<
            size_t Index, 
            typename TupleType
        >
        typename tuple_index_impl<Index, TupleType>::value_type 
        constexpr tuple_index(TupleType &t) {
            return tuple_index_impl<Index, TupleType>::value(t);
        }
        // static tuple by index setter method template
        template<
            size_t Index, 
            typename TupleType
        >
        constexpr void tuple_index(
                TupleType &t,
                const typename tuple_index_impl<
                        Index, 
                        TupleType>::value_type& v) {
            return tuple_index_impl<Index, TupleType>::value(t,v);
        }
        
        template<int ...> struct index_sequence {};
        template<int N, int ...S> struct make_index_sequence : make_index_sequence<N - 1, N - 1, S...> { };
        template<int ...S> struct make_index_sequence<0, S...>{ typedef index_sequence<S...> type; };
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