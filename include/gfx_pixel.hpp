#ifndef HTCW_GFX_PIXEL_HPP
#define HTCW_GFX_PIXEL_HPP
#include "bits.hpp"
#include <math.h>
#define GFX_CHANNEL_NAME(x) struct x { static inline constexpr const char* value() { return #x; } };
namespace gfx {
    namespace helpers {
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
        // adjusts byte order if necessary
        constexpr static inline uint64_t order_guard(uint64_t value) {
            if(bits::endianness()==bits::endian_mode::little_endian) {
                return bits::swap(value);
            }
            return value;
        }
        // adjusts byte order if necessary (disambiguation)
        constexpr static inline uint8_t order_guard(uint8_t value) {
            return value;
        }
        template<typename IntType>
        constexpr static inline IntType clamp(IntType value,IntType min,IntType max) {
            return value<min?min:value>max?max:value;
        }
        
    }
    // predefined channel names
    struct channel_name {
        // red
        GFX_CHANNEL_NAME(R)
        // green
        GFX_CHANNEL_NAME(G)
        // blue
        GFX_CHANNEL_NAME(B)
        // alpha
        GFX_CHANNEL_NAME(A)
        // hue
        GFX_CHANNEL_NAME(H)
        // saturation
        GFX_CHANNEL_NAME(S)
        // red projection
        GFX_CHANNEL_NAME(V)
        // blue projection
        GFX_CHANNEL_NAME(U)
        // luma
        GFX_CHANNEL_NAME(Y)
        // luminosity
        GFX_CHANNEL_NAME(L)
        // index (for indexed color)
        GFX_CHANNEL_NAME(index)
        // non-op
        GFX_CHANNEL_NAME(nop);
        // TODO: add more of these
    };
    // defines a channel for a pixel
    template<
        // the channel name, like channel_name::R
        typename Name,
        // the bit depth
        size_t BitDepth, 
        // the minimum value
        bits::uintx<bits::get_word_size(BitDepth)> Min=0, 
        // the maximum value
        bits::uintx<bits::get_word_size(BitDepth)>Max=((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1)), 
        // the scale denominator
        bits::uintx<bits::get_word_size(BitDepth)> Scale = Max> 
    struct channel_traits {
        // this type
        using type = channel_traits<Name,BitDepth,Min,Max,Scale>;
        // the type that represents the name
        using name_type = Name;
        // the integer type of this channel
        using int_type = bits::uintx<bits::get_word_size(BitDepth)>;
        // the floating point type of this channel
        using real_type = bits::realx<16<=BitDepth?64:32>;
        // the bit depth of this channel
        constexpr static const size_t bit_depth = BitDepth;
        // the minimum value
        constexpr static const int_type min = Min;
        // the maximum value
        constexpr static const int_type max = Max;
        // the scale denominator
        constexpr static const int_type scale = Scale;
        // the reciprocal of the scale denominator
        constexpr static const real_type scalef = 1/(real_type)Scale; 
        // a mask of the int value
        constexpr static const int_type int_mask = ~int_type(0);
        // a mask of the channel value
        constexpr static const int_type mask =int_type(int_mask>>((sizeof(int_type)*8)-BitDepth));
        // constraints
        static_assert(BitDepth>0,"Bit depth must be greater than 0");
        static_assert(BitDepth<=64,"Bit depth must be less than or equal to 64");
        static_assert(Min<=Max,"Min must be less than or equal to Max");
        static_assert(Max<=((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1)),"Max is greater than the maximum allowable value");
        static_assert(Scale>0,"Scale must not be zero");
    };
    // specialization for empty channel
    template<>
    struct channel_traits<channel_name::nop,0, 0,0, 0> {
        using type = channel_traits<channel_name::nop,0, 0,0, 0>;
        using name_type = channel_name::nop;
        using int_type = uint8_t;
        using real_type = float;
        constexpr static const size_t bit_depth = 0;
        constexpr static const int_type min = 0;
        constexpr static const int_type max = 0;
        constexpr static const int_type scale = 0;
        constexpr static const float scalef = 0.0; 
        constexpr static const int_type int_mask = 0;
        constexpr static const int_type mask = 0;
    };
    
    // represents a channel's metadata
    template<typename PixelType,typename ChannelTraits,
            size_t Index,
            size_t BitsToLeft> 
    struct channel final {
        // the declaring pixel's type
        using pixel_type = PixelType;
        // this type
        using type = channel<pixel_type,ChannelTraits,Index,BitsToLeft>;
        // the name type for the channel
        using name_type = typename ChannelTraits::name_type;
        // the integer type for the channel
        using int_type = typename ChannelTraits::int_type;
        // the floating point type for the channel
        using real_type = typename ChannelTraits::real_type;
        // the declaring pixel's integer type
        using pixel_int_type = typename PixelType::int_type;
        // the number of bits to the left of this channel
        constexpr static const size_t bits_to_left = ChannelTraits::bit_depth!=0?BitsToLeft:0;
        // the total bits to the right of this channel including padding
        constexpr static const size_t total_bits_to_right =  ChannelTraits::bit_depth!=0?sizeof(pixel_int_type)*8-BitsToLeft-ChannelTraits::bit_depth:0;
        // the bits to the right of this channel excluding padding
        constexpr static const size_t bits_to_right =ChannelTraits::bit_depth!=0?total_bits_to_right-PixelType::pad_right_bits:0;
        // the name of the channel
        constexpr static inline const char* name() { return ChannelTraits::name(); }
        // the bit depth of the channel
        constexpr static const size_t bit_depth = ChannelTraits::bit_depth;
        // the mask of the channel's value
        constexpr static const int_type value_mask = ChannelTraits::mask;
        // the mask of the channel's value within the entire pixel's value
        constexpr static const pixel_int_type channel_mask = (value_mask>0)?pixel_int_type(pixel_int_type(value_mask)<<total_bits_to_right):pixel_int_type(0);
        // the minimum value for the channel
        constexpr static const int_type min = ChannelTraits::min;
        // the maximum value for the channel
        constexpr static const int_type max = ChannelTraits::max;
        // the scale denominator
        constexpr static const int_type scale = ChannelTraits::scale;
        // the reciprocal of the scale denominator
        constexpr static const real_type scalef = ChannelTraits::scalef;

        
    };
    // various utility templates and methods
    namespace helpers {
        
        template <typename... ChannelTraits> struct bit_depth;
        template <typename T, typename... ChannelTraits>
        struct bit_depth<T, ChannelTraits...> {
            static constexpr const size_t value = T::bit_depth + bit_depth<ChannelTraits...>::value;
        };
        template <> struct bit_depth<> { static const size_t value = 0; };
        
