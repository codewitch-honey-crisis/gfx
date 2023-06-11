#ifndef HTCW_GFX_PIXEL_HPP
#define HTCW_GFX_PIXEL_HPP
#include "gfx_core.hpp"
#include <math.h>
#define GFX_CHANNEL_NAME(x) struct x { static inline constexpr const char* value() { return #x; } };
namespace gfx {
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
        // Cb for YCbCr such as jpeg
        GFX_CHANNEL_NAME(Cb)
        // Cr for YCbCr such as jpeg
        GFX_CHANNEL_NAME(Cr)
        // for CMYK
        GFX_CHANNEL_NAME(C)
        // for CMYK
        GFX_CHANNEL_NAME(M)
        // for CMYK
        GFX_CHANNEL_NAME(K)
        // index (for indexed color)
        GFX_CHANNEL_NAME(index)
        // non-op
        GFX_CHANNEL_NAME(nop)
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
#if HTCW_MAX_WORD >= 64
        bits::uintx<bits::get_word_size(BitDepth)> Max= ((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1)), 
#else
        bits::uintx<bits::get_word_size(BitDepth)> Max= ((BitDepth==32)?0xFFFFFFFF:((1<<BitDepth)-1)), 
#endif
        // the default value
        bits::uintx<bits::get_word_size(BitDepth)> Default = Min,
        // the scale denominator
        bits::uintx<bits::get_word_size(BitDepth)> Scale = Max
    >
    struct channel_traits {
        // this type
        using type = channel_traits<Name,BitDepth,Min,Max,Scale>;
        // the type that represents the name
        using name_type = Name;
        // the integer type of this channel
        using int_type = bits::uintx<bits::get_word_size(BitDepth)>;
        // the floating point type of this channel
        using real_type = bits::realx<16<=BitDepth?HTCW_MAX_WORD:32>;
        // the bit depth of this channel
        constexpr static const size_t bit_depth = BitDepth;
        // the minimum value
        constexpr static const int_type min = Min;
        // the maximum value
        constexpr static const int_type max = Max;
        // the default value
        constexpr static const int_type default_ = Default;
        // the scale denominator
        constexpr static const int_type scale = Scale;
        // the reciprocal of the scale denominator
        constexpr static const real_type scaler = 1/(real_type)Scale; 
        // a mask of the int value
        constexpr static const int_type int_mask = ~int_type(0);
        // a mask of the channel value
        constexpr static const int_type mask = bits::mask<BitDepth>::right;//=int_type(int_mask>>((sizeof(int_type)*8)-BitDepth));
        // constraints
        static_assert(BitDepth>0,"Bit depth must be greater than 0");
        static_assert(BitDepth<=64,"Bit depth must be less than or equal to 64");
        static_assert(Min<=Max,"Min must be less than or equal to Max");
        static_assert(Default>=Min,"Default must be greater than or equal to the minimum value");
        static_assert(Default<=Max,"Default must be less than or equal to the maximum value");
#if HTCW_MAX_WORD >= 64        
        static_assert(Max<=((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1)),"Max is greater than the maximum allowable value");
#else
        static_assert(Max<=((BitDepth==32)?0xFFFFFFFF:((1<<BitDepth)-1)),"Max is greater than the maximum allowable value");
#endif
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
        constexpr static const int_type default_=0;
        constexpr static const int_type scale = 0;
        constexpr static const float scaler = 0.0; 
        constexpr static const int_type int_mask = 0;
        constexpr static const int_type mask = 0;
    };
    
    // represents a channel's metadata
    template<typename PixelType,typename ChannelTraits,
            int Index,
            size_t BitsToLeft> 
    struct pixel_channel final {
        // the declaring pixel's type
        using pixel_type = PixelType;
        // this type
        using type = pixel_channel<pixel_type,ChannelTraits,Index,BitsToLeft>;
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
        // the default value for the channel
        constexpr static const int_type default_ = ChannelTraits::default_;  
        // the scale denominator
        constexpr static const int_type scale = ChannelTraits::scale;
        // the reciprocal of the scale denominator
        constexpr static const real_type scaler = ChannelTraits::scaler;

        
    };
    // various utility templates and methods
    namespace helpers {
        
        template <typename... ChannelTraits> struct bit_depth;
        template <typename T, typename... ChannelTraits>
        struct bit_depth<T, ChannelTraits...> {
            static constexpr const size_t value = T::bit_depth + bit_depth<ChannelTraits...>::value;
        };
        template <> struct bit_depth<> { static const size_t value = 0; };
        