        template<typename PixelType,size_t Index,size_t Count,size_t BitsToLeft,typename... ChannelTraits>
        struct channel_by_index_impl;        
        template<typename PixelType,size_t Index,size_t Count, size_t BitsToLeft, typename ChannelTrait,typename... ChannelTraits>
        struct channel_by_index_impl<PixelType,Index,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = typename channel_by_index_impl<PixelType,Index-1,Count+1,BitsToLeft+ChannelTrait::bit_depth, ChannelTraits...>::type;
        };
        template <typename PixelType,size_t Count,size_t BitsToLeft,typename ChannelTrait, typename ...ChannelTraits>
        struct channel_by_index_impl<PixelType,0,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = channel<PixelType,ChannelTrait,Count,BitsToLeft>;
        };
        template<typename PixelType,size_t Index,size_t Count,size_t BitsToLeft> 
        struct channel_by_index_impl<PixelType,Index,Count,BitsToLeft> {
            using type = void;
        };

        template<typename PixelType,size_t Index,size_t Count,size_t BitsToLeft,typename... ChannelTraits>
        struct channel_by_index_unchecked_impl;        
        template<typename PixelType,size_t Index,size_t Count, size_t BitsToLeft, typename ChannelTrait,typename... ChannelTraits>
        struct channel_by_index_unchecked_impl<PixelType,Index,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = typename channel_by_index_unchecked_impl<PixelType,Index-1,Count+1,BitsToLeft+ChannelTrait::bit_depth, ChannelTraits...>::type;
        };
        template <typename PixelType,size_t Count,size_t BitsToLeft,typename ChannelTrait, typename ...ChannelTraits>
        struct channel_by_index_unchecked_impl<PixelType,0,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = channel<PixelType,ChannelTrait,Count,BitsToLeft>;
        };
        template<typename PixelType,size_t Index,size_t Count,size_t BitsToLeft> 
        struct channel_by_index_unchecked_impl<PixelType,Index,Count,BitsToLeft> {
            using type = channel<PixelType,channel_traits<channel_name::nop,0,0,0,0>,0,0>;
        };

        template<typename PixelType,size_t Count,typename... ChannelTraits>
        struct pixel_init_impl;        
        template<typename PixelType,size_t Count, typename ChannelTrait,typename... ChannelTraits>
        struct pixel_init_impl<PixelType,Count,ChannelTrait,ChannelTraits...> {
            using ch = typename PixelType::template channel_by_index<Count>;
            using next = pixel_init_impl<PixelType,Count+1, ChannelTraits...>;
            
            constexpr static inline void init(PixelType& pixel,typename ChannelTrait::int_type value, typename ChannelTraits::int_type... values) {
                constexpr const size_t index = Count;
                if(ChannelTrait::bit_depth==0) return;
                pixel.template channel<index>(value);
                next::init(pixel,values...);
            }
            constexpr static inline void initf(PixelType& pixel,typename ChannelTrait::real_type value, typename ChannelTraits::real_type... values) {
                constexpr const size_t index = Count;
                if(ChannelTrait::bit_depth==0) return;
                pixel.template channelf<index>(value);
                next::initf(pixel,values...);
            }
            
        };
        template<typename PixelType,size_t Count>
        struct pixel_init_impl<PixelType,Count> {
            constexpr static inline void init(PixelType& pixel) {
            
            }
            constexpr static inline void initf(PixelType& pixel) {
            
            }
        };

        template<bool Value>
        struct boolean_type {
            constexpr static const bool value = Value;
        };

        template<typename T, typename U>
        struct is_same : boolean_type<false> {};
        template<typename T>
        struct is_same<T, T> : boolean_type<true> {};

        template<size_t Count,typename Name, typename ...ChannelTraits>
        struct channel_index_by_name_impl;
        template<size_t Count,typename Name>
        struct channel_index_by_name_impl<Count,Name> {
            static constexpr size_t value=-1;
        };
        template<size_t Count,typename Name, typename ChannelTrait, typename ...ChannelTraits>
        struct channel_index_by_name_impl<Count,Name, ChannelTrait, ChannelTraits...> {
            static constexpr size_t value = is_same<Name, typename ChannelTrait::name_type>::value ? Count : channel_index_by_name_impl<Count+1,Name, ChannelTraits...>::value;            
        };

        template <typename PixelType, typename... ChannelTraits>
        struct is_subset_pixel_impl;
        template <typename PixelType>
        struct is_subset_pixel_impl<PixelType>
        {
            // this is what we return when there
            // are no items left
            static constexpr bool value = true;
        };
        template <typename PixelType,
                typename ChannelTrait,
                typename... ChannelTraits>
        struct is_subset_pixel_impl<
            PixelType,
            ChannelTrait,
            ChannelTraits...>
        {
        private:
            using result = typename PixelType::template channel_index_by_name<typename ChannelTrait::name_type>;
            using next = typename helpers::is_subset_pixel_impl<PixelType, ChannelTraits...>;

        public:
            static constexpr bool value = (-1 != result::value) &&
                                        next::value;
        };
        
        template <typename PixelTypeLhs, typename PixelTypeRhs>
        class unordered_equals_pixel_impl
        {
            using lhs = typename PixelTypeLhs::template is_superset_of<PixelTypeRhs>;
            using rhs = typename PixelTypeRhs::template is_superset_of<PixelTypeLhs>;

        public:
            constexpr static bool value = lhs::value &&
                                        rhs::value;
        };
        
        template <
            typename PixelType,
            size_t Index,
            typename... ChannelTraits>
        struct is_equal_pixel_impl;
        template <
            typename PixelType,
            size_t Index>
        struct is_equal_pixel_impl<PixelType, Index>
        {
            // this is what we return when there
            // are no items left
            static constexpr bool value = true;
        };
        template <typename PixelType,
                size_t Index,
                typename ChannelTrait,
                typename... ChannelTraits>
        struct is_equal_pixel_impl<
            PixelType,
            Index,
            ChannelTrait,
            ChannelTraits...>   
        {
        private:
            using result = typename PixelType::template channel_index_by_name<typename ChannelTrait::name_type>;
            using next = typename helpers::is_equal_pixel_impl<PixelType, Index + 1, ChannelTraits...>;

        public:
            static constexpr bool value = result::value == Index && next::value;
        };
        template <typename PixelTypeLhs, typename... ChannelTraits>
        class equals_pixel_impl
        {
            using compare = typename helpers::is_equal_pixel_impl<PixelTypeLhs, 0, ChannelTraits...>;

        public:
            constexpr static bool value =
                PixelTypeLhs::channels == sizeof...(ChannelTraits) &&
                compare::value;
        };

        template <typename PixelType,typename... ChannelNames> class has_channel_names_impl;
        template<typename PixelType,typename ChannelName,typename... ChannelNames> 
        class has_channel_names_impl<PixelType,ChannelName,ChannelNames...> {
            using chidx = typename PixelType::template channel_index_by_name<ChannelName>;
        public:
            constexpr static const bool value = (-1!= chidx::value) &&
                (-1!= has_channel_names_impl<PixelType,ChannelNames...>::value);
        };
        template<typename PixelType> 
        class has_channel_names_impl<PixelType> {
        public:
            constexpr static const bool value = true;
        };
        // converts one channel's bit depth to another
        template <typename ChannelLhs,typename ChannelRhs>
        constexpr inline static typename ChannelRhs::int_type convert_channel_depth(typename ChannelLhs::int_type v) {
            typename ChannelRhs::int_type rv=0;
            if(ChannelLhs::bit_depth==0 || ChannelRhs::bit_depth==0) return 0;
            const int srf = (int)ChannelLhs::bit_depth-(int)ChannelRhs::bit_depth;
            if(0<srf) {
                rv = (typename ChannelRhs::int_type)(v>>srf);
                //float vs = v*ChannelLhs::scalef;
                //rv = clamp((typename ChannelRhs::int_type)(vs*ChannelRhs::scale+.5),ChannelRhs::min,ChannelRhs::max);
            } else if(0>srf) {
                float vs = v*ChannelLhs::scalef;
                rv = clamp((typename ChannelRhs::int_type)(vs*ChannelRhs::scale+.5),ChannelRhs::min,ChannelRhs::max);
            } else
                rv = (typename ChannelRhs::int_type)(v);
            return rv;
        }
        // gets the native_value of a channel without doing compile time checking on the index
        template<typename PixelType,size_t Index>
        constexpr inline typename PixelType::template channel_by_index_unchecked<Index>::int_type get_channel_direct_unchecked(const typename PixelType::int_type& pixel_value) {
            using ch = typename PixelType::template channel_by_index_unchecked<Index>;
            if(0>Index || Index>=PixelType::channels) return 0;
            const typename PixelType::int_type p = pixel_value>>ch::total_bits_to_right;
            const typename ch::int_type result = typename ch::int_type(typename PixelType::int_type(p & typename PixelType::int_type(ch::value_mask)));
            return result;
            
        }
        // sets the native_value of a channel without doing compile time checking on the index
        template<typename PixelType,size_t Index>
        constexpr inline void 
        
        set_channel_direct_unchecked(typename PixelType::int_type& pixel_value,typename PixelType::template channel_by_index_unchecked<Index>::int_type value) {
            if(0>Index || Index>=PixelType::channels) return;
            using ch = typename PixelType::template channel_by_index_unchecked<Index>;
            const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(value,ch::min,ch::max))<<ch::total_bits_to_right);
            pixel_value=typename PixelType::int_type((pixel_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
        }

    }
    // represents a pixel
    template<typename... ChannelTraits> 
    struct pixel {
        // this type
        using type = pixel<ChannelTraits...>;
        // the type used for doing intermediary conversions to different formats when no explicit conversion is implemented
        using rgb_conversion_type = pixel<channel_traits<channel_name::R,8>,channel_traits<channel_name::G,8>,channel_traits<channel_name::B,8>,channel_traits<channel_name::A,8>>;
        // the integer type of the pixel
        using int_type = bits::uintx<bits::get_word_size(helpers::bit_depth<ChannelTraits...>::value)>;
        // the number of channels
        constexpr static const size_t channels = sizeof...(ChannelTraits);
        // the total bit depth of the pixel
        constexpr static const size_t bit_depth = helpers::bit_depth<ChannelTraits...>::value;
        // the minimum number of bytes needed to store the pixel
        constexpr static const size_t packed_size = bit_depth / 8.0 + .99999;
        // true if the pixel is a whole number of bytes
        constexpr static const bool byte_aligned = 0==bit_depth/8.0 - (size_t)(bit_depth/8);
        // the total size in bits, including padding
        constexpr static const size_t total_size_bits = sizeof(int_type)*8;
        // the packed size, in bits
        constexpr static const size_t packed_size_bits = packed_size*8;
        // the count of bits to the right that are unused
        constexpr static const size_t pad_right_bits = total_size_bits-bit_depth;
        // the mask of the pixel's value
        constexpr static const int_type mask = int_type(int_type(~int_type(0))<<(pad_right_bits));//int_type(bit_depth==64?0xFFFFFFFFFFFFFFFF:(((1<<bit_depth)-1)<<pad_right_bits));
        
        // the pixel value, in platform native format
        int_type native_value;
        
        // initializes the pixel
        constexpr inline pixel() : native_value(0) {}
        // initializes the pixel with a set of channel values
        constexpr inline pixel(typename ChannelTraits::int_type... values) : native_value(0) {
            helpers::pixel_init_impl<type,0,ChannelTraits...>::init(*this,values...);
        }
        // initializes the pixel with a set of floating point values between 0 and 1.0
        constexpr inline pixel(bool dummy,typename ChannelTraits::real_type... values) : native_value(0) {
            helpers::pixel_init_impl<type,0,ChannelTraits...>::initf(*this,values...);
        }
        // gets the pixel value in big endian form
        constexpr inline int_type value() const {
            return helpers::order_guard(native_value);
        }
        // sets the pixel value in big endian form
        constexpr inline void value(int_type value) {
            native_value=helpers::order_guard(value);
        }
        // retrieves a channel's metadata by index
        template<size_t Index> using channel_by_index = typename helpers::channel_by_index_impl<type,Index,channels,0,ChannelTraits...>::type;
        // retrieves a channel's metadata by index in cases where the checked version will cause an error
        template<size_t Index> using channel_by_index_unchecked = typename helpers::channel_by_index_unchecked_impl<type,Index,channels,0,ChannelTraits...>::type;
        // gets the index of the channel by the channel name
        template<typename Name> using channel_index_by_name = typename helpers::channel_index_by_name_impl<0,Name,ChannelTraits...>;
        // gets the channel by name
        template<typename Name> using channel_by_name = channel_by_index<helpers::channel_index_by_name_impl<0,Name,ChannelTraits...>::value>;
        // returns true if the pixel contains channels with each name
        template<typename... ChannelNames> using has_channel_names = typename helpers::has_channel_names_impl<type,ChannelNames...>;
        // returns true if this channel is a subset of the other
        template<typename PixelRhs> using is_subset_of = typename helpers::is_subset_pixel_impl<PixelRhs,ChannelTraits...>;
        // returns true if this channel is a superset of the other
        template<typename PixelRhs> using is_superset_of = typename PixelRhs::template is_subset_of<type>;
        // returns true if the two pixels have channels with the same names, regardless of order
        template<typename PixelRhs> using unordered_equals = typename helpers::unordered_equals_pixel_impl<type,PixelRhs>;
        // returns true if the two pixels have channels with the same names, in the same order
        template<typename PixelRhs> using equals = typename helpers::equals_pixel_impl<PixelRhs,ChannelTraits...>;
        // retrieves the integer channel value without performing compile time checking on Index
        template<size_t Index>
        constexpr inline typename channel_by_index_unchecked<Index>::int_type channel_unchecked() const {
            return helpers::get_channel_direct_unchecked<type,Index>(native_value);
        }
        // sets the integer channel value without performing compile time checking on Index
        template<size_t Index>
        constexpr inline void channel_unchecked(typename channel_by_index_unchecked<Index>::int_type value) {
            native_value = helpers::set_channel_direct_unchecked<type,Index>(native_value,value);
        }
        // retrieves the integer channel value by index
        template<size_t Index>
        constexpr inline typename channel_by_index<Index>::int_type channel() const {
            using ch = channel_by_index<Index>;
            return typename ch::int_type(typename ch::pixel_type::int_type(native_value & ch::channel_mask)>>ch::total_bits_to_right);
            
        }
        // sets the integer channel value by index
        template<size_t Index>
        constexpr inline void channel(typename channel_by_index<Index>::int_type value) {
            using ch = channel_by_index<Index>;
            const typename ch::pixel_type::int_type shval = typename ch::pixel_type::int_type(typename ch::pixel_type::int_type(helpers::clamp(value,ch::min,ch::max))<<ch::total_bits_to_right);
            native_value=typename ch::pixel_type::int_type((native_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
        }
        // retrieves the floating point channel value by index
        template<size_t Index>
        constexpr inline auto channelf() const {
            using ch = channel_by_index<Index>;
            return channel<Index>()*ch::scalef;
        }
        // sets the floating point channel value by index
        template<size_t Index>
        constexpr inline void channelf(typename channel_by_index<Index>::real_type value) {
            using ch = channel_by_index<Index>;
            channel<Index>(value*ch::scale+.5);
        }
        // retrieves the integer channel value by name
        template<typename Name>
        constexpr inline auto channel() const {
            const size_t index = channel_index_by_name<Name>::value;
            return channel<index>();
        }
        // sets the integer channel value by name
        template<typename Name>
        constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name>::value>::int_type value) {
            const size_t index = channel_index_by_name<Name>::value;
            channel<index>(value);
        }
        // gets the floating point channel value by name
        template<typename Name>
        constexpr inline auto channelf() const {
            return channelf<value>();
        }
        // sets the floating point channel value by name
        template<typename Name>
        constexpr inline void channelf(typename channel_by_name<Name>::real_type value) {
            const size_t index = channel_index_by_name<Name>::value;
            channelf<index>(value);
        }
        // converts a pixel to the destination pixel type
        template<typename PixelTypeRhs>
        constexpr inline bool convert(PixelTypeRhs* result) const {
            if(nullptr==result) return false;
            bool good = false;
            typename PixelTypeRhs::int_type native_value = 0;

            // here's where we gather color model information
            using is_rgb = has_channel_names<channel_name::R,channel_name::G,channel_name::B>;
            using is_yuv = has_channel_names<channel_name::Y,channel_name::U,channel_name::V>;
            using trhas_alpha = typename PixelTypeRhs::template has_channel_names<channel_name::A>;
            const bool has_alpha = has_channel_names<channel_name::A>::value;
            const bool is_bw_candidate = 1==channels || (2==channels && has_alpha);
            using tis_bw_candidate = has_channel_names<channel_name::L>;
            const bool is_bw_candidate2 = tis_bw_candidate::value;
            const bool rhas_alpha = trhas_alpha::value;
            const bool ris_bw_candidate = 1==PixelTypeRhs::channels || (2==PixelTypeRhs::channels && rhas_alpha);
            using tris_bw_candidate = typename PixelTypeRhs::template has_channel_names<channel_name::L>;
            const bool ris_bw_candidate2 = tris_bw_candidate::value;
            using is_rhs_rgb = typename PixelTypeRhs::template has_channel_names<channel_name::R,channel_name::G,channel_name::B>;
            using is_rhs_yuv = typename PixelTypeRhs::template has_channel_names<channel_name::Y,channel_name::U,channel_name::V>;

            // TODO: Add code for determining other additional color models here

            // check the source color model
            if(is_rgb::value && channels<5) {
                // source color model is RGB
                using tindexR = channel_index_by_name<channel_name::R>;
                using tchR = channel_by_index_unchecked<tindexR::value>;
                const size_t chiR = tindexR::value;
                    
                using tindexG = channel_index_by_name<channel_name::G>;
                using tchG = channel_by_index_unchecked<tindexG::value>;
                const size_t chiG = tindexG::value;

                using tindexB = channel_index_by_name<channel_name::B>;
                using tchB = channel_by_index_unchecked<tindexB::value>;
                const size_t chiB = tindexB::value;
                
                if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {      
                    // destination color model is RGB
                    using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                    using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                    
                    using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                    using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
                
                    using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                    using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                    
                    auto chR = channel_unchecked<chiR>();
                    auto cR = helpers::convert_channel_depth<tchR,trchR>(chR);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,cR);

                    auto chG = channel_unchecked<chiG>();
                    auto cG = helpers::convert_channel_depth<tchG,trchG>(chG);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,cG);

                    auto chB = channel_unchecked<chiB>();
                    auto cB = helpers::convert_channel_depth<tchB,trchB>(chB);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,cB);

                    good = true;
                } else if(is_rhs_yuv::value && PixelTypeRhs::channels<5)  {
                    // destination is Y'UV color model
                    using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                    using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
                    
                    using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
                    using trchU = typename PixelTypeRhs::template channel_by_index_unchecked<trindexU::value>;
                
                    using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
                    using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;
                    
                    const auto cR = (channel_unchecked<chiR>()*tchR::scalef)*255.0;
                    const auto cG = (channel_unchecked<chiG>()*tchG::scalef)*255.0;
                    const auto cB = (channel_unchecked<chiB>()*tchB::scalef)*255.0;
                    const auto chY =  (0.257 * cR + 0.504 * cG + 0.098 * cB +  16)/255.0;
                    const auto chU = (-0.148 * cR - 0.291 * cG + 0.439 * cB + 128)/255.0;
                    const auto chV = (0.439 * cR - 0.368 * cG - 0.071 * cB + 128)/255.0;
                    const typename trchY::int_type cY =chY*trchY::scale+.5;
                    const typename trchU::int_type cU = chU*trchU::scale+.5;
                    const typename trchV::int_type cV = chV*trchV::scale+.5;
                    
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,cY);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexU::value>(native_value,cU);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexV::value>(native_value,cV);
                    
                    good = true;
                } else if(ris_bw_candidate && ris_bw_candidate2) {
                    // destination is grayscale or monochrome
                    using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                    using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
                    auto cR = channel_unchecked<chiR>();
                    auto cG = channel_unchecked<chiG>();
                    auto cB = channel_unchecked<chiB>();
                    double f = (cR*tchR::scalef*.299) +
                            (cG*tchG::scalef*.587) +
                            (cB*tchB::scalef*.114);
                    const size_t scale = trchL::scale;
                    f=(f*(double)scale)+.5;
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,f);
                
                    good = true;
                } // TODO: add destination color models other than RGB and grayscale/mono
            } else if(is_bw_candidate && is_bw_candidate2) {
                // source is grayscale or monochrome
                using tindexL = channel_index_by_name<channel_name::L>;
                using tchL = channel_by_index_unchecked<tindexL::value>;
                const size_t chiL = tindexL::value;
                
                if(ris_bw_candidate && ris_bw_candidate2) {
                    // destination color model is grayscale or monochrome
                    using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                    using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
                    
                    typename trchL::int_type chL = helpers::convert_channel_depth<tchL,trchL>(channel_unchecked<chiL>());
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,chL);

                    good = true;
                } else if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {
                    // destination color model is RGB
                    using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                    using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                    
                    using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                    using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
                
                    using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                    using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                    
                    const auto chL = channel_unchecked<chiL>();
                    
                    typename trchR::int_type chR = helpers::convert_channel_depth<tchL,trchR>(chL);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,chR);

                    auto chG = helpers::convert_channel_depth<tchL,trchG>(chL);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,chG);

                    auto chB = helpers::convert_channel_depth<tchL,trchB>(chL);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,chB);

                    good = true;
                } else if(is_rhs_yuv::value && PixelTypeRhs::channels<5) {
                    using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                    using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
                    using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
                    using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
                    const auto chY=channel_unchecked<tindexL::value>();
                    auto cY = helpers::convert_channel_depth<tchL,trchY>(chY);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,cY);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexU::value>(native_value,0);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexV::value>(native_value,0);

                    good = true;
                }
            } else if(is_yuv::value && channels<5) {
                // source color model is Y'UV
                using tindexY = channel_index_by_name<channel_name::Y>;
                using tchY = channel_by_index_unchecked<tindexY::value>;
                const size_t chiY = tindexY::value;
                    
                using tindexU = channel_index_by_name<channel_name::U>;
                using tchU = channel_by_index_unchecked<tindexU::value>;
                const size_t chiU = tindexU::value;

                using tindexV = channel_index_by_name<channel_name::V>;
                using tchV = channel_by_index_unchecked<tindexV::value>;
                const size_t chiV = tindexV::value;

                if(is_rhs_yuv::value && PixelTypeRhs::channels<5) {
                    // destination color model is YUV
                    using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                    using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
                    
                    using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
                    using trchU = typename PixelTypeRhs::template channel_by_index_unchecked<trindexU::value>;
                
                    using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
                    using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;
                
                    auto chY = helpers::convert_channel_depth<tchY,trchY>(channel_unchecked<chiY>());
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,chY);

                    auto chU = helpers::convert_channel_depth<tchU,trchU>(channel_unchecked<chiU>());
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexU::value>(native_value,chU);

                    auto chV = helpers::convert_channel_depth<tchV,trchV>(channel_unchecked<chiV>());
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexV::value>(native_value,chV);

                    good = true;
                } else if(is_rhs_rgb::value && PixelTypeRhs::channels<5)  {
                    // destination color model is RGB
                    using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                    using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                    
                    using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                    using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
                
                    using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                    using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                    
                    double chY = (channel_unchecked<chiY>()*tchY::scalef)*256.0-16;
                    double chU = (channel_unchecked<chiU>()*tchU::scalef)*256.0-128;
                    double chV = (channel_unchecked<chiV>()*tchV::scalef)*256.0-128;
                   
                    auto cR = (1.164 * chY + 1.596 * chV)/255.0; 
                    auto cG = (1.164 * chY - 0.392 * chU - 0.813 * chV)/255.0;
                    auto cB = (1.164 * chY + 2.017 * chU)/255.0;
                   
                    auto r = typename trchR::int_type(cR*trchR::scale+.5);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,r);
                    auto g = typename trchG::int_type(cG*trchG::scale+.5);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,g);
                    auto b = typename trchB::int_type(cB*trchB::scale+.5);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,b);

                    good = true;
                } else if(ris_bw_candidate && ris_bw_candidate2) {
                    // destination is monochrome or grayscale
                    using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                    using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
                    
                    auto cY = channel_unchecked<chiY>();
                    auto chL = helpers::convert_channel_depth<tchY,trchL>(cY);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,chL);
                
                    good = true;
                }
            } // TODO: add more source color models other than RGB and grayscale/mono
            if(good) {
                // now do the alpha channels
                if(has_alpha) {
                    using tindexA = channel_index_by_name<channel_name::A>;
                    const size_t chiA = tindexA::value;
                    using tchA = channel_by_index_unchecked<chiA>;
                    if(rhas_alpha) {
                        using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                        const size_t chirA = trindexA::value;
                        using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<chirA>;
                        auto chA = helpers::convert_channel_depth<tchA,trchA>(channel_unchecked<chiA>());
                        helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexA::value>(native_value,chA);
                    }
                } else {
                    if(rhas_alpha) {
                        using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                        using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<trindexA::value>;
                        helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexA::value>(native_value,trchA::max);
                    }
                }
                // finally, set the result
                result->native_value = native_value;
                return true;
            }  else {
                // if we couldn't convert directly
                // do a chain conversion
                rgb_conversion_type tmp1;
                PixelTypeRhs tmp2;
                good = convert(&tmp1);
                if(good) {
                    good = tmp1.template convert(&tmp2);
                    if(good) {
                        *result=tmp2;
                        return true;
                    }
                }
            }
            // TODO: Add any additional metachannel processing (like alpha above) here
            return false;
        } 
        // converts a pixel to the destination pixel type
        template<typename PixelTypeRhs>
        constexpr inline PixelTypeRhs convert() const {
            PixelTypeRhs result;
            if(!convert(&result)) {
                result.native_value = 0;
            }
            return result;
        }
    
        static_assert(sizeof...(ChannelTraits)>0,"A pixel must have at least one channel trait");
        static_assert(bit_depth<=64,"Bit depth must be less than or equal to 64");
        //static_assert(bit_depth<=32,"Bit depth must be less than or equal to 32 (temporary limitation)");
    };
        // creates an RGB pixel by making each channel 
    // one third of the whole. Any remainder bits
    // are added to the green channel
    template<size_t BitDepth>
    using rgb_pixel = pixel<
        channel_traits<channel_name::R,(BitDepth/3)>,
        channel_traits<channel_name::G,((BitDepth/3)+(BitDepth%3))>,
        channel_traits<channel_name::B,(BitDepth/3)>
    >;
    // creates an RGBA pixel by making each channel 
    // one quarter of the whole. Any remainder bits
    // are added to the green channel
    template<size_t BitDepth>
    using rgba_pixel = pixel<
        channel_traits<channel_name::R,(BitDepth/4)>,
        channel_traits<channel_name::G,((BitDepth/4)+(BitDepth%4))>,
        channel_traits<channel_name::B,(BitDepth/4)>,
        channel_traits<channel_name::A,(BitDepth/4)>
    >;
    // creates a grayscale or monochome pixel
    template<size_t BitDepth>
    using gsc_pixel = pixel<
        channel_traits<channel_name::L,BitDepth>
    >;
    // predefined color values
    template<typename PixelType>
    class color final {
        // we use a super precise 64-bit RGB pixel for this
        using source_type = rgb_pixel<64>;
    public:
        constexpr static const PixelType alice_blue = source_type(true,0.941176470588235,0.972549019607843,1).convert<PixelType>();
        constexpr static const PixelType antique_white = source_type(true,0.980392156862745,0.92156862745098,0.843137254901961).convert<PixelType>();
        constexpr static const PixelType aqua = source_type(true,0,1,1).convert<PixelType>();
        constexpr static const PixelType aquamarine = source_type(true,0.498039215686275,1,0.831372549019608).convert<PixelType>();
        constexpr static const PixelType azure = source_type(true,0.941176470588235,1,1).convert<PixelType>();
        constexpr static const PixelType beige = source_type(true,0.96078431372549,0.96078431372549,0.862745098039216).convert<PixelType>();
        constexpr static const PixelType bisque = source_type(true,1,0.894117647058824,0.768627450980392).convert<PixelType>();
        constexpr static const PixelType black = source_type(true,0,0,0).convert<PixelType>();
        constexpr static const PixelType blanched_almond = source_type(true,1,0.92156862745098,0.803921568627451).convert<PixelType>();
        constexpr static const PixelType blue = source_type(true,0,0,1).convert<PixelType>();
        constexpr static const PixelType blue_violet = source_type(true,0.541176470588235,0.168627450980392,0.886274509803922).convert<PixelType>();
        constexpr static const PixelType brown = source_type(true,0.647058823529412,0.164705882352941,0.164705882352941).convert<PixelType>();
        constexpr static const PixelType burly_wood = source_type(true,0.870588235294118,0.72156862745098,0.529411764705882).convert<PixelType>();
        constexpr static const PixelType cadet_blue = source_type(true,0.372549019607843,0.619607843137255,0.627450980392157).convert<PixelType>();
        constexpr static const PixelType chartreuse = source_type(true,0.498039215686275,1,0).convert<PixelType>();
        constexpr static const PixelType chocolate = source_type(true,0.823529411764706,0.411764705882353,0.117647058823529).convert<PixelType>();
        constexpr static const PixelType coral = source_type(true,1,0.498039215686275,0.313725490196078).convert<PixelType>();
        constexpr static const PixelType cornflower_blue = source_type(true,0.392156862745098,0.584313725490196,0.929411764705882).convert<PixelType>();
        constexpr static const PixelType cornsilk = source_type(true,1,0.972549019607843,0.862745098039216).convert<PixelType>();
        constexpr static const PixelType crimson = source_type(true,0.862745098039216,0.0784313725490196,0.235294117647059).convert<PixelType>();
        constexpr static const PixelType cyan = source_type(true,0,1,1).convert<PixelType>();
        constexpr static const PixelType dark_blue = source_type(true,0,0,0.545098039215686).convert<PixelType>();
        constexpr static const PixelType dark_cyan = source_type(true,0,0.545098039215686,0.545098039215686).convert<PixelType>();
        constexpr static const PixelType dark_goldenrod = source_type(true,0.72156862745098,0.525490196078431,0.0431372549019608).convert<PixelType>();
        constexpr static const PixelType dark_gray = source_type(true,0.662745098039216,0.662745098039216,0.662745098039216).convert<PixelType>();
        constexpr static const PixelType dark_green = source_type(true,0,0.392156862745098,0).convert<PixelType>();
        constexpr static const PixelType dark_khaki = source_type(true,0.741176470588235,0.717647058823529,0.419607843137255).convert<PixelType>();
        constexpr static const PixelType dark_magenta = source_type(true,0.545098039215686,0,0.545098039215686).convert<PixelType>();
        constexpr static const PixelType dark_olive_green = source_type(true,0.333333333333333,0.419607843137255,0.184313725490196).convert<PixelType>();
        constexpr static const PixelType dark_orange = source_type(true,1,0.549019607843137,0).convert<PixelType>();
        constexpr static const PixelType dark_orchid = source_type(true,0.6,0.196078431372549,0.8).convert<PixelType>();
        constexpr static const PixelType dark_red = source_type(true,0.545098039215686,0,0).convert<PixelType>();
        constexpr static const PixelType dark_salmon = source_type(true,0.913725490196078,0.588235294117647,0.47843137254902).convert<PixelType>();
        constexpr static const PixelType dark_sea_green = source_type(true,0.56078431372549,0.737254901960784,0.545098039215686).convert<PixelType>();
        constexpr static const PixelType dark_slate_blue = source_type(true,0.282352941176471,0.23921568627451,0.545098039215686).convert<PixelType>();
        constexpr static const PixelType dark_slate_gray = source_type(true,0.184313725490196,0.309803921568627,0.309803921568627).convert<PixelType>();
        constexpr static const PixelType dark_turquoise = source_type(true,0,0.807843137254902,0.819607843137255).convert<PixelType>();
        constexpr static const PixelType dark_violet = source_type(true,0.580392156862745,0,0.827450980392157).convert<PixelType>();
        constexpr static const PixelType deep_pink = source_type(true,1,0.0784313725490196,0.576470588235294).convert<PixelType>();
        constexpr static const PixelType deep_sky_blue = source_type(true,0,0.749019607843137,1).convert<PixelType>();
        constexpr static const PixelType dim_gray = source_type(true,0.411764705882353,0.411764705882353,0.411764705882353).convert<PixelType>();
        constexpr static const PixelType dodger_blue = source_type(true,0.117647058823529,0.564705882352941,1).convert<PixelType>();
        constexpr static const PixelType firebrick = source_type(true,0.698039215686274,0.133333333333333,0.133333333333333).convert<PixelType>();
        constexpr static const PixelType floral_white = source_type(true,1,0.980392156862745,0.941176470588235).convert<PixelType>();
        constexpr static const PixelType forest_green = source_type(true,0.133333333333333,0.545098039215686,0.133333333333333).convert<PixelType>();
        constexpr static const PixelType fuchsia = source_type(true,1,0,1).convert<PixelType>();
        constexpr static const PixelType gainsboro = source_type(true,0.862745098039216,0.862745098039216,0.862745098039216).convert<PixelType>();
        constexpr static const PixelType ghost_white = source_type(true,0.972549019607843,0.972549019607843,1).convert<PixelType>();
        constexpr static const PixelType gold = source_type(true,1,0.843137254901961,0).convert<PixelType>();
        constexpr static const PixelType goldenrod = source_type(true,0.854901960784314,0.647058823529412,0.125490196078431).convert<PixelType>();
        constexpr static const PixelType gray = source_type(true,0.501960784313725,0.501960784313725,0.501960784313725).convert<PixelType>();
        constexpr static const PixelType green = source_type(true,0,0.501960784313725,0).convert<PixelType>();
        constexpr static const PixelType green_yellow = source_type(true,0.67843137254902,1,0.184313725490196).convert<PixelType>();
        constexpr static const PixelType honeydew = source_type(true,0.941176470588235,1,0.941176470588235).convert<PixelType>();
        constexpr static const PixelType hot_pink = source_type(true,1,0.411764705882353,0.705882352941177).convert<PixelType>();
        constexpr static const PixelType indian_red = source_type(true,0.803921568627451,0.36078431372549,0.36078431372549).convert<PixelType>();
        constexpr static const PixelType indigo = source_type(true,0.294117647058824,0,0.509803921568627).convert<PixelType>();
        constexpr static const PixelType ivory = source_type(true,1,1,0.941176470588235).convert<PixelType>();
        constexpr static const PixelType khaki = source_type(true,0.941176470588235,0.901960784313726,0.549019607843137).convert<PixelType>();
        constexpr static const PixelType lavender = source_type(true,0.901960784313726,0.901960784313726,0.980392156862745).convert<PixelType>();
        constexpr static const PixelType lavender_blush = source_type(true,1,0.941176470588235,0.96078431372549).convert<PixelType>();
        constexpr static const PixelType lawn_green = source_type(true,0.486274509803922,0.988235294117647,0).convert<PixelType>();
        constexpr static const PixelType lemon_chiffon = source_type(true,1,0.980392156862745,0.803921568627451).convert<PixelType>();
        constexpr static const PixelType light_blue = source_type(true,0.67843137254902,0.847058823529412,0.901960784313726).convert<PixelType>();
        constexpr static const PixelType light_coral = source_type(true,0.941176470588235,0.501960784313725,0.501960784313725).convert<PixelType>();
        constexpr static const PixelType light_cyan = source_type(true,0.87843137254902,1,1).convert<PixelType>();
        constexpr static const PixelType light_goldenrod_yellow = source_type(true,0.980392156862745,0.980392156862745,0.823529411764706).convert<PixelType>();
        constexpr static const PixelType light_green = source_type(true,0.564705882352941,0.933333333333333,0.564705882352941).convert<PixelType>();
        constexpr static const PixelType light_gray = source_type(true,0.827450980392157,0.827450980392157,0.827450980392157).convert<PixelType>();
        constexpr static const PixelType light_pink = source_type(true,1,0.713725490196078,0.756862745098039).convert<PixelType>();
        constexpr static const PixelType light_salmon = source_type(true,1,0.627450980392157,0.47843137254902).convert<PixelType>();
        constexpr static const PixelType light_sea_green = source_type(true,0.125490196078431,0.698039215686274,0.666666666666667).convert<PixelType>();
        constexpr static const PixelType light_sky_blue = source_type(true,0.529411764705882,0.807843137254902,0.980392156862745).convert<PixelType>();
        constexpr static const PixelType light_slate_gray = source_type(true,0.466666666666667,0.533333333333333,0.6).convert<PixelType>();
        constexpr static const PixelType light_steel_blue = source_type(true,0.690196078431373,0.768627450980392,0.870588235294118).convert<PixelType>();
        constexpr static const PixelType light_yellow = source_type(true,1,1,0.87843137254902).convert<PixelType>();
        constexpr static const PixelType lime = source_type(true,0,1,0).convert<PixelType>();
        constexpr static const PixelType lime_green = source_type(true,0.196078431372549,0.803921568627451,0.196078431372549).convert<PixelType>();
        constexpr static const PixelType linen = source_type(true,0.980392156862745,0.941176470588235,0.901960784313726).convert<PixelType>();
        constexpr static const PixelType magenta = source_type(true,1,0,1).convert<PixelType>();
        constexpr static const PixelType maroon = source_type(true,0.501960784313725,0,0).convert<PixelType>();
        constexpr static const PixelType medium_aquamarine = source_type(true,0.4,0.803921568627451,0.666666666666667).convert<PixelType>();
        constexpr static const PixelType medium_blue = source_type(true,0,0,0.803921568627451).convert<PixelType>();
        constexpr static const PixelType medium_orchid = source_type(true,0.729411764705882,0.333333333333333,0.827450980392157).convert<PixelType>();
        constexpr static const PixelType medium_purple = source_type(true,0.576470588235294,0.43921568627451,0.858823529411765).convert<PixelType>();
        constexpr static const PixelType medium_sea_green = source_type(true,0.235294117647059,0.701960784313725,0.443137254901961).convert<PixelType>();
        constexpr static const PixelType medium_slate_blue = source_type(true,0.482352941176471,0.407843137254902,0.933333333333333).convert<PixelType>();
        constexpr static const PixelType medium_spring_green = source_type(true,0,0.980392156862745,0.603921568627451).convert<PixelType>();
        constexpr static const PixelType medium_turquoise = source_type(true,0.282352941176471,0.819607843137255,0.8).convert<PixelType>();
        constexpr static const PixelType medium_violet_red = source_type(true,0.780392156862745,0.0823529411764706,0.52156862745098).convert<PixelType>();
        constexpr static const PixelType midnight_blue = source_type(true,0.0980392156862745,0.0980392156862745,0.43921568627451).convert<PixelType>();
        constexpr static const PixelType mint_cream = source_type(true,0.96078431372549,1,0.980392156862745).convert<PixelType>();
        constexpr static const PixelType misty_rose = source_type(true,1,0.894117647058824,0.882352941176471).convert<PixelType>();
        constexpr static const PixelType moccasin = source_type(true,1,0.894117647058824,0.709803921568627).convert<PixelType>();
        constexpr static const PixelType navajo_white = source_type(true,1,0.870588235294118,0.67843137254902).convert<PixelType>();
        constexpr static const PixelType navy = source_type(true,0,0,0.501960784313725).convert<PixelType>();
        constexpr static const PixelType old_lace = source_type(true,0.992156862745098,0.96078431372549,0.901960784313726).convert<PixelType>();
        constexpr static const PixelType olive = source_type(true,0.501960784313725,0.501960784313725,0).convert<PixelType>();
        constexpr static const PixelType olive_drab = source_type(true,0.419607843137255,0.556862745098039,0.137254901960784).convert<PixelType>();
        constexpr static const PixelType orange = source_type(true,1,0.647058823529412,0).convert<PixelType>();
        constexpr static const PixelType orange_red = source_type(true,1,0.270588235294118,0).convert<PixelType>();
        constexpr static const PixelType orchid = source_type(true,0.854901960784314,0.43921568627451,0.83921568627451).convert<PixelType>();
        constexpr static const PixelType pale_goldenrod = source_type(true,0.933333333333333,0.909803921568627,0.666666666666667).convert<PixelType>();
        constexpr static const PixelType pale_green = source_type(true,0.596078431372549,0.984313725490196,0.596078431372549).convert<PixelType>();
        constexpr static const PixelType pale_turquoise = source_type(true,0.686274509803922,0.933333333333333,0.933333333333333).convert<PixelType>();
        constexpr static const PixelType pale_violet_red = source_type(true,0.858823529411765,0.43921568627451,0.576470588235294).convert<PixelType>();
        constexpr static const PixelType papaya_whip = source_type(true,1,0.937254901960784,0.835294117647059).convert<PixelType>();
        constexpr static const PixelType peach_puff = source_type(true,1,0.854901960784314,0.725490196078431).convert<PixelType>();
        constexpr static const PixelType peru = source_type(true,0.803921568627451,0.52156862745098,0.247058823529412).convert<PixelType>();
        constexpr static const PixelType pink = source_type(true,1,0.752941176470588,0.796078431372549).convert<PixelType>();
        constexpr static const PixelType plum = source_type(true,0.866666666666667,0.627450980392157,0.866666666666667).convert<PixelType>();
        constexpr static const PixelType powder_blue = source_type(true,0.690196078431373,0.87843137254902,0.901960784313726).convert<PixelType>();
        constexpr static const PixelType purple = source_type(true,0.501960784313725,0,0.501960784313725).convert<PixelType>();
        constexpr static const PixelType red = source_type(true,1,0,0).convert<PixelType>();
        constexpr static const PixelType rosy_brown = source_type(true,0.737254901960784,0.56078431372549,0.56078431372549).convert<PixelType>();
        constexpr static const PixelType royal_blue = source_type(true,0.254901960784314,0.411764705882353,0.882352941176471).convert<PixelType>();
        constexpr static const PixelType saddle_brown = source_type(true,0.545098039215686,0.270588235294118,0.0745098039215686).convert<PixelType>();
        constexpr static const PixelType salmon = source_type(true,0.980392156862745,0.501960784313725,0.447058823529412).convert<PixelType>();
        constexpr static const PixelType sandy_brown = source_type(true,0.956862745098039,0.643137254901961,0.376470588235294).convert<PixelType>();
        constexpr static const PixelType sea_green = source_type(true,0.180392156862745,0.545098039215686,0.341176470588235).convert<PixelType>();
        constexpr static const PixelType sea_shell = source_type(true,1,0.96078431372549,0.933333333333333).convert<PixelType>();
        constexpr static const PixelType sienna = source_type(true,0.627450980392157,0.32156862745098,0.176470588235294).convert<PixelType>();
        constexpr static const PixelType silver = source_type(true,0.752941176470588,0.752941176470588,0.752941176470588).convert<PixelType>();
        constexpr static const PixelType sky_blue = source_type(true,0.529411764705882,0.807843137254902,0.92156862745098).convert<PixelType>();
        constexpr static const PixelType slate_blue = source_type(true,0.415686274509804,0.352941176470588,0.803921568627451).convert<PixelType>();
        constexpr static const PixelType slate_gray = source_type(true,0.43921568627451,0.501960784313725,0.564705882352941).convert<PixelType>();
        constexpr static const PixelType snow = source_type(true,1,0.980392156862745,0.980392156862745).convert<PixelType>();
        constexpr static const PixelType spring_green = source_type(true,0,1,0.498039215686275).convert<PixelType>();
        constexpr static const PixelType steel_blue = source_type(true,0.274509803921569,0.509803921568627,0.705882352941177).convert<PixelType>();
        constexpr static const PixelType tan = source_type(true,0.823529411764706,0.705882352941177,0.549019607843137).convert<PixelType>();
        constexpr static const PixelType teal = source_type(true,0,0.501960784313725,0.501960784313725).convert<PixelType>();
        constexpr static const PixelType thistle = source_type(true,0.847058823529412,0.749019607843137,0.847058823529412).convert<PixelType>();
        constexpr static const PixelType tomato = source_type(true,1,0.388235294117647,0.27843137254902).convert<PixelType>();
        constexpr static const PixelType turquoise = source_type(true,0.250980392156863,0.87843137254902,0.815686274509804).convert<PixelType>();
        constexpr static const PixelType violet = source_type(true,0.933333333333333,0.509803921568627,0.933333333333333).convert<PixelType>();
        constexpr static const PixelType wheat = source_type(true,0.96078431372549,0.870588235294118,0.701960784313725).convert<PixelType>();
        constexpr static const PixelType white = source_type(true,1,1,1).convert<PixelType>();
        constexpr static const PixelType white_smoke = source_type(true,0.96078431372549,0.96078431372549,0.96078431372549).convert<PixelType>();
        constexpr static const PixelType yellow = source_type(true,1,1,0).convert<PixelType>();
        constexpr static const PixelType yellow_green = source_type(true,0.603921568627451,0.803921568627451,0.196078431372549).convert<PixelType>();
        // ensure colors can be converted to destination type:
        static_assert(0!=white.native_value || 0!=black.native_value,"Color type is not convertible from RGB");
    };
}
#endif