        template<typename PixelType,int Index,int Count,size_t BitsToLeft,typename... ChannelTraits>
        struct channel_by_index_impl;        
        template<typename PixelType,int Index,int Count, size_t BitsToLeft, typename ChannelTrait,typename... ChannelTraits>
        struct channel_by_index_impl<PixelType,Index,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = typename channel_by_index_impl<PixelType,Index-1,Count+1,BitsToLeft+ChannelTrait::bit_depth, ChannelTraits...>::type;
        };
        template <typename PixelType,int Count,size_t BitsToLeft,typename ChannelTrait, typename ...ChannelTraits>
        struct channel_by_index_impl<PixelType,0,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = pixel_channel<PixelType,ChannelTrait,Count,BitsToLeft>;
        };
        template<typename PixelType,int Index,int Count,size_t BitsToLeft> 
        struct channel_by_index_impl<PixelType,Index,Count,BitsToLeft> {
            using type = void;
        };

        template<typename PixelType,int Index,int Count,size_t BitsToLeft,typename... ChannelTraits>
        struct channel_by_index_unchecked_impl;        
        template<typename PixelType,int Index,int Count, size_t BitsToLeft, typename ChannelTrait,typename... ChannelTraits>
        struct channel_by_index_unchecked_impl<PixelType,Index,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = typename channel_by_index_unchecked_impl<PixelType,Index-1,Count+1,BitsToLeft+ChannelTrait::bit_depth, ChannelTraits...>::type;
        };
        template <typename PixelType,int Count,size_t BitsToLeft,typename ChannelTrait, typename ...ChannelTraits>
        struct channel_by_index_unchecked_impl<PixelType,0,Count,BitsToLeft,ChannelTrait,ChannelTraits...> {
            using type = pixel_channel<PixelType,ChannelTrait,Count,BitsToLeft>;
        };
        template<typename PixelType,int Index,int Count,size_t BitsToLeft> 
        struct channel_by_index_unchecked_impl<PixelType,Index,Count,BitsToLeft> {
            using type = pixel_channel<PixelType,channel_traits<channel_name::nop,0,0,0,0>,0,0>;
        };

        template<typename PixelType,int Count,typename... ChannelTraits>
        struct pixel_init_impl;        
        template<typename PixelType,int Count, typename ChannelTrait,typename... ChannelTraits>
        struct pixel_init_impl<PixelType,Count,ChannelTrait,ChannelTraits...> {
            using ch = typename PixelType::template channel_by_index<Count>;
            using next = pixel_init_impl<PixelType,Count+1, ChannelTraits...>;
            constexpr static inline void init(PixelType& pixel) {
                if(ChannelTrait::bit_depth==0) return;
                pixel.native_value = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(ch::default_,ch::min,ch::max))<<ch::total_bits_to_right);
                
                next::init(pixel);
            }
            constexpr static inline void init(PixelType& pixel,typename ChannelTrait::int_type value, typename ChannelTraits::int_type... values) {
                if(ChannelTrait::bit_depth==0) return;
                const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(value,ch::min,ch::max))<<ch::total_bits_to_right);
                pixel.native_value=typename PixelType::int_type((pixel.native_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
                next::init(pixel,values...);
            }
            constexpr static inline void initf(PixelType& pixel,typename ChannelTrait::real_type value, typename ChannelTraits::real_type... values) {
                if(ChannelTrait::bit_depth==0) return;
                typename ch::int_type ivalue = helpers::clamp<decltype(value)>(value,0.0,1.0) * ch::scale;
                const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(ivalue)<<ch::total_bits_to_right);
                pixel.native_value=typename PixelType::int_type((pixel.native_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
                next::initf(pixel,values...);
            }
            
        };
        template<typename PixelType,int Count>
        struct pixel_init_impl<PixelType,Count> {
            constexpr static inline void init(PixelType& pixel) {
            
            }
            constexpr static inline void initf(PixelType& pixel) {
            
            }
        };

        template<typename PixelType,int Count,typename... ChannelTraits>
        struct pixel_diff_impl;        
        template<typename PixelType,int Count, typename ChannelTrait,typename... ChannelTraits>
        struct pixel_diff_impl<PixelType,Count,ChannelTrait,ChannelTraits...> {
            using ch = typename PixelType::template channel_by_index<Count>;
            using next = pixel_diff_impl<PixelType,Count+1, ChannelTraits...>;
            constexpr static inline double diff_sum(PixelType lhs,PixelType rhs) {
                constexpr const size_t index = Count;
                if(ChannelTrait::bit_depth==0) return NAN;
                const double d = (lhs.template channelr<index>()-rhs.template channelr<index>());
                return d*d+next::diff_sum(lhs,rhs);
            }
        };
        template<typename PixelType,int Count>
        struct pixel_diff_impl<PixelType,Count> {
            constexpr static inline double diff_sum(PixelType lhs,PixelType rhs) {
                return 0.0;
            }
            
        };

        template<typename PixelType,int Count,typename... ChannelTraits>
        struct pixel_blend_impl;        
        template<typename PixelType,int Count, typename ChannelTrait,typename... ChannelTraits>
        struct pixel_blend_impl<PixelType,Count,ChannelTrait,ChannelTraits...> {
            using ch = typename PixelType::template channel_by_index<Count>;
            using next = pixel_blend_impl<PixelType,Count+1, ChannelTraits...>;
            constexpr static inline void blend_val(PixelType lhs,PixelType rhs,double amount,PixelType* out_pixel) {
                constexpr const size_t index = Count;
                if(ChannelTrait::bit_depth==0) return;
                //const double l = lhs.template channelr<index>()*amount;
                //const double r = rhs.template channelr<index>()*(1.0-amount);
                //out_pixel->template channelr<index>(l+r);
                auto l = lhs.template channel<index>();
                l=l*amount+.5;
                auto r = rhs.template channel<index>();
                r=r*(1.0-amount)+.5;
                out_pixel->template channel<index>(l+r);
                next::blend_val(lhs,rhs,amount,out_pixel);
            }
        };
        template<typename PixelType,int Count>
        struct pixel_blend_impl<PixelType,Count> {
            constexpr static inline void blend_val(PixelType lhs,PixelType rhs,double amount,PixelType* out_pixel) {
            }
            
        };

        template<int Count,typename Name, typename ...ChannelTraits>
        struct channel_index_by_name_impl;
        template<int Count,typename Name>
        struct channel_index_by_name_impl<Count,Name> {
            static constexpr int value=-1;
        };
        template<int Count,typename Name, typename ChannelTrait, typename ...ChannelTraits>
        struct channel_index_by_name_impl<Count,Name, ChannelTrait, ChannelTraits...> {
            static constexpr int value = is_same<Name, typename ChannelTrait::name_type>::value ? Count : channel_index_by_name_impl<Count+1,Name, ChannelTraits...>::value;            
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
            int Index,
            typename... ChannelTraits>
        struct is_equal_pixel_impl;
        template <
            typename PixelType,
            int Index>
        struct is_equal_pixel_impl<PixelType, Index>
        {
            // this is what we return when there
            // are no items left
            static constexpr bool value = true;
        };
        template <typename PixelType,
                int Index,
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
                has_channel_names_impl<PixelType,ChannelNames...>::value;
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
                rv = (typename ChannelRhs::int_type)(v>>(0>srf?0:srf));
                rv = clamp(rv,ChannelRhs::min,ChannelRhs::max);
                //float vs = v*ChannelLhs::scaler;
                //rv = clamp((typename ChannelRhs::int_type)(vs*ChannelRhs::scale+.5),ChannelRhs::min,ChannelRhs::max);
            } else if(0>srf) {
                //rv = (typename ChannelRhs::int_type)(v<<(0<srf?0:-srf));
                //rv = clamp(rv,ChannelRhs::min,ChannelRhs::max);
                float vs = v*ChannelLhs::scaler;
                rv = clamp((typename ChannelRhs::int_type)(vs*ChannelRhs::scale+.5),ChannelRhs::min,ChannelRhs::max);
            } else
                rv = (typename ChannelRhs::int_type)(v);
            return rv;
        }
        // gets the native_value of a channel without doing compile time checking on the index
        template<typename PixelType,int Index>
        constexpr inline typename PixelType::template channel_by_index_unchecked<Index>::int_type get_channel_direct_unchecked(const typename PixelType::int_type& pixel_value) {
            using ch = typename PixelType::template channel_by_index_unchecked<Index>;
            if(0>Index || Index>=(int)PixelType::channels) return 0;
            const typename PixelType::int_type p = pixel_value>>ch::total_bits_to_right;
            const typename ch::int_type result = typename ch::int_type(typename PixelType::int_type(p & typename PixelType::int_type(ch::value_mask)));
            return result;
            
        }
        // sets the native_value of a channel without doing compile time checking on the index
        template<typename PixelType,int Index>
        constexpr inline void 
        
        set_channel_direct_unchecked(typename PixelType::int_type& pixel_value,typename PixelType::template channel_by_index_unchecked<Index>::int_type value) {
            if(0>Index || Index>=(int)PixelType::channels) return;
            using ch = typename PixelType::template channel_by_index_unchecked<Index>;
            const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(value,ch::min,ch::max))<<ch::total_bits_to_right);
            pixel_value=typename PixelType::int_type((pixel_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
        }
        template<typename AlphaTrait,bool HasAlpha, typename... ChannelTraits>
        struct make_alpha_impl {

        };
        
    }
    // represents the pixel base class
    template<typename... ChannelTraits> 
    struct pixel {
        // this type
        using type = pixel<ChannelTraits...>;
        // the type used for doing intermediary conversions to different formats when no explicit conversion is implemented
        using rgb_conversion_type = pixel<channel_traits<channel_name::R,16>,channel_traits<channel_name::G,16>,channel_traits<channel_name::B,16>,channel_traits<channel_name::A,16>>;
        // the integer type of the pixel
        using int_type = bits::uintx<bits::get_word_size(helpers::bit_depth<ChannelTraits...>::value)>;
        // the number of channels
        constexpr static const size_t channels = sizeof...(ChannelTraits);
        // the total bit depth of the pixel
        constexpr static const size_t bit_depth = helpers::bit_depth<ChannelTraits...>::value;
        // the minimum number of bytes needed to store the pixel
        constexpr static const size_t packed_size = (bit_depth+7) / 8;
        // true if the pixel is a whole number of bytes
        constexpr static const bool byte_aligned = 0==bit_depth/8.0 - (size_t)(bit_depth/8);
        // the total size in bits, including padding
        constexpr static const size_t total_size_bits = sizeof(int_type)*8;
        // the packed size, in bits
        constexpr static const size_t packed_size_bits = packed_size*8;
        // the count of bits to the right that are unused
        constexpr static const size_t pad_right_bits = total_size_bits-bit_depth;
        // the mask of the pixel's value
        constexpr static const int_type mask = int_type(int_type(~int_type(0))<<(pad_right_bits));
        
        // the pixel value, in platform native format
        int_type native_value;
        
        // initializes the pixel
        constexpr inline pixel() : native_value(0) {
            helpers::pixel_init_impl<type,0,ChannelTraits...>::init(*this);
        }
        constexpr inline pixel(int_type native_value,bool dummy) : native_value(native_value) {
        }
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
        constexpr inline bool operator==(pixel rhs) {
            return rhs.native_value==native_value;
        }
        constexpr inline bool operator!=(pixel rhs) {
            return rhs.native_value!=native_value;
        }
        // retrieves a channel's metadata by index
        template<int Index> using channel_by_index = typename helpers::channel_by_index_impl<type,Index,channels,0,ChannelTraits...>::type;
        // retrieves a channel's metadata by index in cases where the checked version will cause an error
        template<int Index> using channel_by_index_unchecked = typename helpers::channel_by_index_unchecked_impl<type,Index,channels,0,ChannelTraits...>::type;
        // gets the index of the channel by the channel name
        template<typename Name> using channel_index_by_name = typename helpers::channel_index_by_name_impl<0,Name,ChannelTraits...>;
        // gets the channel by name
        template<typename Name> using channel_by_name = channel_by_index<helpers::channel_index_by_name_impl<0,Name,ChannelTraits...>::value>;
        // retrieves a channel's metadata by name in cases where the checked version will cause an error
        template<typename Name> using channel_by_name_unchecked = channel_by_index_unchecked<channel_index_by_name<Name>::value>;
        
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
        // returns true if the two pixels are exactly the same
        template<typename PixelRhs> using equals_exact = typename helpers::is_same<type,PixelRhs>;
       
        // retrieves the integer channel value without performing compile time checking on Index
        template<int Index>
        constexpr inline typename channel_by_index_unchecked<Index>::int_type channel_unchecked() const {
            return helpers::get_channel_direct_unchecked<type,Index>(native_value);
        }
        // sets the integer channel value without performing compile time checking on Index
        template<int Index>
        constexpr inline void channel_unchecked(typename channel_by_index_unchecked<Index>::int_type value) {
            helpers::set_channel_direct_unchecked<type,Index>(native_value,value);
        }
        // retrieves the integer channel value by index
        template<int Index>
        constexpr inline typename channel_by_index<Index>::int_type channel() const {
            using ch = channel_by_index<Index>;
            return typename ch::int_type(typename ch::pixel_type::int_type(native_value & ch::channel_mask)>>ch::total_bits_to_right);
            
        }
        // sets the integer channel value by index
        template<int Index>
        constexpr inline void channel(typename channel_by_index<Index>::int_type value) {
            using ch = channel_by_index<Index>;
            const typename ch::pixel_type::int_type shval = typename ch::pixel_type::int_type(typename ch::pixel_type::int_type(helpers::clamp(value,ch::min,ch::max))<<ch::total_bits_to_right);
            native_value=typename ch::pixel_type::int_type((native_value&typename ch::pixel_type::int_type(~ch::channel_mask))|shval);
        }
        // retrieves the floating point channel value by index
        template<int Index>
        constexpr inline typename channel_by_index_unchecked<Index>::real_type channelr() const {
            using ch = channel_by_index<Index>;
            return channel<Index>()*ch::scaler;
        }
        // sets the floating point channel value by index
        template<int Index>
        constexpr inline void channelr(typename channel_by_index<Index>::real_type value) {
            using ch = channel_by_index<Index>;
            channel<Index>(value*ch::scale+.5);
        }
        // retrieves the floating point channel value by index
        template<int Index>
        constexpr inline typename channel_by_index_unchecked<Index>::real_type channelr_unchecked() const {
            using ch = channel_by_index_unchecked<Index>;
            return (typename ch::real_type)channel_unchecked<Index>()*ch::scaler;
        }
        // sets the floating point channel value by index
        template<int Index>
        constexpr inline void channelr_unchecked(typename channel_by_index<Index>::real_type value) {
            using ch = channel_by_index_unchecked<Index>;
            channel_unchecked<Index>(value*ch::scale+.5);
        }
        // retrieves the integer channel value by name
        template<typename Name>
        constexpr inline auto channel() const {
            const int index = channel_index_by_name<Name>::value;
            return channel<index>();
        }
        // sets the integer channel value by name
        template<typename Name>
        constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name>::value>::int_type value) {
            const int index = channel_index_by_name<Name>::value;
            channel<index>(value);
        }
        // gets the floating point channel value by name
        template<typename Name>
        constexpr inline auto channelr() const {
            const int index = channel_index_by_name<Name>::value;
            return channelr<index>();
        }
        // sets the floating point channel value by name
        template<typename Name>
        constexpr inline void channelr(typename channel_by_name<Name>::real_type value) {
            const int index = channel_index_by_name<Name>::value;
            channelr<index>(value);
        }
        // returns the difference between two pixels
        constexpr double difference(type rhs) const {
            return sqrt(helpers::pixel_diff_impl<type,0,ChannelTraits...>::diff_sum(*this,rhs));
        }
        // blends two pixels. ratio is between zero and one. larger ratio numbers favor this pixel
        constexpr gfx_result blend(const type rhs,double ratio,type* out_pixel) const {
            if(out_pixel==nullptr) {
                return gfx_result::invalid_argument;
            }
            static_assert(!has_channel_names<channel_name::index>::value,"pixel must not be indexed");
            if(ratio==1.0) {
                out_pixel->native_value = native_value;
                return gfx_result::success;
            } else if(ratio==0.0) {
                out_pixel->native_value = rhs.native_value;
                return gfx_result::success;
            }
            if(type::template has_channel_names<channel_name::A>::value) {
                
                constexpr const int ai = type::channel_index_by_name<channel_name::A>::value;

                float a1 = this->template channelr_unchecked<ai>();
                float a2 = rhs.template channelr_unchecked<ai>();
                float r2 = a1/a2;
                ratio = ratio * r2;
                if(ratio>1.0)
                    ratio = 1.0;               
            }
            
            helpers::pixel_blend_impl<type,0,ChannelTraits...>::blend_val(*this,rhs,ratio,out_pixel);
            return gfx_result::success;
        }
        // blends two pixels. ratio is between zero and one. larger ratio numbers favor this pixel
        type blend(const type rhs,double ratio) const {
            type result;
            blend(rhs,ratio,&result);
            return result;
        }
        
        static_assert(sizeof...(ChannelTraits)>0,"A pixel must have at least one channel trait");
        static_assert(bit_depth<=HTCW_MAX_WORD,"Bit depth must be less than or equal to the maximum machine word size");
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
        channel_traits<channel_name::A,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>
    >;
    // creates a grayscale or monochome pixel
    template<size_t BitDepth>
    using gsc_pixel = pixel<
        channel_traits<channel_name::L,BitDepth>
    >;
    // creates a Y'UV pixel by making each channel 
    // one third of the whole. Any remainder bits
    // are added to the Y' channel
    template<size_t BitDepth>
    using yuv_pixel = pixel<
        channel_traits<channel_name::Y,((BitDepth/3)+(BitDepth%3))>,
        channel_traits<channel_name::U,(BitDepth/3)>,
        channel_traits<channel_name::V,(BitDepth/3)>
    >;
    // creates a Y'UV/A pixel by making each 
    // channel 1/4 of the whole. Remaining bits
    // are added to Y'
    template<size_t BitDepth>
    using yuva_pixel = pixel<
        channel_traits<channel_name::Y,((BitDepth/4)+(BitDepth%4))>,
        channel_traits<channel_name::U,(BitDepth/4)>,
        channel_traits<channel_name::V,(BitDepth/4)>,
        channel_traits<channel_name::A,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>
    >;

    // creates a YCbCr pixel by making each channel 
    // one third of the whole. Any remainder bits
    // are added to the Y channel
    template<size_t BitDepth>
    using ycbcr_pixel = pixel<
        channel_traits<channel_name::Y,((BitDepth/3)+(BitDepth%3))>,
        channel_traits<channel_name::Cb,(BitDepth/3)>,
        channel_traits<channel_name::Cr,(BitDepth/3)>
    >;
    // creates a ycbcr pixel by making each 
    // channel 1/4 of the whole. Remaining bits
    // are added to Y
    template<size_t BitDepth>
    using ycbcra_pixel = pixel<
        channel_traits<channel_name::Y,((BitDepth/4)+(BitDepth%4))>,
        channel_traits<channel_name::Cb,(BitDepth/4)>,
        channel_traits<channel_name::Cr,(BitDepth/4)>,
        channel_traits<channel_name::A,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>
    >;
    // creates an indexed pixel
    template<size_t BitDepth>
    using indexed_pixel=pixel<channel_traits<channel_name::index,BitDepth>>;
    
    // creates a HSV pixel by making each channel 
    // one third of the whole. Any remainder bits
    // are added to the V channel
    template<size_t BitDepth>
    using hsv_pixel = pixel<
        channel_traits<channel_name::H,(BitDepth/3)>,
        channel_traits<channel_name::S,(BitDepth/3)>,
        channel_traits<channel_name::V,((BitDepth/3)+(BitDepth%3))>
    >;
    // creates a HSV/A pixel by making each 
    // channel 1/4 of the whole. Remaining bits
    // are added to V
    template<size_t BitDepth>
    using hsva_pixel = pixel<
        channel_traits<channel_name::H,(BitDepth/4)>,
        channel_traits<channel_name::S,(BitDepth/4)>,
        channel_traits<channel_name::V,((BitDepth/4)+(BitDepth%4))>,
        channel_traits<channel_name::A,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>
    >;

    // creates a HSL pixel by making each channel 
    // one third of the whole. Any remainder bits
    // are added to the L channel
    template<size_t BitDepth>
    using hsl_pixel = pixel<
        channel_traits<channel_name::H,(BitDepth/3)>,
        channel_traits<channel_name::S,(BitDepth/3)>,
        channel_traits<channel_name::L,((BitDepth/3)+(BitDepth%3))>
    >;
    // creates a HSL/A pixel by making each 
    // channel 1/4 of the whole. Remaining bits
    // are added to L
    template<size_t BitDepth>
    using hsla_pixel = pixel<
        channel_traits<channel_name::H,(BitDepth/4)>,
        channel_traits<channel_name::S,(BitDepth/4)>,
        channel_traits<channel_name::L,((BitDepth/4)+(BitDepth%4))>,
        channel_traits<channel_name::A,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>
    >;


    // creates a CMYK pixel by making each channel 
    // one quarter of the whole. Any remainder bits
    // are added to the K channel
    template<size_t BitDepth>
    using cmyk_pixel = pixel<
        channel_traits<channel_name::C,(BitDepth/4)>,
        channel_traits<channel_name::M,(BitDepth/4)>,
        channel_traits<channel_name::Y,(BitDepth/4)>,
        channel_traits<channel_name::K,((BitDepth/4)+(BitDepth%4))>
    >;
    // creates a CMYK/A pixel by making each 
    // channel 1/5 of the whole. Remaining bits
    // are added to K
    template<size_t BitDepth>
    using cmyka_pixel = pixel<
        channel_traits<channel_name::C,(BitDepth/5)>,
        channel_traits<channel_name::M,(BitDepth/5)>,
        channel_traits<channel_name::Y,(BitDepth/5)>,
        channel_traits<channel_name::K,((BitDepth/5)+(BitDepth%5))>,
        channel_traits<channel_name::A,(BitDepth/5),0,(1<<(BitDepth/5))-1,(1<<(BitDepth/5))-1>
    >;
    template<size_t BitDepth>
    using alpha_pixel = pixel<
            channel_traits<
                channel_name::A,
                BitDepth,
                0,
#if HTCW_MAX_WORD >= 64
                ((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1)), 
#else
                ((BitDepth==32)?0xFFFFFFFF:((1<<BitDepth)-1)), 
#endif
#if HTCW_MAX_WORD >= 64
                ((BitDepth==64)?0xFFFFFFFFFFFFFFFF:((1<<BitDepth)-1))
#else
                ((BitDepth==32)?0xFFFFFFFF:((1<<BitDepth)-1))
#endif
>>;
    
    namespace helpers {
        struct dither {
         //	16x16 Bayer Dithering Matrix.  Color levels: 256
            static const int* bayer_16;
        };
        // for HSL conversion
        inline constexpr double hue2rgb(double p, double q, double t){
            if(t < 0.0) t += 1.0;
            if(t > 1.0) t -= 1.0;
            if(t < 1.0/6.0) return p + (q - p) * 6.0 * t;
            if(t < 1.0/2.0) return q;
            if(t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
            return p;
        }
		inline constexpr double clampcymk(double value) {
            if((value!=value) || value < 0.0) {
                return 0.0;
            }
            return value;
        }
    }

    
    // converts a pixel to the destination pixel type
    template<typename PixelTypeLhs, typename PixelTypeRhs>
    constexpr static inline gfx_result convert(PixelTypeLhs source,PixelTypeRhs* result,const PixelTypeRhs* background=nullptr) {
        static_assert(helpers::is_same<PixelTypeLhs,PixelTypeRhs>::value || !PixelTypeLhs::template has_channel_names<channel_name::index>::value,"left hand pixel must not be indexed");
        static_assert(helpers::is_same<PixelTypeLhs,PixelTypeRhs>::value || !PixelTypeRhs::template has_channel_names<channel_name::index>::value,"right hand pixel must not be indexed");
        if(nullptr==result) return gfx_result::invalid_argument;
        if(helpers::is_same<PixelTypeLhs,PixelTypeRhs>::value) {
            if(nullptr==background) {
                result->native_value=source.native_value;
                return gfx_result::success;
            } else {
                return gfx_result::invalid_format;
            }
        }
        bool good = false;
        PixelTypeRhs tmp;
        typename PixelTypeRhs::int_type native_value = tmp.native_value;
        
        // here's where we gather color model information
        using is_rgb = typename PixelTypeLhs::template has_channel_names<channel_name::R,channel_name::G,channel_name::B>;
        using is_yuv = typename PixelTypeLhs::template has_channel_names<channel_name::Y,channel_name::U,channel_name::V>;
        using is_ycbcr = typename PixelTypeLhs::template has_channel_names<channel_name::Y,channel_name::Cb,channel_name::Cr>;
        using is_hsv = typename PixelTypeLhs::template has_channel_names<channel_name::H,channel_name::S,channel_name::V>;
        using is_hsl = typename PixelTypeLhs::template has_channel_names<channel_name::H,channel_name::S,channel_name::L>;
        using is_cmyk = typename PixelTypeLhs::template has_channel_names<channel_name::C,channel_name::M,channel_name::Y,channel_name::K>;
        using trhas_alpha = typename PixelTypeRhs::template has_channel_names<channel_name::A>;
        using thas_alpha = typename PixelTypeLhs::template has_channel_names<channel_name::A>;
        const bool has_alpha = thas_alpha::value;
        const bool is_bw_candidate = 1==PixelTypeLhs::channels || (2==PixelTypeLhs::channels && has_alpha);
        using tis_bw_candidate = typename PixelTypeLhs::template has_channel_names<channel_name::L>;
        const bool is_bw_candidate2 = tis_bw_candidate::value;
        const bool rhas_alpha = trhas_alpha::value;
        const bool ris_bw_candidate = 1==PixelTypeRhs::channels || (2==PixelTypeRhs::channels && rhas_alpha);
        using tris_bw_candidate = typename PixelTypeRhs::template has_channel_names<channel_name::L>;
        const bool ris_bw_candidate2 = tris_bw_candidate::value;
        using is_rhs_rgb = typename PixelTypeRhs::template has_channel_names<channel_name::R,channel_name::G,channel_name::B>;
        using is_rhs_yuv = typename PixelTypeRhs::template has_channel_names<channel_name::Y,channel_name::U,channel_name::V>;
        using is_rhs_ycbcr = typename PixelTypeRhs::template has_channel_names<channel_name::Y,channel_name::Cb,channel_name::Cr>;
        using is_rhs_hsv = typename PixelTypeRhs::template has_channel_names<channel_name::H,channel_name::S,channel_name::V>;
        using is_rhs_hsl = typename PixelTypeRhs::template has_channel_names<channel_name::H,channel_name::S,channel_name::L>;
        using is_rhs_cmyk = typename PixelTypeRhs::template has_channel_names<channel_name::C,channel_name::M,channel_name::Y,channel_name::K>;
        //using is_rhs_ycbcr = typename PixelTypeRhs::template has_channel_names<channel_name::Y,channel_name::Cb,channel_name::Cr>;
        // TODO: Add code for determining other additional color models here

        // check the source color model
        if(is_rgb::value && PixelTypeLhs::channels<5) {
            // source color model is RGB
            using tindexR = typename PixelTypeLhs::template channel_index_by_name<channel_name::R>;
            using tchR = typename PixelTypeLhs::template channel_by_index_unchecked<tindexR::value>;
            const int chiR = tindexR::value;
                
            using tindexG = typename PixelTypeLhs::template channel_index_by_name<channel_name::G>;
            using tchG = typename PixelTypeLhs::template channel_by_index_unchecked<tindexG::value>;
            const int chiG = tindexG::value;

            using tindexB = typename PixelTypeLhs::template channel_index_by_name<channel_name::B>;
            using tchB = typename PixelTypeLhs::template channel_by_index_unchecked<tindexB::value>;
            const int chiB = tindexB::value;
            
            if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {      
                // destination color model is RGB
                using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                
                using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
            
                using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                
                auto chR = source.template channel_unchecked<chiR>();
                auto cR = helpers::convert_channel_depth<tchR,trchR>(chR);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,cR);

                auto chG = source.template channel_unchecked<chiG>();
                auto cG = helpers::convert_channel_depth<tchG,trchG>(chG);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,cG);

                auto chB = source.template channel_unchecked<chiB>();
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
                
                const auto cR = (source.template channel_unchecked<chiR>()*tchR::scaler)*255.0;
                const auto cG = (source.template channel_unchecked<chiG>()*tchG::scaler)*255.0;
                const auto cB = (source.template channel_unchecked<chiB>()*tchB::scaler)*255.0;
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
            } else if(is_rhs_ycbcr::value && PixelTypeRhs::channels<5) {
                    // destination is YCbCr color model
                    using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                    using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
                    
                    using trindexCb = typename PixelTypeRhs::template channel_index_by_name<channel_name::Cb>;
                    using trchCb = typename PixelTypeRhs::template channel_by_index_unchecked<trindexCb::value>;
                
                    using trindexCr = typename PixelTypeRhs::template channel_index_by_name<channel_name::Cr>;
                    using trchCr = typename PixelTypeRhs::template channel_by_index_unchecked<trindexCr::value>;
                    const auto cR = source.template channelr_unchecked<chiR>();
                    const auto cG = source.template channelr_unchecked<chiG>();
                    const auto cB = source.template channelr_unchecked<chiB>();
                    // for BT.601 spec:
                    const double a = .299,
                                b=.587,
                                c=.114,
                                d=1.772,
                                e=1.402;
                    
                    const double y  = a * cR + b * cG + c * cB;
                    const double cb = (cB - y) / d+.5;
                    const double cr = (cR - y) / e+.5;
                    
                    const typename trchY::int_type cY =helpers::clamp(typename trchY::int_type(y*trchY::scale+.5),trchY::min,trchY::max);
                    const typename trchCb::int_type cCb = helpers::clamp(typename trchCb::int_type((cb*trchCb::scale+.5)),trchCb::min,trchCb::max);
                    const typename trchCr::int_type cCr = helpers::clamp(typename trchCr::int_type((cr*trchCr::scale+.5)),trchCr::min,trchCr::max);
                    
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,cY);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexCb::value>(native_value,cCb);
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexCr::value>(native_value,cCr);
                    
                    good = true;
            } else if(is_rhs_hsv::value && PixelTypeRhs::channels<5) {
                // destination is HSV color model
                using trindexH = typename PixelTypeRhs::template channel_index_by_name<channel_name::H>;
                using trchH = typename PixelTypeRhs::template channel_by_index_unchecked<trindexH::value>;
                
                using trindexS = typename PixelTypeRhs::template channel_index_by_name<channel_name::S>;
                using trchS = typename PixelTypeRhs::template channel_by_index_unchecked<trindexS::value>;
            
                using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
                using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;

                const double cR = source.template channelr_unchecked<chiR>();
                const double cG = source.template channelr_unchecked<chiG>();
                const double cB = source.template channelr_unchecked<chiB>();

                double cmin = cG<cB?cG:cB;
                cmin = cR<cmin?cR:cmin;
                double cmax = cG>cB?cG:cB;
                cmax = cR>cmax?cR:cmax;
                double chroma = cmax-cmin;
                
                double v = cmax;

                double s = cmax == 0 ? 0 : chroma / cmax;
                double h = 0; // achromatic
                if(cmax != cmin){
                    if(cmax==cR) {
                        h=(cG-cB)/chroma+(cG<cB?6:0);
                    } else if(cmax==cG) {
                        h = (cG - cR) / chroma + 2;
                    } else { // if(cmax==cB)
                        h = (cR - cG) / chroma + 4; 
                    }
                    h /= 6.0;
                }
                const typename trchH::int_type cH =helpers::clamp(typename trchH::int_type(h*trchH::scale+.5),trchH::min,trchH::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexH::value>(native_value,cH);
                const typename trchS::int_type cS =helpers::clamp(typename trchS::int_type(s*trchS::scale+.5),trchS::min,trchS::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexS::value>(native_value,cS);
                const typename trchV::int_type cV =helpers::clamp(typename trchV::int_type(v*trchV::scale+.5),trchV::min,trchV::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexV::value>(native_value,cV);
                good = true;
            } else if(is_rhs_hsl::value && PixelTypeRhs::channels<5) {
                // destination is HSL color model
                using trindexH = typename PixelTypeRhs::template channel_index_by_name<channel_name::H>;
                using trchH = typename PixelTypeRhs::template channel_by_index_unchecked<trindexH::value>;
                
                using trindexS = typename PixelTypeRhs::template channel_index_by_name<channel_name::S>;
                using trchS = typename PixelTypeRhs::template channel_by_index_unchecked<trindexS::value>;
            
                using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;

                const double cR = source.template channelr_unchecked<chiR>();
                const double cG = source.template channelr_unchecked<chiG>();
                const double cB = source.template channelr_unchecked<chiB>();

                double cmin = cG<cB?cG:cB;
                cmin = cR<cmin?cR:cmin;
                double cmax = cG>cB?cG:cB;
                cmax = cR>cmax?cR:cmax;
                
                double h =0, s=0, l = (cmax + cmin) / 2.0;

                if(cmax != cmin){
                    double chroma = cmax - cmin;
                    s = l > 0.5 ? chroma / (2.0 - cmax - cmin) : chroma / (cmax + cmin);
                    if(cmax==cR) {
                        h = (cG - cB) / chroma + (cG < cB ? 6 : 0);
                    } else if(cmax==cG) {
                        h = (cB - cG) / chroma + 2.0;
                    } else { // if(cmax==cB)
                        h = (cR - cG) / chroma + 4.0; 
                    }
                    h /= 6.0;
                }
                const typename trchH::int_type cH =helpers::clamp(typename trchH::int_type(h*trchH::scale+.5),trchH::min,trchH::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexH::value>(native_value,cH);
                const typename trchS::int_type cS =helpers::clamp(typename trchS::int_type(s*trchS::scale+.5),trchS::min,trchS::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexS::value>(native_value,cS);
                const typename trchL::int_type cL =helpers::clamp(typename trchL::int_type(l*trchL::scale+.5),trchL::min,trchL::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,cL);
                good = true;
            } else if(is_rhs_cmyk::value && PixelTypeRhs::channels<6) {
                // destination is CMYK color model
                using trindexC = typename PixelTypeRhs::template channel_index_by_name<channel_name::C>;
                using trchC = typename PixelTypeRhs::template channel_by_index_unchecked<trindexC::value>;
                
                using trindexM = typename PixelTypeRhs::template channel_index_by_name<channel_name::M>;
                using trchM = typename PixelTypeRhs::template channel_by_index_unchecked<trindexM::value>;
            
                using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;

                using trindexK = typename PixelTypeRhs::template channel_index_by_name<channel_name::K>;
                using trchK = typename PixelTypeRhs::template channel_by_index_unchecked<trindexK::value>;

                const double cR = source.template channelr_unchecked<chiR>();
                const double cG = source.template channelr_unchecked<chiG>();
                const double cB = source.template channelr_unchecked<chiB>();
                double cmax = cR>cG?cR:cG;
                cmax = cmax>cB?cmax:cB;
                double k = helpers::clampcymk(1 - cmax);
                double c = helpers::clampcymk((1 - cR - k) / (1 - k));
                double m = helpers::clampcymk((1 - cG - k) / (1 - k));
                double y = helpers::clampcymk((1 - cB - k) / (1 - k));

                const typename trchC::int_type cC =helpers::clamp(typename trchC::int_type(c*trchC::scale+.5),trchC::min,trchC::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexC::value>(native_value,cC);
                const typename trchM::int_type cM =helpers::clamp(typename trchM::int_type(m*trchM::scale+.5),trchM::min,trchM::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexM::value>(native_value,cM);
                const typename trchY::int_type cY =helpers::clamp(typename trchY::int_type(y*trchY::scale+.5),trchY::min,trchY::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,cY);
                const typename trchK::int_type cK =helpers::clamp(typename trchK::int_type(k*trchK::scale+.5),trchK::min,trchK::max);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexK::value>(native_value,cK);
                good = true;
            }
            
            if(ris_bw_candidate && ris_bw_candidate2) {
                // destination is grayscale or monochrome
                using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
                auto cR = source.template channel_unchecked<chiR>();
                auto cG = source.template channel_unchecked<chiG>();
                auto cB = source.template channel_unchecked<chiB>();
                double f = (cR*tchR::scaler*.299) +
                        (cG*tchG::scaler*.587) +
                        (cB*tchB::scaler*.114);
                const size_t scale = trchL::scale;
                f=(f*(double)scale)+.5;
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,f);
            
                good = true;
            } // TODO: add destination color models
        } else if(is_bw_candidate && is_bw_candidate2) {
            // source is grayscale or monochrome
            using tindexL = typename PixelTypeLhs::template channel_index_by_name<channel_name::L>;
            using tchL = typename PixelTypeLhs::template channel_by_index_unchecked<tindexL::value>;
            const int chiL = tindexL::value;
            
            if(ris_bw_candidate && ris_bw_candidate2) {
                // destination color model is grayscale or monochrome
                using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
                using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
                
                typename trchL::int_type chL = helpers::convert_channel_depth<tchL,trchL>(source.template channel_unchecked<chiL>());
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
                
                const auto chL = source.template channel_unchecked<chiL>();
                
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
                const auto chY=source.template channel_unchecked<tindexL::value>();
                auto cY = helpers::convert_channel_depth<tchL,trchY>(chY);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,cY);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexU::value>(native_value,0);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexV::value>(native_value,0);

                good = true;
            }
        } else if(is_yuv::value && PixelTypeLhs::channels<5) {
            // source color model is Y'UV
            using tindexY = typename PixelTypeLhs::template channel_index_by_name<channel_name::Y>;
            using tchY = typename PixelTypeLhs::template channel_by_index_unchecked<tindexY::value>;
            const int chiY = tindexY::value;
                
            using tindexU = typename PixelTypeLhs::template channel_index_by_name<channel_name::U>;
            using tchU = typename PixelTypeLhs::template channel_by_index_unchecked<tindexU::value>;
            const int chiU = tindexU::value;

            using tindexV = typename PixelTypeLhs::template channel_index_by_name<channel_name::V>;
            using tchV = typename PixelTypeLhs::template channel_by_index_unchecked<tindexV::value>;
            const int chiV = tindexV::value;

            if(is_rhs_yuv::value && PixelTypeRhs::channels<5) {
                // destination color model is YUV
                using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
                using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
                
                using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
                using trchU = typename PixelTypeRhs::template channel_by_index_unchecked<trindexU::value>;
            
                using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
                using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;
            
                auto chY = helpers::convert_channel_depth<tchY,trchY>(source.template channel_unchecked<chiY>());
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexY::value>(native_value,chY);

                auto chU = helpers::convert_channel_depth<tchU,trchU>(source.template channel_unchecked<chiU>());
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexU::value>(native_value,chU);

                auto chV = helpers::convert_channel_depth<tchV,trchV>(source.template channel_unchecked<chiV>());
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
                
                double chY = (source.template channel_unchecked<chiY>()*tchY::scaler)*256.0-16;
                double chU = (source.template channel_unchecked<chiU>()*tchU::scaler)*256.0-128;
                double chV = (source.template channel_unchecked<chiV>()*tchV::scaler)*256.0-128;
                
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
                
                auto cY = source.template channel_unchecked<chiY>();
                auto chL = helpers::convert_channel_depth<tchY,trchL>(cY);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexL::value>(native_value,chL);
            
                good = true;
            }
        } else if(is_ycbcr::value && PixelTypeLhs::channels<5) {
            // source color model is YCbCr
            using tindexY = typename PixelTypeLhs::template channel_index_by_name<channel_name::Y>;
            //using tchY = typename PixelTypeLhs::template channel_by_index_unchecked<tindexY::value>;
            const int chiY = tindexY::value;
                
            using tindexCb = typename PixelTypeLhs::template channel_index_by_name<channel_name::Cb>;
            //using tchCb = typename PixelTypeLhs::template channel_by_index_unchecked<tindexCb::value>;
            const int chiCb = tindexCb::value;

            using tindexCr = typename PixelTypeLhs::template channel_index_by_name<channel_name::Cr>;
            //using tchCr = typename PixelTypeLhs::template channel_by_index_unchecked<tindexCr::value>;
            const int chiCr = tindexCr::value;

            if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {
                const int CVACC = (sizeof(int) > 2) ? 1024 : 128; /* Adaptive accuracy for both 16-/32-bit systems */
                // destination color model is RGB
                using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                
                using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
            
                using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                
                const auto chY = uint8_t(source.template channelr_unchecked<chiY>()*255);
                const auto chCb = uint8_t(source.template channelr_unchecked<chiCb>()*255);
                const auto chCr = uint8_t(source.template channelr_unchecked<chiCr>()*255);
                const int cBA = chCb-128;
                const int cRA = chCr-128;
                const auto cnR = (uint8_t)helpers::clamp((int)(chY + ((int)(1.402 * CVACC) * cRA) / (float)CVACC),0,255);
                const auto cnG = (uint8_t)helpers::clamp((int)(chY - ((int)(0.344 * CVACC) * cBA + (int)(0.714 * CVACC) * cRA) / (float)CVACC),0,255);
                const auto cnB = (uint8_t)helpers::clamp((int)(chY + ((int)(1.772 * CVACC) * cBA) / (float)CVACC),0,255);
                
                const auto r = typename trchR::int_type(cnR*(trchR::scale/255.0));
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,r);
                const auto g = typename trchG::int_type(cnG*(trchG::scale/255.0));
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,g);
                const auto b = typename trchB::int_type(cnB*(trchB::scale/255.0));
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,b);
                good = true;                    
            }

        } else if(is_hsv::value && PixelTypeLhs::channels<5) {
            using tindexH = typename PixelTypeLhs::template channel_index_by_name<channel_name::H>;
            const int chiH = tindexH::value;
            using tindexS = typename PixelTypeLhs::template channel_index_by_name<channel_name::S>;
            const int chiS = tindexS::value;
            using tindexV = typename PixelTypeLhs::template channel_index_by_name<channel_name::V>;
            const int chiV = tindexV::value;
            if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {
                // destination color model is RGB
                using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                
                using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
            
                using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                
                const auto chH = source.template channelr_unchecked<chiH>();
                const auto chS = source.template channelr_unchecked<chiS>();
                const auto chV = source.template channelr_unchecked<chiV>();

                int i = floor(chH * 6);
                double f = chH * 6 - i;
                double p = chV * (1 - chS);
                double q = chV * (1 - f * chS);
                double t = chV * (1 - (1 - f) * chS);
                double r=0,g=0,b=0;
                switch(i % 6){
                    case 0: r = chV; g = t; b = p; break;
                    case 1: r = q; g = chV, b = p; break;
                    case 2: r = p; g = chV; b = t; break;
                    case 3: r = p; g = q; b = chV; break;
                    case 4: r = t; g = p; b = chV; break;
                    case 5: r = chV; g = p; b = q; break;
                }
                const auto sr = typename trchR::int_type(r*trchR::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,sr);
                const auto sg = typename trchG::int_type(g*trchG::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,sg);
                const auto sb = typename trchB::int_type(b*trchB::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,sb);
                good = true;                    
            }
        } else if(is_hsl::value && PixelTypeLhs::channels<5) {
            using tindexH = typename PixelTypeLhs::template channel_index_by_name<channel_name::H>;
            const int chiH = tindexH::value;
            using tindexS = typename PixelTypeLhs::template channel_index_by_name<channel_name::S>;
            const int chiS = tindexS::value;
            using tindexL = typename PixelTypeLhs::template channel_index_by_name<channel_name::L>;
            const int chiL = tindexL::value;
            if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {
                // destination color model is RGB
                using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                
                using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
            
                using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                
                const auto chH = source.template channelr_unchecked<chiH>();
                const auto chS = source.template channelr_unchecked<chiS>();
                const auto chL = source.template channelr_unchecked<chiL>();
                double r=0,g=0,b=0;
                if(chS == 0){
                    r = g = b = chL; // achromatic
                } else {
                    double q = chL < 0.5 ? chL * (1 + chS) : chL + chS - chL * chS;
                    double p = 2 * chL - q;
                    r = helpers::hue2rgb(p, q, chH + 1.0/3.0);
                    g = helpers::hue2rgb(p, q, chH);
                    b = helpers::hue2rgb(p, q, chH - 1.0/3.0);
                }

                printf("r: %f, g: %f, b: %f\n",r,g,b);
                const auto sr = typename trchR::int_type(r*trchR::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,sr);
                const auto sg = typename trchG::int_type(g*trchG::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,sg);
                const auto sb = typename trchB::int_type(b*trchB::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,sb);
                good = true;                    
            }
        } else if(is_cmyk::value && PixelTypeLhs::channels<6) {
            using tindexC = typename PixelTypeLhs::template channel_index_by_name<channel_name::C>;
            const int chiC = tindexC::value;
            using tindexM = typename PixelTypeLhs::template channel_index_by_name<channel_name::M>;
            const int chiM = tindexM::value;
            using tindexY = typename PixelTypeLhs::template channel_index_by_name<channel_name::Y>;
            const int chiY = tindexY::value;
            using tindexK = typename PixelTypeLhs::template channel_index_by_name<channel_name::K>;
            const int chiK = tindexK::value;
            
            if(is_rhs_rgb::value && PixelTypeRhs::channels<5) {
                // destination color model is RGB
                using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
                using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;
                
                using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
                using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;
            
                using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
                using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
                
                const double chC = source.template channelr_unchecked<chiC>();
                const double chM = source.template channelr_unchecked<chiM>();
                const double chY = source.template channelr_unchecked<chiY>();
                const double chK = source.template channelr_unchecked<chiK>();
                
                double r = (1.0 - chC) * (1.0 - chK);
                double g = (1.0 - chM) * (1.0 - chK);
                double b = (1.0 - chY) * (1.0 - chK);
                const auto sr = typename trchR::int_type(r*trchR::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexR::value>(native_value,sr);
                const auto sg = typename trchG::int_type(g*trchG::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexG::value>(native_value,sg);
                const auto sb = typename trchB::int_type(b*trchB::scale);
                helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexB::value>(native_value,sb);
                good = true;                    
            }
        }
        // TODO: add more source color models
        if(good) {
            // now do the alpha channels
            if(has_alpha) {
                using tindexA = typename PixelTypeLhs::template channel_index_by_name<channel_name::A>;
                const int chiA = tindexA::value;
                using tchA = typename PixelTypeLhs::template channel_by_index_unchecked<chiA>;
                
                // we need to blend it
                if(nullptr!=background) {
                    // first set the result
                    result->native_value = native_value;
                    // now blend it
                    return result->blend(*background,source.template channel_unchecked<chiA>()*tchA::scaler,result);
                }
                if(rhas_alpha) {
                    using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                    const int chirA = trindexA::value;
                    using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<chirA>;
                    auto chA = helpers::convert_channel_depth<tchA,trchA>(source.template channel_unchecked<chiA>());
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexA::value>(native_value,chA);
                } 
            } else {
                if(rhas_alpha) {
                    using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                    using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<trindexA::value>;
                    helpers::set_channel_direct_unchecked<PixelTypeRhs,trindexA::value>(native_value,trchA::default_);
                }
            }
            // finally, set the result
            result->native_value = native_value;
            return gfx_result::success;
        }  else {
            // if we couldn't convert directly
            // do a chain conversion
            rgba_pixel<HTCW_MAX_WORD> tmp1;
            PixelTypeRhs tmp2;
            gfx_result rr = convert(source,&tmp1);
            if(gfx_result::success!=rr) {
                return rr;
            }
            rr = convert(tmp1,&tmp2,background);
            if(gfx_result::success!=rr) {
                return rr;
            }
            *result=tmp2;
            return gfx_result::success;
        
        
        }
        // TODO: Add any additional metachannel processing (like alpha above) here
        return gfx_result::not_supported;
    } 
    // converts a pixel to the destination pixel type. background is optional and is for alpha blending
    template<typename PixelTypeLhs,typename PixelTypeRhs>
    constexpr inline static PixelTypeRhs convert(PixelTypeLhs lhs, const PixelTypeRhs* background=nullptr) {
        PixelTypeRhs result;
        gfx_result r = convert(lhs,&result,background);
        if(gfx_result::success!=r) {
            result.native_value = 0;
        }
        return result;
    }    
}
#endif
