#ifndef HTCW_GFX_PIXEL_HPP
#define HTCW_GFX_PIXEL_HPP
#include <math.h>

#include "gfx_core.hpp"
#include "gfx_math.hpp"
#define GFX_CHANNEL_NAME(x)                                        \
    struct x {                                                     \
        static inline constexpr const char* value() { return #x; } \
    };
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
    // for RGBW
    GFX_CHANNEL_NAME(W)
    // index (for indexed color)
    GFX_CHANNEL_NAME(index)
    // non-op
    GFX_CHANNEL_NAME(nop)
    // TODO: add more of these
};
// defines a channel for a pixel
template <
    // the channel name, like channel_name::R
    typename Name,
    // the bit depth
    size_t BitDepth,
    // the minimum value
    bits::uintx<bits::get_word_size(BitDepth)> Min = 0,
// the maximum value
#if HTCW_MAX_WORD >= 64
    bits::uintx<bits::get_word_size(BitDepth)> Max = helpers::is_same<Name, channel_name::nop>::value ? 0 : ((BitDepth == 64) ? 0xFFFFFFFFFFFFFFFF : ((1 << BitDepth) - 1)),
#else
    bits::uintx<bits::get_word_size(BitDepth)> Max = helpers::is_same<Name, channel_name::nop>::value ? 0 : ((BitDepth == 32) ? 0xFFFFFFFF : ((1 << BitDepth) - 1)),
#endif
    // the default value
    bits::uintx<bits::get_word_size(BitDepth)> Default = helpers::is_same<Name, channel_name::nop>::value ? 0 : Min,
    // the scale denominator
    bits::uintx<bits::get_word_size(BitDepth)> Scale = helpers::is_same<Name, channel_name::nop>::value ? 1 : Max,
    bool ColorChannel = !(helpers::is_same<channel_name::nop, Name>::value || helpers::is_same<channel_name::A, Name>::value)>
struct channel_traits {
    // this type
    using type = channel_traits<Name, BitDepth, Min, Max, Scale>;
    // the type that represents the name
    using name_type = Name;
    // the integer type of this channel
    using int_type = bits::uintx<bits::get_word_size(BitDepth)>;
    // the floating point type of this channel
    using real_type = bits::realx<16 <= BitDepth ? HTCW_MAX_WORD : 32>;
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
    constexpr static const real_type scaler = 1 / (real_type)Scale;
    // a mask of the int value
    constexpr static const int_type int_mask = ~int_type(0);
    // a mask of the channel value
    constexpr static const int_type mask = bits::mask<BitDepth>::right;  //=int_type(int_mask>>((sizeof(int_type)*8)-BitDepth));
    constexpr static const bool color_channel = ColorChannel;
    // constraints
    static_assert(BitDepth > 0, "Bit depth must be greater than 0");
    static_assert(BitDepth <= 64, "Bit depth must be less than or equal to 64");
    static_assert(Min <= Max, "Min must be less than or equal to Max");
    static_assert(Default >= Min, "Default must be greater than or equal to the minimum value");
    static_assert(Default <= Max, "Default must be less than or equal to the maximum value");
#if HTCW_MAX_WORD >= 64
    static_assert(Max <= ((BitDepth == 64) ? 0xFFFFFFFFFFFFFFFF : ((1 << BitDepth) - 1)), "Max is greater than the maximum allowable value");
#else
    static_assert(Max <= ((BitDepth == 32) ? 0xFFFFFFFF : ((1 << BitDepth) - 1)), "Max is greater than the maximum allowable value");
#endif
    static_assert(Scale > 0, "Scale must not be zero");
};

// specialization for empty channel
template <>
struct channel_traits<channel_name::nop, 0, 0, 0, 0> {
    using type = channel_traits<channel_name::nop, 0, 0, 0, 0>;
    using name_type = channel_name::nop;
    using int_type = uint8_t;
    using real_type = float;
    constexpr static const size_t bit_depth = 0;
    constexpr static const int_type min = 0;
    constexpr static const int_type max = 0;
    constexpr static const int_type default_ = 0;
    constexpr static const int_type scale = 0;
    constexpr static const float scaler = 0.0;
    constexpr static const int_type int_mask = 0;
    constexpr static const int_type mask = 0;
    constexpr static const bool color_channel = false;
};

// represents a channel's metadata
template <typename PixelType, typename ChannelTraits,
          int Index,
          size_t BitsToLeft>
struct pixel_channel final {
    // the declaring pixel's type
    using pixel_type = PixelType;
    // this type
    using type = pixel_channel<pixel_type, ChannelTraits, Index, BitsToLeft>;
    // the name type for the channel
    using name_type = typename ChannelTraits::name_type;
    // the integer type for the channel
    using int_type = typename ChannelTraits::int_type;
    // the floating point type for the channel
    using real_type = typename ChannelTraits::real_type;
    // the declaring pixel's integer type
    using pixel_int_type = typename PixelType::int_type;
    // the number of bits to the left of this channel
    constexpr static const size_t bits_to_left = ChannelTraits::bit_depth != 0 ? BitsToLeft : 0;
    // the total bits to the right of this channel including padding
#ifndef HTCW_GFX_NO_SWAP
    constexpr static const size_t total_bits_to_right = ChannelTraits::bit_depth != 0 ? sizeof(pixel_int_type) * 8 - BitsToLeft - ChannelTraits::bit_depth : 0;
#else
    constexpr static const size_t total_bits_to_right = ChannelTraits::bit_depth != 0 ? pixel_type::packed_size * 8 - BitsToLeft - ChannelTraits::bit_depth : 0;
#endif
    // the bits to the right of this channel excluding padding
    constexpr static const size_t bits_to_right = ChannelTraits::bit_depth != 0 ? total_bits_to_right - PixelType::pad_right_bits : 0;
    // the name of the channel
    constexpr static inline const char* name() { return ChannelTraits::name(); }
    // the bit depth of the channel
    constexpr static const size_t bit_depth = ChannelTraits::bit_depth;
    // indicates the pixel byte alignment or 0 if not byte aligned
    constexpr static const size_t byte_alignment = (size_t)(0 == (bit_depth % 8)) * (bit_depth / 8);
    // the mask of the channel's value
    constexpr static const int_type value_mask = ChannelTraits::mask;
    // the mask of the channel's value within the entire pixel's value
    constexpr static const pixel_int_type channel_mask = (value_mask > 0) ? pixel_int_type(pixel_int_type(value_mask) << total_bits_to_right) : pixel_int_type(0);
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
    // true if this channel is part of the color model
    constexpr static const bool color_channel = ChannelTraits::color_channel;
};
// various utility templates and methods
namespace helpers {

template <typename... ChannelTraits>
struct bit_depth;
template <typename T, typename... ChannelTraits>
struct bit_depth<T, ChannelTraits...> {
    static constexpr const size_t value = T::bit_depth + bit_depth<ChannelTraits...>::value;
};
template <>
struct bit_depth<> {
    static const size_t value = 0;
};

template <typename... ChannelTraits>
struct color_bit_depth;
template <typename T, typename... ChannelTraits>
struct color_bit_depth<T, ChannelTraits...> {
    static constexpr const size_t value = (T::color_channel ? T::bit_depth : 0) + color_bit_depth<ChannelTraits...>::value;
};
template <>
struct color_bit_depth<> {
    static const size_t value = 0;
};

template <typename... ChannelTraits>
struct color_channels_size;
template <typename T, typename... ChannelTraits>
struct color_channels_size<T, ChannelTraits...> {
    static constexpr const size_t value = ((int)T::color_channel) + color_channels_size<ChannelTraits...>::value;
};
template <>
struct color_channels_size<> {
    static const size_t value = 0;
};

template <typename PixelType, int Index, int Count, size_t BitsToLeft, typename... ChannelTraits>
struct channel_by_index_impl;
template <typename PixelType, int Index, int Count, size_t BitsToLeft, typename ChannelTrait, typename... ChannelTraits>
struct channel_by_index_impl<PixelType, Index, Count, BitsToLeft, ChannelTrait, ChannelTraits...> {
    using type = typename channel_by_index_impl<PixelType, Index - 1, Count + 1, BitsToLeft + ChannelTrait::bit_depth, ChannelTraits...>::type;
};
template <typename PixelType, int Count, size_t BitsToLeft, typename ChannelTrait, typename... ChannelTraits>
struct channel_by_index_impl<PixelType, 0, Count, BitsToLeft, ChannelTrait, ChannelTraits...> {
    using type = pixel_channel<PixelType, ChannelTrait, Count, BitsToLeft>;
};
template <typename PixelType, int Index, int Count, size_t BitsToLeft>
struct channel_by_index_impl<PixelType, Index, Count, BitsToLeft> {
    using type = void;
};

template <typename PixelType, int Index, int Count, size_t BitsToLeft, typename... ChannelTraits>
struct channel_by_index_unchecked_impl;
template <typename PixelType, int Index, int Count, size_t BitsToLeft, typename ChannelTrait, typename... ChannelTraits>
struct channel_by_index_unchecked_impl<PixelType, Index, Count, BitsToLeft, ChannelTrait, ChannelTraits...> {
    using type = typename channel_by_index_unchecked_impl<PixelType, Index - 1, Count + 1, BitsToLeft + ChannelTrait::bit_depth, ChannelTraits...>::type;
};
template <typename PixelType, int Count, size_t BitsToLeft, typename ChannelTrait, typename... ChannelTraits>
struct channel_by_index_unchecked_impl<PixelType, 0, Count, BitsToLeft, ChannelTrait, ChannelTraits...> {
    using type = pixel_channel<PixelType, ChannelTrait, Count, BitsToLeft>;
};
template <typename PixelType, int Index, int Count, size_t BitsToLeft>
struct channel_by_index_unchecked_impl<PixelType, Index, Count, BitsToLeft> {
    using type = pixel_channel<PixelType, channel_traits<channel_name::nop, 0, 0, 0, 0>, 0, 0>;
};

template <typename PixelType, int Count, typename... ChannelTraits>
struct pixel_init_impl;
template <typename PixelType, int Count, typename ChannelTrait, typename... ChannelTraits>
struct pixel_init_impl<PixelType, Count, ChannelTrait, ChannelTraits...> {
    using ch = typename PixelType::template channel_by_index<Count>;
    using next = pixel_init_impl<PixelType, Count + 1, ChannelTraits...>;
    constexpr static inline void init(PixelType& pixel) {
        if (ChannelTrait::bit_depth != 0) {
            pixel.native_value = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(ch::default_, ch::min, ch::max)) << ch::total_bits_to_right);
        }
        next::init(pixel);
    }
    constexpr static inline void init(PixelType& pixel, typename ChannelTrait::int_type value, typename ChannelTraits::int_type... values) {
        if (ChannelTrait::bit_depth != 0) {
            const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(value, ch::min, ch::max)) << ch::total_bits_to_right);
            pixel.native_value = typename PixelType::int_type((pixel.native_value & typename ch::pixel_type::int_type(~ch::channel_mask)) | shval);
        }
        next::init(pixel, values...);
    }
    constexpr static inline void initf(PixelType& pixel, typename ChannelTrait::real_type value, typename ChannelTraits::real_type... values) {
        if (ChannelTrait::bit_depth != 0) {
            typename ch::int_type ivalue = helpers::clamp<decltype(value)>(value, 0.0, 1.0) * ch::scale;
            const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(ivalue) << ch::total_bits_to_right);
            pixel.native_value = typename PixelType::int_type((pixel.native_value & typename ch::pixel_type::int_type(~ch::channel_mask)) | shval);
        }
        next::initf(pixel, values...);
    }
};
template <typename PixelType, int Count>
struct pixel_init_impl<PixelType, Count> {
    constexpr static inline void init(PixelType& pixel) {
    }
    constexpr static inline void initf(PixelType& pixel) {
    }
};
template <typename PixelType, bool HasAlpha>
struct pixel_get_alpha_255 {
    constexpr inline static uint8_t value(PixelType px) {
        return 255;
    }
};
template <typename PixelType>
struct pixel_get_alpha_255<PixelType, true> {
    constexpr inline static uint8_t value(PixelType px) {
        //constexpr static const 
        using ch = PixelType::template channel_by_index_unchecked<PixelType::channel_index_by_name<channel_name::A>::value>;
        return px.template channel_unchecked<PixelType::template channel_index_by_name<channel_name::A>::value>()*255/ch::scale;
    }
};
template <typename PixelType, bool HasAlpha>
struct pixel_get_alpha {
    constexpr inline static float valuer(PixelType px) {
        return 1.0f;
    }
};
template <typename PixelType>
struct pixel_get_alpha<PixelType, true> {
    constexpr inline static auto valuer(PixelType px) {
        return px.template channelr_unchecked<PixelType::template channel_index_by_name<channel_name::A>::value>();
    }
};
template <typename PixelType, bool HasAlpha>
struct pixel_set_alpha {
    using type = float;
    constexpr inline static void valuer(PixelType& px, type dummy) {
    }
};
template <typename PixelType>
struct pixel_set_alpha<PixelType, true> {
    using type = float;
    constexpr inline static void valuer(PixelType& px, type value) {
        px.template channelr_unchecked<PixelType::template channel_index_by_name<channel_name::A>::value>(value);
    }
};
template <typename PixelType, bool HasAlpha>
struct pixel_set_alpha_255 {
    using type = uint8_t;
    constexpr inline static void value(PixelType& px, type dummy) {
    }
};
template <typename PixelType>
struct pixel_set_alpha_255<PixelType, true> {
    using type = uint8_t;
    constexpr inline static void value(PixelType& px, type value) {
        using ch = PixelType::template channel_by_index_unchecked<PixelType::channel_index_by_name<channel_name::A>::value>;
        px.template channel_unchecked<PixelType::template channel_index_by_name<channel_name::A>::value>(value * ch::scale / 255);
    }
};
template <typename PixelType, int Count, typename... ChannelTraits>
struct pixel_diff_impl;
template <typename PixelType, int Count, typename ChannelTrait, typename... ChannelTraits>
struct pixel_diff_impl<PixelType, Count, ChannelTrait, ChannelTraits...> {
    using ch = typename PixelType::template channel_by_index<Count>;
    using next = pixel_diff_impl<PixelType, Count + 1, ChannelTraits...>;
    constexpr static inline double diff_sum(PixelType lhs, PixelType rhs) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return NAN;
        const double d = (lhs.template channelr<index>() - rhs.template channelr<index>());
        return d * d + next::diff_sum(lhs, rhs);
    }
    constexpr static inline bits::uintx<HTCW_MAX_WORD> diff_sum_fast(PixelType lhs, PixelType rhs) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return NAN;
        const auto d = (lhs.template channel<index>() - rhs.template channel<index>());
        return d * d + next::diff_sum_fast(lhs, rhs);
    }
};
template <typename PixelType, int Count>
struct pixel_diff_impl<PixelType, Count> {
    constexpr static inline double diff_sum(PixelType lhs, PixelType rhs) {
        return 0.0;
    }
    constexpr static inline bits::uintx<HTCW_MAX_WORD> diff_sum_fast(PixelType lhs, PixelType rhs) {
        return 0;
    }
};

template <typename PixelType, int Count, typename... ChannelTraits>
struct pixel_blend_impl;
template <typename PixelType, int Count, typename ChannelTrait, typename... ChannelTraits>
struct pixel_blend_impl<PixelType, Count, ChannelTrait, ChannelTraits...> {
    using ch = typename PixelType::template channel_by_index<Count>;
    using next = pixel_blend_impl<PixelType, Count + 1, ChannelTraits...>;
    constexpr static inline void blend_val(PixelType lhs, PixelType rhs, double amount, PixelType* out_pixel) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return;
        // const double l = lhs.template channelr<index>()*amount;
        // const double r = rhs.template channelr<index>()*(1.0-amount);
        // out_pixel->template channelr<index>(l+r);
        auto l = lhs.template channel<index>();
        l = l * amount + .5f;
        auto r = rhs.template channel<index>();
        r = r * (1.0f - amount) + .5f;
        out_pixel->template channel<index>(l + r);
        next::blend_val(lhs, rhs, amount, out_pixel);
    }
    constexpr static inline void blend_val255(PixelType lhs, PixelType rhs, uint8_t amount, PixelType* out_pixel) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return;
        bits::uintx<HTCW_MAX_WORD> l = lhs.template channel<index>();
        bits::uintx<HTCW_MAX_WORD> r = rhs.template channel<index>();
        out_pixel->template channel<index>((l * amount + r * (255 - amount)) / 255);
        next::blend_val255(lhs, rhs, amount, out_pixel);
    }
};
template <typename PixelType, int Count>
struct pixel_blend_impl<PixelType, Count> {
    constexpr static inline void blend_val(PixelType lhs, PixelType rhs, double amount, PixelType* out_pixel) {
    }
    constexpr static inline void blend_val255(PixelType lhs, PixelType rhs, uint8_t amount, PixelType* out_pixel) {
    }
};

template <typename PixelType, int Count, typename... ChannelTraits>
struct pixel_premultiply_impl;
template <typename PixelType, int Count, typename ChannelTrait, typename... ChannelTraits>
struct pixel_premultiply_impl<PixelType, Count, ChannelTrait, ChannelTraits...> {
    using ch = typename PixelType::template channel_by_index<Count>;
    using next = pixel_premultiply_impl<PixelType, Count + 1, ChannelTraits...>;
    constexpr static inline void premultiply_val(PixelType lhs, size_t amount, PixelType* out_pixel) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return;
        if (!ChannelTrait::color_channel) {
            const auto v = lhs.template channel<index>();
            out_pixel->template channel<index>(v);
        } else {
            unsigned long long l = lhs.template channel<index>();
            l = ((l * amount) / ChannelTrait::scale);
            out_pixel->template channel<index>(l);
        }
        next::premultiply_val(lhs, amount, out_pixel);
    }
};
template <typename PixelType, int Count>
struct pixel_premultiply_impl<PixelType, Count> {
    constexpr static inline void premultiply_val(PixelType lhs, size_t amount, PixelType* out_pixel) {
    }
};

template <typename PixelType, int Count, typename... ChannelTraits>
struct pixel_unpremultiply_impl;
template <typename PixelType, int Count, typename ChannelTrait, typename... ChannelTraits>
struct pixel_unpremultiply_impl<PixelType, Count, ChannelTrait, ChannelTraits...> {
    using ch = typename PixelType::template channel_by_index<Count>;
    using next = pixel_unpremultiply_impl<PixelType, Count + 1, ChannelTraits...>;
    constexpr static inline void unpremultiply_val(PixelType lhs, size_t amount, PixelType* out_pixel) {
        constexpr const size_t index = Count;
        if (ChannelTrait::bit_depth == 0) return;
        if (!ChannelTrait::color_channel) {
            const auto v = lhs.template channel<index>();
            out_pixel->template channel<index>(v);
        } else {
            unsigned long long l = lhs.template channel<index>();
            l = (l * ChannelTrait::scale) / amount;
            out_pixel->template channel<index>(l);
        }
        next::unpremultiply_val(lhs, amount, out_pixel);
    }
};
template <typename PixelType, int Count>
struct pixel_unpremultiply_impl<PixelType, Count> {
    constexpr static inline void unpremultiply_val(PixelType lhs, size_t amount, PixelType* out_pixel) {
    }
};

template <int Count, typename Name, typename... ChannelTraits>
struct channel_index_by_name_impl;
template <int Count, typename Name>
struct channel_index_by_name_impl<Count, Name> {
    static constexpr int value = -1;
};
template <int Count, typename Name, typename ChannelTrait, typename... ChannelTraits>
struct channel_index_by_name_impl<Count, Name, ChannelTrait, ChannelTraits...> {
    static constexpr int value = is_same<Name, typename ChannelTrait::name_type>::value ? Count : channel_index_by_name_impl<Count + 1, Name, ChannelTraits...>::value;
};

template <typename PixelType, typename... ChannelTraits>
struct is_subset_pixel_impl;
template <typename PixelType>
struct is_subset_pixel_impl<PixelType> {
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
    ChannelTraits...> {
   private:
    using result = typename PixelType::template channel_index_by_name<typename ChannelTrait::name_type>;
    using next = typename helpers::is_subset_pixel_impl<PixelType, ChannelTraits...>;

   public:
    static constexpr bool value = (-1 != result::value) &&
                                  next::value;
};

template <typename PixelTypeLhs, typename PixelTypeRhs>
class unordered_equals_pixel_impl {
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
struct is_equal_pixel_impl<PixelType, Index> {
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
    ChannelTraits...> {
   private:
    using result = typename PixelType::template channel_index_by_name<typename ChannelTrait::name_type>;
    using next = typename helpers::is_equal_pixel_impl<PixelType, Index + 1, ChannelTraits...>;

   public:
    static constexpr bool value = result::value == Index && next::value;
};
template <typename PixelTypeLhs, typename... ChannelTraits>
class equals_pixel_impl {
    using compare = typename helpers::is_equal_pixel_impl<PixelTypeLhs, 0, ChannelTraits...>;

   public:
    constexpr static bool value =
        PixelTypeLhs::channels == sizeof...(ChannelTraits) &&
        compare::value;
};

template <typename PixelType, typename... ChannelNames>
class has_channel_names_impl;
template <typename PixelType, typename ChannelName, typename... ChannelNames>
class has_channel_names_impl<PixelType, ChannelName, ChannelNames...> {
    using chidx = typename PixelType::template channel_index_by_name<ChannelName>;

   public:
    constexpr static const bool value = (-1 != chidx::value) &&
                                        has_channel_names_impl<PixelType, ChannelNames...>::value;
};
template <typename PixelType>
class has_channel_names_impl<PixelType> {
   public:
    constexpr static const bool value = true;
};

template <typename PixelType, typename... ChannelNames>
class is_color_model_inner_impl;
template <typename PixelType, typename ChannelName, typename... ChannelNames>
class is_color_model_inner_impl<PixelType, ChannelName, ChannelNames...> {
    using chidx = typename PixelType::template channel_index_by_name<ChannelName>;

   public:
    constexpr static const bool value = (-1 != chidx::value) &&
                                        PixelType::template channel_by_index_unchecked<chidx::value>::color_channel &&
                                        is_color_model_inner_impl<PixelType, ChannelNames...>::value;
};
template <typename PixelType>
class is_color_model_inner_impl<PixelType> {
   public:
    constexpr static const bool value = true;
};

template <typename PixelType, typename... ChannelNames>
class is_color_model_impl {
   public:
    constexpr static const bool value = sizeof...(ChannelNames) == PixelType::color_channels && is_color_model_inner_impl<PixelType, ChannelNames...>::value;
};

// ---- integer fixed-point color math (Q16: 1.0 == 65536) --------------------
// The color-model conversions in convert<>() work in a normalized Q16
// fixed-point space instead of floating point. Internal working precision is
// intentionally capped at 16 bits per channel: that is ample for every real
// display format (which top out around 16bpp), and the only case that loses
// sub-16-bit precision is a lone >16-bit grayscale channel, a non-display
// outlier. No transcendental or non-constexpr library calls are used, so these
// paths are usable in constant expressions on GCC/MSVC/Clang under C++17.
constexpr static const uint32_t fp16_one = 65536u;   // represents 1.0
constexpr static const uint32_t fp16_half = 32768u;  // represents 0.5
// raw channel integer value -> Q16 normalized [0..65536], 16-bit-capped
constexpr inline static uint32_t channel_to_fp16(uint64_t v, size_t bit_depth, uint64_t scale) {
    if (scale == 0) return 0;
    const int s = (bit_depth > 16) ? (int)(bit_depth - 16) : 0;
    uint64_t vv = v >> s;
    uint64_t sc = scale >> s;
    if (sc == 0) sc = 1;
    return (uint32_t)((vv * (uint64_t)fp16_one + (sc >> 1)) / sc);
}
// read channel Index of a pixel as Q16 normalized [0..65536]
template <typename PixelType, int Index>
constexpr inline static uint32_t channel_fp16(const PixelType& px) {
    using ch = typename PixelType::template channel_by_index_unchecked<Index>;
    return channel_to_fp16((uint64_t)px.template channel_unchecked<Index>(), (size_t)ch::bit_depth, (uint64_t)ch::scale);
}
// Q16 normalized [0..65536] -> destination channel value (rounded, clamped)
template <typename Channel>
constexpr inline static typename Channel::int_type fp16_to_channel(uint32_t n) {
    uint64_t dv = ((uint64_t)n * (uint64_t)Channel::scale + (uint64_t)fp16_half) >> 16;
    if (dv < (uint64_t)Channel::min) dv = (uint64_t)Channel::min;
    if (dv > (uint64_t)Channel::max) dv = (uint64_t)Channel::max;
    return (typename Channel::int_type)dv;
}
// unsigned Q16 multiply, inputs/result in the [0..65536] domain, round to nearest
constexpr inline static uint32_t fp16_mul(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b + (uint64_t)fp16_half) >> 16);
}
// signed Q16 multiply, round to nearest (avoids shifting negatives)
constexpr inline static int64_t fp16_smul(int64_t a, int64_t b) {
    int64_t p = a * b;
    return (p >= 0) ? ((p + (int64_t)fp16_half) >> 16) : -(((-p) + (int64_t)fp16_half) >> 16);
}
// clamp a signed Q16 value into [0..65536]
constexpr inline static uint32_t fp16_clampu(int64_t v) {
    return (uint32_t)(v < 0 ? 0 : (v > (int64_t)fp16_one ? (int64_t)fp16_one : v));
}
// integer HSL hue->channel helper (Q16). p,q,t are Q16; result clamped to [0..65536]
constexpr inline static uint32_t fp16_hue2rgb(int64_t p, int64_t q, int64_t t) {
    if (t < 0) t += (int64_t)fp16_one;
    if (t > (int64_t)fp16_one) t -= (int64_t)fp16_one;
    const int64_t s6 = (int64_t)fp16_one / 6;
    const int64_t h2 = (int64_t)fp16_one / 2;
    const int64_t t3 = (2 * (int64_t)fp16_one) / 3;
    if (t < s6) return fp16_clampu(p + fp16_smul(q - p, 6 * t));
    if (t < h2) return fp16_clampu(q);
    if (t < t3) return fp16_clampu(p + fp16_smul(q - p, (t3 - t) * 6));
    return fp16_clampu(p);
}

// converts one channel's bit depth to another (integer rational rescale)
template <typename ChannelLhs, typename ChannelRhs>
constexpr inline static typename ChannelRhs::int_type convert_channel_depth(typename ChannelLhs::int_type v) {
    if (ChannelLhs::bit_depth == 0 || ChannelRhs::bit_depth == 0) return 0;
    if (ChannelLhs::bit_depth == ChannelRhs::bit_depth) return (typename ChannelRhs::int_type)v;
    const uint32_t n = channel_to_fp16((uint64_t)v, (size_t)ChannelLhs::bit_depth, (uint64_t)ChannelLhs::scale);
    return fp16_to_channel<ChannelRhs>(n);
}
// gets the native_value of a channel without doing compile time checking on the index
template <typename PixelType, int Index>
constexpr inline typename PixelType::template channel_by_index_unchecked<Index>::int_type get_channel_direct_unchecked(const typename PixelType::int_type& pixel_value) {
    using ch = typename PixelType::template channel_by_index_unchecked<Index>;
    if (0 > Index || Index >= (int)PixelType::channels) return 0;
    const typename PixelType::int_type p = pixel_value >> ch::total_bits_to_right;
    const typename ch::int_type result = typename ch::int_type(typename PixelType::int_type(p & typename PixelType::int_type(ch::value_mask)));
    return result;
}
// sets the native_value of a channel without doing compile time checking on the index
template <typename PixelType, int Index>
constexpr inline void

set_channel_direct_unchecked(typename PixelType::int_type& pixel_value, typename PixelType::template channel_by_index_unchecked<Index>::int_type value) {
    if (0 > Index || Index >= (int)PixelType::channels) return;
    using ch = typename PixelType::template channel_by_index_unchecked<Index>;
    const typename PixelType::int_type shval = typename PixelType::int_type(typename PixelType::int_type(helpers::clamp(value, ch::min, ch::max)) << ch::total_bits_to_right);
    pixel_value = typename PixelType::int_type((pixel_value & typename ch::pixel_type::int_type(~ch::channel_mask)) | shval);
}

}  // namespace helpers
// represents the pixel base class
template <typename... ChannelTraits>
struct pixel {
    // this type
    using type = pixel<ChannelTraits...>;
    // the type used for doing intermediary conversions to different formats when no explicit conversion is implemented
#if HTCW_MAX_WORD == 64
    using rgb_conversion_type = pixel<channel_traits<channel_name::R, 16>, channel_traits<channel_name::G, 16>, channel_traits<channel_name::B, 16>, channel_traits<channel_name::A, 16>>;
#else
    using rgb_conversion_type = pixel<channel_traits<channel_name::R, 8>, channel_traits<channel_name::G, 8>, channel_traits<channel_name::B, 8>, channel_traits<channel_name::A, 8>>;
#endif
    // the integer type of the pixel
    using int_type = bits::uintx<bits::get_word_size(helpers::bit_depth<ChannelTraits...>::value)>;
    // the number of channels
    constexpr static const size_t channels = sizeof...(ChannelTraits);
    // the number of color channels
    constexpr static const size_t color_channels = helpers::color_channels_size<ChannelTraits...>::value;
    // the total bit depth of the pixel
    constexpr static const size_t bit_depth = helpers::bit_depth<ChannelTraits...>::value;
    // indicates the pixel byte alignment or 0 if not byte aligned
    constexpr static const size_t byte_alignment = (size_t)(0 == (bit_depth % 8)) * (bit_depth / 8);
    // the bit depth of the color channels in the pixel
    constexpr static const size_t color_bit_depth = helpers::color_bit_depth<ChannelTraits...>::value;
    // indicates whether the pixel has an alpha channel
    constexpr static const bool has_alpha = helpers::channel_index_by_name_impl<sizeof...(ChannelTraits), channel_name::A, ChannelTraits...>::value >= 0;
    // the minimum number of bytes needed to store the pixel
    constexpr static const size_t packed_size = (bit_depth + 7) / 8;
    // true if the pixel is a whole number of bytes
    constexpr static const bool byte_aligned = 0 != byte_alignment;
    // the total size in bits, including padding
    constexpr static const size_t total_size_bits = sizeof(int_type) * 8;
    // the packed size, in bits
    constexpr static const size_t packed_size_bits = packed_size * 8;
    // the count of bits to the right that are unused
#ifndef HTCW_GFX_NO_SWAP
    constexpr static const size_t pad_right_bits = total_size_bits - bit_depth;
#else
    static_assert(byte_aligned || bit_depth == 1 || bit_depth < 8, "Currently, HTCW_GFX_NO_SWAP requires byte aligned pixels or pixels less than a byte");
    constexpr static const size_t pad_right_bits = 0;
#endif
    // the mask of the pixel's value
    constexpr static const int_type mask = int_type(int_type(~int_type(0)) << (pad_right_bits));

    // the pixel value, in platform native format
    int_type native_value;

    // initializes the pixel
    constexpr inline pixel() : native_value(0) {
        helpers::pixel_init_impl<type, 0, ChannelTraits...>::init(*this);
    }
    constexpr inline pixel(int_type native_value, bool dummy) : native_value(native_value) {
    }
    // initializes the pixel with a set of channel values
    constexpr inline pixel(typename ChannelTraits::int_type... values) : native_value(0) {
        helpers::pixel_init_impl<type, 0, ChannelTraits...>::init(*this, values...);
    }
    // initializes the pixel with a set of floating point values between 0 and 1.0
    constexpr inline pixel(bool dummy, typename ChannelTraits::real_type... values) : native_value(0) {
        helpers::pixel_init_impl<type, 0, ChannelTraits...>::initf(*this, values...);
    }
    // gets the pixel value in reverse endian form
    constexpr inline int_type swapped() const {
        return bits::swap(native_value);
    }
    // sets the pixel value in big endian form
    constexpr inline void swapped(int_type value) {
        native_value = bits::swap(value);
    }
    constexpr inline bool operator==(pixel rhs) {
        return rhs.native_value == native_value;
    }
    constexpr inline bool operator!=(pixel rhs) {
        return rhs.native_value != native_value;
    }
    constexpr inline operator int_type() {
        return native_value;
    }
    // retrieves a channel's metadata by index
    template <int Index>
    using channel_by_index = typename helpers::channel_by_index_impl<type, Index, channels, 0, ChannelTraits...>::type;
    // retrieves a channel's metadata by index in cases where the checked version will cause an error
    template <int Index>
    using channel_by_index_unchecked = typename helpers::channel_by_index_unchecked_impl<type, Index, channels, 0, ChannelTraits...>::type;
    // gets the index of the channel by the channel name
    template <typename Name>
    using channel_index_by_name = typename helpers::channel_index_by_name_impl<0, Name, ChannelTraits...>;
    // gets the channel by name
    template <typename Name>
    using channel_by_name = channel_by_index<helpers::channel_index_by_name_impl<0, Name, ChannelTraits...>::value>;
    // retrieves a channel's metadata by name in cases where the checked version will cause an error
    template <typename Name>
    using channel_by_name_unchecked = channel_by_index_unchecked<channel_index_by_name<Name>::value>;
    // returns true if the pixel contains channels with each name
    template <typename... ChannelNames>
    using has_channel_names = typename helpers::has_channel_names_impl<type, ChannelNames...>;
    // returns true if the pixel has the given color model (discounting non color channels like alpha, and nop)
    template <typename... ChannelNames>
    using is_color_model = typename helpers::is_color_model_impl<type, ChannelNames...>;
    // returns true if this channel is a subset of the other
    template <typename PixelRhs>
    using is_subset_of = typename helpers::is_subset_pixel_impl<PixelRhs, ChannelTraits...>;
    // returns true if this channel is a superset of the other
    template <typename PixelRhs>
    using is_superset_of = typename PixelRhs::template is_subset_of<type>;
    // returns true if the two pixels have channels with the same names, regardless of order
    template <typename PixelRhs>
    using unordered_equals = typename helpers::unordered_equals_pixel_impl<type, PixelRhs>;
    // returns true if the two pixels have channels with the same names, in the same order
    template <typename PixelRhs>
    using equals = typename helpers::equals_pixel_impl<PixelRhs, ChannelTraits...>;
    // returns true if the two pixels are exactly the same
    template <typename PixelRhs>
    using equals_exact = typename helpers::is_same<type, PixelRhs>;

    // retrieves the integer channel value without performing compile time checking on Index
    template <int Index>
    constexpr inline typename channel_by_index_unchecked<Index>::int_type channel_unchecked() const {
        return helpers::get_channel_direct_unchecked<type, Index>(native_value);
    }
    // sets the integer channel value without performing compile time checking on Index
    template <int Index>
    constexpr inline void channel_unchecked(typename channel_by_index_unchecked<Index>::int_type value) {
        helpers::set_channel_direct_unchecked<type, Index>(native_value, value);
    }
    // retrieves the integer channel value by index
    template <int Index>
    constexpr inline typename channel_by_index<Index>::int_type channel() const {
        using ch = channel_by_index<Index>;
        return typename ch::int_type(typename ch::pixel_type::int_type(native_value & ch::channel_mask) >> ch::total_bits_to_right);
    }
    // sets the integer channel value by index
    template <int Index>
    constexpr inline void channel(typename channel_by_index<Index>::int_type value) {
        using ch = channel_by_index<Index>;
        const typename ch::pixel_type::int_type shval = typename ch::pixel_type::int_type(typename ch::pixel_type::int_type(helpers::clamp(value, ch::min, ch::max)) << ch::total_bits_to_right);
        native_value = typename ch::pixel_type::int_type((native_value & typename ch::pixel_type::int_type(~ch::channel_mask)) | shval);
    }

    // retrieves the floating point channel value by index
    template <int Index>
    constexpr inline typename channel_by_index_unchecked<Index>::real_type channelr() const {
        using ch = channel_by_index<Index>;
        return channel<Index>() * ch::scaler;
    }
    // sets the floating point channel value by index
    template <int Index>
    constexpr inline void channelr(typename channel_by_index<Index>::real_type value) {
        using ch = channel_by_index<Index>;
        channel<Index>(value * ch::scale + .5);
    }
    // retrieves the floating point channel value by index
    template <int Index>
    constexpr inline typename channel_by_index_unchecked<Index>::real_type channelr_unchecked() const {
        using ch = channel_by_index_unchecked<Index>;
        return (typename ch::real_type)channel_unchecked<Index>() * ch::scaler;
    }
    // sets the floating point channel value by index
    template <int Index>
    constexpr inline void channelr_unchecked(typename channel_by_index<Index>::real_type value) {
        using ch = channel_by_index_unchecked<Index>;
        channel_unchecked<Index>(value * ch::scale + .5);
    }
    // retrieves the integer channel value by name
    template <typename Name>
    constexpr inline auto channel() const {
        constexpr const int index = channel_index_by_name<Name>::value;
        return channel<index>();
    }
    // sets the integer channel values by name
    template <typename Name>
    constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name>::value>::int_type value) {
        constexpr const int index = channel_index_by_name<Name>::value;
        channel<index>(value);
    }
    // sets the integer channel values by name
    template <typename Name1, typename Name2>
    constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name1>::value>::int_type value1,
                                  typename channel_by_index<channel_index_by_name<Name2>::value>::int_type value2) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channel<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channel<index2>(value2);
    }
    // sets the integer channel values by name
    template <typename Name1, typename Name2, typename Name3>
    constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name1>::value>::int_type value1,
                                  typename channel_by_index<channel_index_by_name<Name2>::value>::int_type value2,
                                  typename channel_by_index<channel_index_by_name<Name3>::value>::int_type value3) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channel<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channel<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channel<index3>(value3);
    }
    // sets the integer channel values by name
    template <typename Name1, typename Name2, typename Name3, typename Name4>
    constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name1>::value>::int_type value1,
                                  typename channel_by_index<channel_index_by_name<Name2>::value>::int_type value2,
                                  typename channel_by_index<channel_index_by_name<Name3>::value>::int_type value3,
                                  typename channel_by_index<channel_index_by_name<Name4>::value>::int_type value4) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channel<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channel<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channel<index3>(value3);
        constexpr const int index4 = channel_index_by_name<Name4>::value;
        channel<index4>(value4);
    }
    // sets the integer channel values by name
    template <typename Name1, typename Name2, typename Name3, typename Name4, typename Name5>
    constexpr inline void channel(typename channel_by_index<channel_index_by_name<Name1>::value>::int_type value1,
                                  typename channel_by_index<channel_index_by_name<Name2>::value>::int_type value2,
                                  typename channel_by_index<channel_index_by_name<Name3>::value>::int_type value3,
                                  typename channel_by_index<channel_index_by_name<Name4>::value>::int_type value4,
                                  typename channel_by_index<channel_index_by_name<Name5>::value>::int_type value5) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channel<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channel<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channel<index3>(value3);
        constexpr const int index4 = channel_index_by_name<Name4>::value;
        channel<index4>(value4);
        constexpr const int index5 = channel_index_by_name<Name5>::value;
        channel<index5>(value5);
    }

    // gets the floating point channel value by name
    template <typename Name>
    constexpr inline auto channelr() const {
        constexpr const int index = channel_index_by_name<Name>::value;
        return channelr<index>();
    }

    // sets the floating point channel value by name
    template <typename Name>
    constexpr inline void channelr(typename channel_by_name<Name>::real_type value) {
        constexpr const int index = channel_index_by_name<Name>::value;
        channelr<index>(value);
    }
    // sets the floating point channel values by name
    template <typename Name1, typename Name2>
    constexpr inline void channelr(typename channel_by_name<Name1>::real_type value1, typename channel_by_name<Name2>::real_type value2) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channelr<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channelr<index2>(value2);
    }
    // sets the floating point channel values by name
    template <typename Name1, typename Name2, typename Name3>
    constexpr inline void channelr(typename channel_by_name<Name1>::real_type value1,
                                   typename channel_by_name<Name2>::real_type value2,
                                   typename channel_by_name<Name3>::real_type value3) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channelr<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channelr<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channelr<index3>(value3);
    }
    // sets the floating point channel values by name
    template <typename Name1, typename Name2, typename Name3, typename Name4>
    constexpr inline void channelr(typename channel_by_name<Name1>::real_type value1,
                                   typename channel_by_name<Name2>::real_type value2,
                                   typename channel_by_name<Name3>::real_type value3,
                                   typename channel_by_name<Name4>::real_type value4) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channelr<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channelr<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channelr<index3>(value3);
        constexpr const int index4 = channel_index_by_name<Name4>::value;
        channelr<index4>(value4);
    }
    // sets the floating point channel values by name
    template <typename Name1, typename Name2, typename Name3, typename Name4, typename Name5>
    constexpr inline void channelr(typename channel_by_name<Name1>::real_type value1,
                                   typename channel_by_name<Name2>::real_type value2,
                                   typename channel_by_name<Name3>::real_type value3,
                                   typename channel_by_name<Name4>::real_type value4,
                                   typename channel_by_name<Name5>::real_type value5) {
        constexpr const int index1 = channel_index_by_name<Name1>::value;
        channelr<index1>(value1);
        constexpr const int index2 = channel_index_by_name<Name2>::value;
        channelr<index2>(value2);
        constexpr const int index3 = channel_index_by_name<Name3>::value;
        channelr<index3>(value3);
        constexpr const int index4 = channel_index_by_name<Name4>::value;
        channelr<index4>(value4);
        constexpr const int index5 = channel_index_by_name<Name5>::value;
        channelr<index4>(value5);
    }
    // returns the difference between two pixels
    constexpr double difference(type rhs) const {
        return sqrt(helpers::pixel_diff_impl<type, 0, ChannelTraits...>::diff_sum(*this, rhs));
    }
    constexpr bits::uintx<HTCW_MAX_WORD> difference_fast(type rhs) const {
        return helpers::pixel_diff_impl<type, 0, ChannelTraits...>::diff_sum_fast(*this, rhs);
    }
    // blends two pixels. ratio is between zero and one. larger ratio numbers favor this pixel
    constexpr gfx_result blend(const type rhs, double ratio, type* out_pixel) const {
        if (out_pixel == nullptr) {
            return gfx_result::invalid_argument;
        }
        static_assert(!has_channel_names<channel_name::index>::value, "pixel must not be indexed");
        if (ratio == 1.0f) {
            out_pixel->native_value = native_value;
            return gfx_result::success;
        } else if (ratio == 0.0f) {
            out_pixel->native_value = rhs.native_value;
            return gfx_result::success;
        }
        if (type::template has_channel_names<channel_name::A>::value) {
            constexpr const int ai = type::channel_index_by_name<channel_name::A>::value;

            auto a1 = this->template channelr_unchecked<ai>();
            auto a2 = rhs.template channelr_unchecked<ai>();
            auto r2 = a1 / a2;
            ratio = ratio * r2;
            if (ratio > 1.0f)
                ratio = 1.0f;
        }

        helpers::pixel_blend_impl<type, 0, ChannelTraits...>::blend_val(*this, rhs, ratio, out_pixel);
        return gfx_result::success;
    }
    // blends two pixels. ratio is between zero and 255. larger ratio numbers favor this pixel
    constexpr gfx_result blend8(const type rhs, uint8_t ratio, type* out_pixel) const {
        if (out_pixel == nullptr) {
            return gfx_result::invalid_argument;
        }
        static_assert(!has_channel_names<channel_name::index>::value, "pixel must not be indexed");
        if (ratio == 255) {
            out_pixel->native_value = native_value;
            return gfx_result::success;
        } else if (ratio == 0) {
            out_pixel->native_value = rhs.native_value;
            return gfx_result::success;
        }
        if (type::template has_channel_names<channel_name::A>::value) {
            constexpr const int ai = type::channel_index_by_name<channel_name::A>::value;
            // raw integer alpha; both are the same channel/scale, so the scale cancels in a1/a2
            const int a1 = (int)this->template channel_unchecked<ai>();
            const int a2 = (int)rhs.template channel_unchecked<ai>();
            int rr;
            if (a2 == 0) {
                rr = (a1 != 0) ? 255 : ratio;   // a1/a2 -> +inf clamps to full; 0/0 -> leave ratio be
            } else {
                rr = (int)ratio * a1 / a2;      // = ratio * (a1/a2); 254*max/1 still fits int
                if (rr > 255) rr = 255;
            }
            ratio = (uint8_t)rr;
        }
        helpers::pixel_blend_impl<type, 0, ChannelTraits...>::blend_val255(*this, rhs, ratio, out_pixel);
        return gfx_result::success;
    }
   
    // blends two pixels. ratio is between zero and one. larger ratio numbers favor this pixel
    constexpr type blend(const type rhs, double ratio) const {
        type result;
        blend(rhs, ratio, &result);
        return result;
    }
    // blends two pixels. ratio is between zero and 255. larger ratio numbers favor this pixel
    constexpr type blend8(const type rhs, uint8_t ratio) const {
        type result;
        blend8(rhs, ratio, &result);
        return result;
    }
    // premultiply pixels. amount is between zero and the channel scale.
    constexpr pixel& premultiply(size_t amount) {
        static_assert(!has_channel_names<channel_name::index>::value, "pixel must not be indexed");
        helpers::pixel_premultiply_impl<type, 0, ChannelTraits...>::premultiply_val(*this, amount, this);
        return *this;
    }
    // unpremultiply pixels. amount is between zero and the channel scale.
    constexpr pixel& unpremultiply(size_t amount) {
        static_assert(!has_channel_names<channel_name::index>::value, "pixel must not be indexed");
        helpers::pixel_unpremultiply_impl<type, 0, ChannelTraits...>::unpremultiply_val(*this, amount, this);
        return *this;
    }
    // indicates the opacity of the pixel
    constexpr auto opacity() const {
        return helpers::pixel_get_alpha<type, has_alpha>::valuer(*this);
    }
    // sets the opacity of the pixel (for those with an alpha channel)
    constexpr pixel& opacity_inplace(typename helpers::pixel_set_alpha<type, has_alpha>::type value) {
        helpers::pixel_set_alpha<type, has_alpha>::valuer(*this, value);
        return *this;
    }
    // sets the opacity of the pixel (for those with an alpha channel)
    constexpr pixel opacity(typename helpers::pixel_set_alpha<type, has_alpha>::type value) const {
        pixel px = *this;
        px.opacity_inplace(value);
        return px;
    }

    // indicates the opacity of the pixel
    constexpr uint8_t opacity8() const {
        return helpers::pixel_get_alpha_255<type, has_alpha>::value(*this);
    }
    // sets the opacity of the pixel (for those with an alpha channel)
    constexpr pixel& opacity8_inplace(uint8_t value) {
        helpers::pixel_set_alpha_255<type, has_alpha>::value(*this, value);
        return *this;
    }
    // sets the opacity of the pixel (for those with an alpha channel)
    constexpr pixel opacity8(uint8_t value) const {
        pixel px = *this;
        px.opacity8_inplace(value);
        return px;
    }
    static_assert(sizeof...(ChannelTraits) > 0, "A pixel must have at least one channel trait");
    static_assert(bit_depth <= HTCW_MAX_WORD, "Bit depth must be less than or equal to the maximum machine word size");
};

// creates an RGB pixel by making each channel
// one third of the whole. Any remainder bits
// are added to the green channel
template <size_t BitDepth>
using rgb_pixel = pixel<
    channel_traits<channel_name::R, (BitDepth / 3)>,
    channel_traits<channel_name::G, ((BitDepth / 3) + (BitDepth % 3))>,
    channel_traits<channel_name::B, (BitDepth / 3)>>;
// creates an RGBA pixel by making each channel
// one quarter of the whole. Any remainder bits
// are added to the green channel
template <size_t BitDepth>
using rgba_pixel = pixel<
    channel_traits<channel_name::R, (BitDepth / 4)>,
    channel_traits<channel_name::G, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::B, (BitDepth / 4)>,
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>>;

// creates an ARGB pixel by making each channel
// one quarter of the whole. Any remainder bits
// are added to the green channel
template <size_t BitDepth>
using argb_pixel = pixel<
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>,
    channel_traits<channel_name::R, (BitDepth / 4)>,
    channel_traits<channel_name::G, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::B, (BitDepth / 4)>>;
// creates a grayscale or monochome pixel
template <size_t BitDepth>
using gsc_pixel = pixel<
    channel_traits<channel_name::L, BitDepth>>;
// creates a Y'UV pixel by making each channel
// one third of the whole. Any remainder bits
// are added to the Y' channel
template <size_t BitDepth>
using yuv_pixel = pixel<
    channel_traits<channel_name::Y, ((BitDepth / 3) + (BitDepth % 3))>,
    channel_traits<channel_name::U, (BitDepth / 3)>,
    channel_traits<channel_name::V, (BitDepth / 3)>>;
// creates a Y'UV/A pixel by making each
// channel 1/4 of the whole. Remaining bits
// are added to Y'
template <size_t BitDepth>
using yuva_pixel = pixel<
    channel_traits<channel_name::Y, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::U, (BitDepth / 4)>,
    channel_traits<channel_name::V, (BitDepth / 4)>,
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>>;

// creates a YCbCr pixel by making each channel
// one third of the whole. Any remainder bits
// are added to the Y channel
template <size_t BitDepth>
using ycbcr_pixel = pixel<
    channel_traits<channel_name::Y, ((BitDepth / 3) + (BitDepth % 3))>,
    channel_traits<channel_name::Cb, (BitDepth / 3)>,
    channel_traits<channel_name::Cr, (BitDepth / 3)>>;
// creates a ycbcr pixel by making each
// channel 1/4 of the whole. Remaining bits
// are added to Y
template <size_t BitDepth>
using ycbcra_pixel = pixel<
    channel_traits<channel_name::Y, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::Cb, (BitDepth / 4)>,
    channel_traits<channel_name::Cr, (BitDepth / 4)>,
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>>;
// creates an indexed pixel
template <size_t BitDepth>
using indexed_pixel = pixel<channel_traits<channel_name::index, BitDepth>>;

// creates a HSV pixel by making each channel
// one third of the whole. Any remainder bits
// are added to the V channel
template <size_t BitDepth>
using hsv_pixel = pixel<
    channel_traits<channel_name::H, (BitDepth / 3)>,
    channel_traits<channel_name::S, (BitDepth / 3)>,
    channel_traits<channel_name::V, ((BitDepth / 3) + (BitDepth % 3))>>;
// creates a HSV/A pixel by making each
// channel 1/4 of the whole. Remaining bits
// are added to V
template <size_t BitDepth>
using hsva_pixel = pixel<
    channel_traits<channel_name::H, (BitDepth / 4)>,
    channel_traits<channel_name::S, (BitDepth / 4)>,
    channel_traits<channel_name::V, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>>;

// creates a HSL pixel by making each channel
// one third of the whole. Any remainder bits
// are added to the L channel
template <size_t BitDepth>
using hsl_pixel = pixel<
    channel_traits<channel_name::H, (BitDepth / 3)>,
    channel_traits<channel_name::S, (BitDepth / 3)>,
    channel_traits<channel_name::L, ((BitDepth / 3) + (BitDepth % 3))>>;
// creates a HSL/A pixel by making each
// channel 1/4 of the whole. Remaining bits
// are added to L
template <size_t BitDepth>
using hsla_pixel = pixel<
    channel_traits<channel_name::H, (BitDepth / 4)>,
    channel_traits<channel_name::S, (BitDepth / 4)>,
    channel_traits<channel_name::L, ((BitDepth / 4) + (BitDepth % 4))>,
    channel_traits<channel_name::A, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1>>;

// creates a CMYK pixel by making each channel
// one quarter of the whole. Any remainder bits
// are added to the K channel
template <size_t BitDepth>
using cmyk_pixel = pixel<
    channel_traits<channel_name::C, (BitDepth / 4)>,
    channel_traits<channel_name::M, (BitDepth / 4)>,
    channel_traits<channel_name::Y, (BitDepth / 4)>,
    channel_traits<channel_name::K, ((BitDepth / 4) + (BitDepth % 4))>>;
// creates a CMYK/A pixel by making each
// channel 1/5 of the whole. Remaining bits
// are added to K
template <size_t BitDepth>
using cmyka_pixel = pixel<
    channel_traits<channel_name::C, (BitDepth / 5)>,
    channel_traits<channel_name::M, (BitDepth / 5)>,
    channel_traits<channel_name::Y, (BitDepth / 5)>,
    channel_traits<channel_name::K, ((BitDepth / 5) + (BitDepth % 5))>,
    channel_traits<channel_name::A, (BitDepth / 5), 0, (1 << (BitDepth / 5)) - 1, (1 << (BitDepth / 5)) - 1>>;
// creates a RGBW pixel by making each channel
// one quarter of the whole. Any remainder bits
// are added to the G channel
template <size_t BitDepth>
using rgbw_pixel = pixel<
    channel_traits<channel_name::R, (BitDepth / 4)>,
    channel_traits<channel_name::G, (BitDepth / 4) + (BitDepth % 4)>,
    channel_traits<channel_name::B, (BitDepth / 4)>,
    channel_traits<channel_name::W, (BitDepth / 4)>>;
// creates a RGBW/A pixel by making each
// channel 1/5 of the whole. Remaining bits
// are added to G
template <size_t BitDepth>
using rgbwa_pixel = pixel<
    channel_traits<channel_name::R, (BitDepth / 5)>,
    channel_traits<channel_name::G, (BitDepth / 5) + (BitDepth % 5)>,
    channel_traits<channel_name::B, (BitDepth / 5)>,
    channel_traits<channel_name::W, (BitDepth / 5)>,
    channel_traits<channel_name::A, (BitDepth / 5), 0, (1 << (BitDepth / 5)) - 1, (1 << (BitDepth / 5)) - 1>>;
template <size_t BitDepth>
using alpha_pixel = pixel<
    channel_traits<
        channel_name::A,
        BitDepth,
        0,
#if HTCW_MAX_WORD >= 64
        ((BitDepth == 64) ? 0xFFFFFFFFFFFFFFFF : ((1 << BitDepth) - 1)),
#else
        ((BitDepth == 32) ? 0xFFFFFFFF : ((1 << BitDepth) - 1)),
#endif
#if HTCW_MAX_WORD >= 64
        ((BitDepth == 64) ? 0xFFFFFFFFFFFFFFFF : ((1 << BitDepth) - 1))
#else
        ((BitDepth == 32) ? 0xFFFFFFFF : ((1 << BitDepth) - 1))
#endif
        >>;

namespace helpers {
struct dither {
    //	16x16 Bayer Dithering Matrix.  Color levels: 256
    static const int* bayer_16;
};
// for HSL conversion
inline constexpr double hue2rgb(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}
inline constexpr double clampcymk(double value) {
    if ((value != value) || value < 0.0) {
        return 0.0;
    }
    return value;
}
}  // namespace helpers

// converts a pixel to the destination pixel type
template <typename PixelTypeLhs, typename PixelTypeRhs>
constexpr static inline gfx_result convert(PixelTypeLhs source, PixelTypeRhs* result, const PixelTypeRhs* background = nullptr) {
    static_assert(helpers::is_same<PixelTypeLhs, PixelTypeRhs>::value || !PixelTypeLhs::template has_channel_names<channel_name::index>::value, "left hand pixel must not be indexed");
    static_assert(helpers::is_same<PixelTypeLhs, PixelTypeRhs>::value || !PixelTypeRhs::template has_channel_names<channel_name::index>::value, "right hand pixel must not be indexed");
    if (nullptr == result) return gfx_result::invalid_argument;
    if (helpers::is_same<PixelTypeLhs, PixelTypeRhs>::value) {
        if (nullptr == background) {
            result->native_value = source.native_value;
            return gfx_result::success;
        } else {
            result->native_value = source.native_value;
            // now blend it
            return result->blend(*background, source.opacity(), result);
        }
    }
    bool good = false;
    PixelTypeRhs tmp;
    typename PixelTypeRhs::int_type native_value = tmp.native_value;

    // here's where we gather color model information
    using is_rgbw = typename PixelTypeLhs::template is_color_model<channel_name::R, channel_name::G, channel_name::B, channel_name::W>;
    using is_rgb = typename PixelTypeLhs::template is_color_model<channel_name::R, channel_name::G, channel_name::B>;
    using is_yuv = typename PixelTypeLhs::template is_color_model<channel_name::Y, channel_name::U, channel_name::V>;
    using is_ycbcr = typename PixelTypeLhs::template is_color_model<channel_name::Y, channel_name::Cb, channel_name::Cr>;
    using is_hsv = typename PixelTypeLhs::template is_color_model<channel_name::H, channel_name::S, channel_name::V>;
    using is_hsl = typename PixelTypeLhs::template is_color_model<channel_name::H, channel_name::S, channel_name::L>;
    using is_cmyk = typename PixelTypeLhs::template is_color_model<channel_name::C, channel_name::M, channel_name::Y, channel_name::K>;
    using trhas_alpha = typename PixelTypeRhs::template has_channel_names<channel_name::A>;
    using thas_alpha = typename PixelTypeLhs::template has_channel_names<channel_name::A>;
    const bool has_alpha = thas_alpha::value;
    const bool is_bw_candidate = 1 == PixelTypeLhs::color_channels;
    using tis_bw_candidate = typename PixelTypeLhs::template is_color_model<channel_name::L>;
    const bool is_bw_candidate2 = tis_bw_candidate::value;
    const bool rhas_alpha = trhas_alpha::value;
    const bool ris_bw_candidate = 1 == PixelTypeRhs::color_channels;
    using tris_bw_candidate = typename PixelTypeRhs::template is_color_model<channel_name::L>;
    const bool ris_bw_candidate2 = tris_bw_candidate::value;
    using is_rhs_rgbw = typename PixelTypeRhs::template is_color_model<channel_name::R, channel_name::G, channel_name::B, channel_name::W>;
    using is_rhs_rgb = typename PixelTypeRhs::template is_color_model<channel_name::R, channel_name::G, channel_name::B>;
    using is_rhs_yuv = typename PixelTypeRhs::template is_color_model<channel_name::Y, channel_name::U, channel_name::V>;
    using is_rhs_ycbcr = typename PixelTypeRhs::template is_color_model<channel_name::Y, channel_name::Cb, channel_name::Cr>;
    using is_rhs_hsv = typename PixelTypeRhs::template is_color_model<channel_name::H, channel_name::S, channel_name::V>;
    using is_rhs_hsl = typename PixelTypeRhs::template is_color_model<channel_name::H, channel_name::S, channel_name::L>;
    using is_rhs_cmyk = typename PixelTypeRhs::template is_color_model<channel_name::C, channel_name::M, channel_name::Y, channel_name::K>;
    // using is_rhs_ycbcr = typename PixelTypeRhs::template has_channel_names<channel_name::Y,channel_name::Cb,channel_name::Cr>;
    //  TODO: Add code for determining other additional color models here

    // check the source color model
    if (!is_rgbw::value && is_rgb::value) {
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

        if (!is_rhs_rgbw::value && is_rhs_rgb::value) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            auto chR = source.template channel_unchecked<chiR>();
            auto cR = helpers::convert_channel_depth<tchR, trchR>(chR);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, cR);

            auto chG = source.template channel_unchecked<chiG>();
            auto cG = helpers::convert_channel_depth<tchG, trchG>(chG);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, cG);

            auto chB = source.template channel_unchecked<chiB>();
            auto cB = helpers::convert_channel_depth<tchB, trchB>(chB);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, cB);

            good = true;
        } else if (is_rhs_rgbw::value) {
            // destination color model is RGBW

            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            using trindexW = typename PixelTypeRhs::template channel_index_by_name<channel_name::W>;
            using trchW = typename PixelTypeRhs::template channel_by_index_unchecked<trindexW::value>;

            const auto cR = source.template channelr_unchecked<chiR>();
            const auto cG = source.template channelr_unchecked<chiG>();
            const auto cB = source.template channelr_unchecked<chiB>();

            // convert to HSL
            auto cmin = cG < cB ? cG : cB;
            cmin = cR < cmin ? cR : cmin;
            auto cmax = cG > cB ? cG : cB;
            cmax = cR > cmax ? cR : cmax;

            double h = 0, s = 0, l = (cmax + cmin) / 2.0;

            if (cmax != cmin) {
                double chroma = cmax - cmin;
                s = l > 0.5 ? chroma / (2.0 - cmax - cmin) : chroma / (cmax + cmin);
                if (cmax == cR) {
                    h = (cG - cB) / chroma + (cG < cB ? 6 : 0);
                } else if (cmax == cG) {
                    h = (cB - cG) / chroma + 2.0;
                } else {  // if(cmax==cB)
                    h = (cR - cG) / chroma + 4.0;
                }
                h /= 6.0;
            }

            float H = h * 360.0f;
            H = 3.14159f * H / 180.0f;
            float S = s;
            float I = l;
            float r = 0, g = 0, b = 0;  //,w=0;
            if (H < 2.09439f) {
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                r = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                g = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                b = 0.0f;
                // w = (1.0f - S) * I;
            } else if (H < 4.188787f) {
                H = H - 2.09439f;
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                g = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                b = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                r = 0.0f;
                // w = (1.0f - S) * I;
            } else {
                H = H - 4.188787f;
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                b = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                r = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                g = 0.0f;
                // w = (1.0f - S) * I;
            }
            const auto sr = typename trchR::int_type(r * trchR::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, sr);
            const auto sg = typename trchG::int_type(g * trchG::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, sg);
            const auto sb = typename trchB::int_type(b * trchB::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, sb);
            const auto sw = typename trchW::int_type(b * trchW::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexW::value>(native_value, sw);
            good = true;

        } else if (is_rhs_yuv::value) {
            // destination is Y'UV color model
            using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
            using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;

            using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
            using trchU = typename PixelTypeRhs::template channel_by_index_unchecked<trindexU::value>;

            using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
            using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;

            // Q16 fixed-point (coefficients scaled by 65536); +offset = studio-swing bias
            const int64_t cR = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const int64_t cG = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const int64_t cB = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiB>(source);
            const int64_t chY = helpers::fp16_smul(cR, 16843) + helpers::fp16_smul(cG, 33030) + helpers::fp16_smul(cB, 6423) + 4112;
            const int64_t chU = helpers::fp16_smul(cR, -9699) + helpers::fp16_smul(cG, -19070) + helpers::fp16_smul(cB, 28770) + 32896;
            const int64_t chV = helpers::fp16_smul(cR, 28770) + helpers::fp16_smul(cG, -24117) + helpers::fp16_smul(cB, -4653) + 32896;
            const typename trchY::int_type cY = helpers::fp16_to_channel<trchY>(helpers::fp16_clampu(chY));
            const typename trchU::int_type cU = helpers::fp16_to_channel<trchU>(helpers::fp16_clampu(chU));
            const typename trchV::int_type cV = helpers::fp16_to_channel<trchV>(helpers::fp16_clampu(chV));

            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexY::value>(native_value, cY);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexU::value>(native_value, cU);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexV::value>(native_value, cV);

            good = true;
        } else if (is_rhs_ycbcr::value) {
            // destination is YCbCr color model
            using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
            using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;

            using trindexCb = typename PixelTypeRhs::template channel_index_by_name<channel_name::Cb>;
            using trchCb = typename PixelTypeRhs::template channel_by_index_unchecked<trindexCb::value>;

            using trindexCr = typename PixelTypeRhs::template channel_index_by_name<channel_name::Cr>;
            using trchCr = typename PixelTypeRhs::template channel_by_index_unchecked<trindexCr::value>;
            // BT.601 in Q16 fixed-point. Coeffs*65536: .299=19595 .587=38470 .114=7471
            // 1/1.772=36981, 1/1.402=46741 ; +0.5 bias = +32768
            const int64_t cR = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const int64_t cG = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const int64_t cB = (int64_t)helpers::channel_fp16<PixelTypeLhs, chiB>(source);
            const int64_t y = helpers::fp16_smul(cR, 19595) + helpers::fp16_smul(cG, 38470) + helpers::fp16_smul(cB, 7471);
            const int64_t cb = helpers::fp16_smul(cB - y, 36981) + (int64_t)helpers::fp16_half;
            const int64_t cr = helpers::fp16_smul(cR - y, 46741) + (int64_t)helpers::fp16_half;

            const typename trchY::int_type cY = helpers::fp16_to_channel<trchY>(helpers::fp16_clampu(y));
            const typename trchCb::int_type cCb = helpers::fp16_to_channel<trchCb>(helpers::fp16_clampu(cb));
            const typename trchCr::int_type cCr = helpers::fp16_to_channel<trchCr>(helpers::fp16_clampu(cr));

            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexY::value>(native_value, cY);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexCb::value>(native_value, cCb);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexCr::value>(native_value, cCr);

            good = true;
        } else if (is_rhs_hsv::value) {
            // destination is HSV color model
            using trindexH = typename PixelTypeRhs::template channel_index_by_name<channel_name::H>;
            using trchH = typename PixelTypeRhs::template channel_by_index_unchecked<trindexH::value>;

            using trindexS = typename PixelTypeRhs::template channel_index_by_name<channel_name::S>;
            using trchS = typename PixelTypeRhs::template channel_by_index_unchecked<trindexS::value>;

            using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
            using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;

            const uint32_t cR = helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const uint32_t cG = helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const uint32_t cB = helpers::channel_fp16<PixelTypeLhs, chiB>(source);

            uint32_t cmin = cG < cB ? cG : cB;
            cmin = cR < cmin ? cR : cmin;
            uint32_t cmax = cG > cB ? cG : cB;
            cmax = cR > cmax ? cR : cmax;
            const uint32_t chroma = cmax - cmin;

            const uint32_t v = cmax;
            const uint32_t s = cmax == 0 ? 0 : (uint32_t)(((uint64_t)chroma * (uint64_t)helpers::fp16_one + (cmax >> 1)) / cmax);
            uint32_t h = 0;  // achromatic
            if (cmax != cmin) {
                int64_t seg = 0, off = 0;
                if (cmax == cR) {
                    seg = (int64_t)cG - (int64_t)cB;
                    off = (cG < cB) ? 6 : 0;
                } else if (cmax == cG) {
                    seg = (int64_t)cB - (int64_t)cR;   // standard HSV: (B - R)/chroma + 2
                    off = 2;
                } else {  // if(cmax==cB)
                    seg = (int64_t)cR - (int64_t)cG;
                    off = 4;
                }
                const int64_t h6 = (seg * (int64_t)helpers::fp16_one) / (int64_t)chroma + off * (int64_t)helpers::fp16_one;
                h = (uint32_t)((h6 + 3) / 6);  // /6 with rounding; h6 >= 0 by construction
            }
            const typename trchH::int_type cH = helpers::fp16_to_channel<trchH>(h);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexH::value>(native_value, cH);
            const typename trchS::int_type cS = helpers::fp16_to_channel<trchS>(s);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexS::value>(native_value, cS);
            const typename trchV::int_type cV = helpers::fp16_to_channel<trchV>(v);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexV::value>(native_value, cV);
            good = true;
        } else if (is_rhs_hsl::value && PixelTypeRhs::channels < 5) {
            // destination is HSL color model
            using trindexH = typename PixelTypeRhs::template channel_index_by_name<channel_name::H>;
            using trchH = typename PixelTypeRhs::template channel_by_index_unchecked<trindexH::value>;

            using trindexS = typename PixelTypeRhs::template channel_index_by_name<channel_name::S>;
            using trchS = typename PixelTypeRhs::template channel_by_index_unchecked<trindexS::value>;

            using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
            using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;

            const uint32_t cR = helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const uint32_t cG = helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const uint32_t cB = helpers::channel_fp16<PixelTypeLhs, chiB>(source);

            uint32_t maxc = cG > cB ? cG : cB;
            maxc = cR > maxc ? cR : maxc;
            uint32_t minc = cG < cB ? cG : cB;
            minc = cR < minc ? cR : minc;
            const uint32_t c = maxc - minc;
            uint32_t hue = 0;
            if (c != 0) {
                int64_t seg = 0, off = 0;
                if (maxc == cR) {
                    seg = (int64_t)cG - (int64_t)cB;
                    off = (seg < 0) ? 6 : 0;  // hue > 180 -> full rotation
                } else if (maxc == cG) {
                    seg = (int64_t)cB - (int64_t)cR;
                    off = 2;  // 120 / 60
                } else {                      // max==cB
                    seg = (int64_t)cR - (int64_t)cG;
                    off = 4;  // 240 / 60
                }
                const int64_t h6 = (seg * (int64_t)helpers::fp16_one) / (int64_t)c + off * (int64_t)helpers::fp16_one;
                hue = (uint32_t)((h6 + 3) / 6);  // hue spans 0..6 in Q16, rescale to 0..1
            }

            const uint32_t l = (maxc + minc) >> 1;
            uint32_t s = 0;
            if (maxc != minc) {
                const uint64_t denom = (l > (uint32_t)helpers::fp16_half)
                                           ? (2ull * (uint64_t)helpers::fp16_one - maxc - minc)
                                           : ((uint64_t)maxc + minc);
                if (denom) s = (uint32_t)(((uint64_t)c * (uint64_t)helpers::fp16_one + (denom >> 1)) / denom);
            }
            const typename trchH::int_type cH = helpers::fp16_to_channel<trchH>(hue);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexH::value>(native_value, cH);
            const typename trchS::int_type cS = helpers::fp16_to_channel<trchS>(s);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexS::value>(native_value, cS);
            const typename trchL::int_type cL = helpers::fp16_to_channel<trchL>(l);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexL::value>(native_value, cL);
            good = true;
        } else if (is_rhs_cmyk::value) {
            // destination is CMYK color model
            using trindexC = typename PixelTypeRhs::template channel_index_by_name<channel_name::C>;
            using trchC = typename PixelTypeRhs::template channel_by_index_unchecked<trindexC::value>;

            using trindexM = typename PixelTypeRhs::template channel_index_by_name<channel_name::M>;
            using trchM = typename PixelTypeRhs::template channel_by_index_unchecked<trindexM::value>;

            using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
            using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;

            using trindexK = typename PixelTypeRhs::template channel_index_by_name<channel_name::K>;
            using trchK = typename PixelTypeRhs::template channel_by_index_unchecked<trindexK::value>;

            const uint32_t cR = helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const uint32_t cG = helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const uint32_t cB = helpers::channel_fp16<PixelTypeLhs, chiB>(source);
            uint32_t cmax = cR > cG ? cR : cG;
            cmax = cmax > cB ? cmax : cB;
            // k = 1 - cmax ; c = (cmax - r)/cmax ; m,y likewise (all >= 0, so clampcymk is a no-op)
            const uint32_t k = (uint32_t)helpers::fp16_one - cmax;
            uint32_t c = 0, m = 0, y = 0;
            if (cmax) {
                c = (uint32_t)(((uint64_t)(cmax - cR) * (uint64_t)helpers::fp16_one + (cmax >> 1)) / cmax);
                m = (uint32_t)(((uint64_t)(cmax - cG) * (uint64_t)helpers::fp16_one + (cmax >> 1)) / cmax);
                y = (uint32_t)(((uint64_t)(cmax - cB) * (uint64_t)helpers::fp16_one + (cmax >> 1)) / cmax);
            }

            const typename trchC::int_type cC = helpers::fp16_to_channel<trchC>(c);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexC::value>(native_value, cC);
            const typename trchM::int_type cM = helpers::fp16_to_channel<trchM>(m);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexM::value>(native_value, cM);
            const typename trchY::int_type cY = helpers::fp16_to_channel<trchY>(y);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexY::value>(native_value, cY);
            const typename trchK::int_type cK = helpers::fp16_to_channel<trchK>(k);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexK::value>(native_value, cK);
            good = true;
        }

        if (ris_bw_candidate && ris_bw_candidate2) {
            // destination is grayscale or monochrome
            using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
            using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;
            // luma: .299/.587/.114 as Q16 coefficients (19595 + 38470 + 7471 = 65536)
            const uint32_t cR = helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const uint32_t cG = helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const uint32_t cB = helpers::channel_fp16<PixelTypeLhs, chiB>(source);
            const uint32_t l = (uint32_t)(((uint64_t)cR * 19595 + (uint64_t)cG * 38470 + (uint64_t)cB * 7471) >> 16);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexL::value>(native_value, helpers::fp16_to_channel<trchL>(l));

            good = true;
        }  // TODO: add destination color models
    } else if (is_bw_candidate && is_bw_candidate2) {
        // source is grayscale or monochrome
        using tindexL = typename PixelTypeLhs::template channel_index_by_name<channel_name::L>;
        using tchL = typename PixelTypeLhs::template channel_by_index_unchecked<tindexL::value>;
        const int chiL = tindexL::value;

        if (ris_bw_candidate && ris_bw_candidate2) {
            // destination color model is grayscale or monochrome
            using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
            using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;

            typename trchL::int_type chL = helpers::convert_channel_depth<tchL, trchL>(source.template channel_unchecked<chiL>());
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexL::value>(native_value, chL);

            good = true;
        } else if (!is_rhs_rgbw::value && is_rhs_rgb::value) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            const auto chL = source.template channel_unchecked<chiL>();

            typename trchR::int_type chR = helpers::convert_channel_depth<tchL, trchR>(chL);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, chR);

            auto chG = helpers::convert_channel_depth<tchL, trchG>(chL);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, chG);

            auto chB = helpers::convert_channel_depth<tchL, trchB>(chL);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, chB);

            good = true;
        } else if (is_rhs_yuv::value) {
            using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
            using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;
            using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
            using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
            const auto chY = source.template channel_unchecked<tindexL::value>();
            auto cY = helpers::convert_channel_depth<tchL, trchY>(chY);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexY::value>(native_value, cY);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexU::value>(native_value, 0);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexV::value>(native_value, 0);

            good = true;
        }
    } else if (is_yuv::value) {
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

        if (is_rhs_yuv::value) {
            // destination color model is YUV
            using trindexY = typename PixelTypeRhs::template channel_index_by_name<channel_name::Y>;
            using trchY = typename PixelTypeRhs::template channel_by_index_unchecked<trindexY::value>;

            using trindexU = typename PixelTypeRhs::template channel_index_by_name<channel_name::U>;
            using trchU = typename PixelTypeRhs::template channel_by_index_unchecked<trindexU::value>;

            using trindexV = typename PixelTypeRhs::template channel_index_by_name<channel_name::V>;
            using trchV = typename PixelTypeRhs::template channel_by_index_unchecked<trindexV::value>;

            auto chY = helpers::convert_channel_depth<tchY, trchY>(source.template channel_unchecked<chiY>());
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexY::value>(native_value, chY);

            auto chU = helpers::convert_channel_depth<tchU, trchU>(source.template channel_unchecked<chiU>());
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexU::value>(native_value, chU);

            auto chV = helpers::convert_channel_depth<tchV, trchV>(source.template channel_unchecked<chiV>());
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexV::value>(native_value, chV);

            good = true;
        } else if (!is_rhs_rgbw::value && is_rhs_rgb::value) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            // Q16: *256 == <<8. chY/U/V carry the studio-swing bias in Q16 units.
            const int64_t chY = ((int64_t)helpers::channel_fp16<PixelTypeLhs, chiY>(source) << 8) - 16 * (int64_t)helpers::fp16_one;
            const int64_t chU = ((int64_t)helpers::channel_fp16<PixelTypeLhs, chiU>(source) << 8) - 128 * (int64_t)helpers::fp16_one;
            const int64_t chV = ((int64_t)helpers::channel_fp16<PixelTypeLhs, chiV>(source) << 8) - 128 * (int64_t)helpers::fp16_one;
            // coeffs*65536: 1.164=76284 1.596=104595 0.392=25690 0.813=53281 2.017=132187
            const int64_t cR = (helpers::fp16_smul(chY, 76284) + helpers::fp16_smul(chV, 104595)) / 255;
            const int64_t cG = (helpers::fp16_smul(chY, 76284) - helpers::fp16_smul(chU, 25690) - helpers::fp16_smul(chV, 53281)) / 255;
            const int64_t cB = (helpers::fp16_smul(chY, 76284) + helpers::fp16_smul(chU, 132187)) / 255;

            const auto r = helpers::fp16_to_channel<trchR>(helpers::fp16_clampu(cR));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, r);
            const auto g = helpers::fp16_to_channel<trchG>(helpers::fp16_clampu(cG));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, g);
            const auto b = helpers::fp16_to_channel<trchB>(helpers::fp16_clampu(cB));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, b);

            good = true;
        } else if (ris_bw_candidate && ris_bw_candidate2) {
            // destination is monochrome or grayscale
            using trindexL = typename PixelTypeRhs::template channel_index_by_name<channel_name::L>;
            using trchL = typename PixelTypeRhs::template channel_by_index_unchecked<trindexL::value>;

            auto cY = source.template channel_unchecked<chiY>();
            auto chL = helpers::convert_channel_depth<tchY, trchL>(cY);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexL::value>(native_value, chL);

            good = true;
        }
    } else if (is_ycbcr::value) {
        // source color model is YCbCr
        using tindexY = typename PixelTypeLhs::template channel_index_by_name<channel_name::Y>;
        // using tchY = typename PixelTypeLhs::template channel_by_index_unchecked<tindexY::value>;
        const int chiY = tindexY::value;

        using tindexCb = typename PixelTypeLhs::template channel_index_by_name<channel_name::Cb>;
        // using tchCb = typename PixelTypeLhs::template channel_by_index_unchecked<tindexCb::value>;
        const int chiCb = tindexCb::value;

        using tindexCr = typename PixelTypeLhs::template channel_index_by_name<channel_name::Cr>;
        // using tchCr = typename PixelTypeLhs::template channel_by_index_unchecked<tindexCr::value>;
        const int chiCr = tindexCr::value;

        if (!is_rhs_rgbw::value && is_rhs_rgb::value && PixelTypeRhs::channels < 5) {
            const int CVACC = (sizeof(int) > 2) ? 1024 : 128; /* Adaptive accuracy for both 16-/32-bit systems */
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            // read the three channels into 0..255 without floating point
            const int chY = (int)(((uint64_t)helpers::channel_fp16<PixelTypeLhs, chiY>(source) * 255u + (uint64_t)helpers::fp16_half) >> 16);
            const int chCb = (int)(((uint64_t)helpers::channel_fp16<PixelTypeLhs, chiCb>(source) * 255u + (uint64_t)helpers::fp16_half) >> 16);
            const int chCr = (int)(((uint64_t)helpers::channel_fp16<PixelTypeLhs, chiCr>(source) * 255u + (uint64_t)helpers::fp16_half) >> 16);
            const int cBA = chCb - 128;
            const int cRA = chCr - 128;
            // (int)(coef*CVACC) are compile-time integer constants (no runtime FP); divisor is now integer
            const int cnR = helpers::clamp((int)(chY + ((int)(1.402 * CVACC) * cRA) / CVACC), 0, 255);
            const int cnG = helpers::clamp((int)(chY - ((int)(0.344 * CVACC) * cBA + (int)(0.714 * CVACC) * cRA) / CVACC), 0, 255);
            const int cnB = helpers::clamp((int)(chY + ((int)(1.772 * CVACC) * cBA) / CVACC), 0, 255);

            const auto r = helpers::fp16_to_channel<trchR>(helpers::channel_to_fp16((uint64_t)cnR, 8, 255));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, r);
            const auto g = helpers::fp16_to_channel<trchG>(helpers::channel_to_fp16((uint64_t)cnG, 8, 255));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, g);
            const auto b = helpers::fp16_to_channel<trchB>(helpers::channel_to_fp16((uint64_t)cnB, 8, 255));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, b);
            good = true;
        }

    } else if (is_hsv::value) {
        using tindexH = typename PixelTypeLhs::template channel_index_by_name<channel_name::H>;
        const int chiH = tindexH::value;
        using tindexS = typename PixelTypeLhs::template channel_index_by_name<channel_name::S>;
        const int chiS = tindexS::value;
        using tindexV = typename PixelTypeLhs::template channel_index_by_name<channel_name::V>;
        const int chiV = tindexV::value;
        if (is_rhs_rgb::value && PixelTypeRhs::channels < 5) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            const uint32_t chH = helpers::channel_fp16<PixelTypeLhs, chiH>(source);
            const uint32_t chS = helpers::channel_fp16<PixelTypeLhs, chiS>(source);
            const uint32_t chV = helpers::channel_fp16<PixelTypeLhs, chiV>(source);

            // h*6 in Q16: integer sextant i and Q16 fraction f (floor is just the high bits)
            const uint64_t h6 = (uint64_t)chH * 6;
            const int i = (int)(h6 >> 16);
            const uint32_t f = (uint32_t)(h6 & 0xFFFFu);
            const uint32_t p = helpers::fp16_mul(chV, helpers::fp16_one - chS);
            const uint32_t q = helpers::fp16_mul(chV, helpers::fp16_one - helpers::fp16_mul(f, chS));
            const uint32_t t = helpers::fp16_mul(chV, helpers::fp16_one - helpers::fp16_mul(helpers::fp16_one - f, chS));
            uint32_t r = 0, g = 0, b = 0;
            switch (i % 6) {
                case 0:
                    r = chV;
                    g = t;
                    b = p;
                    break;
                case 1:
                    r = q;
                    g = chV, b = p;
                    break;
                case 2:
                    r = p;
                    g = chV;
                    b = t;
                    break;
                case 3:
                    r = p;
                    g = q;
                    b = chV;
                    break;
                case 4:
                    r = t;
                    g = p;
                    b = chV;
                    break;
                case 5:
                    r = chV;
                    g = p;
                    b = q;
                    break;
            }
            const auto sr = helpers::fp16_to_channel<trchR>(r);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, sr);
            const auto sg = helpers::fp16_to_channel<trchG>(g);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, sg);
            const auto sb = helpers::fp16_to_channel<trchB>(b);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, sb);
            good = true;
        }
    } else if (is_hsl::value) {
        using tindexH = typename PixelTypeLhs::template channel_index_by_name<channel_name::H>;
        const int chiH = tindexH::value;
        using tindexS = typename PixelTypeLhs::template channel_index_by_name<channel_name::S>;
        const int chiS = tindexS::value;
        using tindexL = typename PixelTypeLhs::template channel_index_by_name<channel_name::L>;
        const int chiL = tindexL::value;
        if (!is_rhs_rgbw::value && is_rhs_rgb::value && PixelTypeRhs::channels < 5) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            const uint32_t chH = helpers::channel_fp16<PixelTypeLhs, chiH>(source);
            const uint32_t chS = helpers::channel_fp16<PixelTypeLhs, chiS>(source);
            const uint32_t chL = helpers::channel_fp16<PixelTypeLhs, chiL>(source);
            uint32_t r = 0, g = 0, b = 0;
            if (chS == 0) {
                r = g = b = chL;  // achromatic
            } else {
                const int64_t q = (chL < helpers::fp16_half)
                                      ? (int64_t)helpers::fp16_mul(chL, helpers::fp16_one + chS)
                                      : ((int64_t)chL + chS - helpers::fp16_smul(chL, chS));
                const int64_t p = 2 * (int64_t)chL - q;
                const int64_t third = (int64_t)helpers::fp16_one / 3;  // 1/3 in Q16
                r = helpers::fp16_hue2rgb(p, q, (int64_t)chH + third);
                g = helpers::fp16_hue2rgb(p, q, (int64_t)chH);
                b = helpers::fp16_hue2rgb(p, q, (int64_t)chH - third);
            }

            const auto sr = helpers::fp16_to_channel<trchR>(r);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, sr);
            const auto sg = helpers::fp16_to_channel<trchG>(g);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, sg);
            const auto sb = helpers::fp16_to_channel<trchB>(b);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, sb);
            good = true;
        } else if (is_rhs_rgbw::value) {
            // destination color model is RGBW
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            using trindexW = typename PixelTypeRhs::template channel_index_by_name<channel_name::W>;
            using trchW = typename PixelTypeRhs::template channel_by_index_unchecked<trindexW::value>;

            const auto chH = source.template channelr_unchecked<chiH>();
            const auto chS = source.template channelr_unchecked<chiS>();
            const auto chL = source.template channelr_unchecked<chiL>();
            float H = chH * 360.0f;
            H = 3.14159f * H / 180.0f;
            float S = chS;
            float I = chL;
            float r = 0, g = 0, b = 0;  //,w=0;
            if (H < 2.09439f) {
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                r = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                g = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                b = 0.0f;
                // w = (1.0f - S) * I;
            } else if (H < 4.188787f) {
                H = H - 2.09439f;
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                g = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                b = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                r = 0.0f;
                // w = (1.0f - S) * I;
            } else {
                H = H - 4.188787f;
                float cos_h = cosf(H);
                float cos_1047_h = cosf(1.047196667f - H);
                b = S * I / 3.0f * (1.0f + cos_h / cos_1047_h);
                r = S * I / 3.0f * (1.0f + (1.0f - cos_h / cos_1047_h));
                g = 0.0f;
                // w = (1.0f - S) * I;
            }
            const auto sr = typename trchR::int_type(r * trchR::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, sr);
            const auto sg = typename trchG::int_type(g * trchG::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, sg);
            const auto sb = typename trchB::int_type(b * trchB::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, sb);
            const auto sw = typename trchW::int_type(b * trchW::scale);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexW::value>(native_value, sw);
            good = true;
        }
    } else if (is_cmyk::value) {
        using tindexC = typename PixelTypeLhs::template channel_index_by_name<channel_name::C>;
        const int chiC = tindexC::value;
        using tindexM = typename PixelTypeLhs::template channel_index_by_name<channel_name::M>;
        const int chiM = tindexM::value;
        using tindexY = typename PixelTypeLhs::template channel_index_by_name<channel_name::Y>;
        const int chiY = tindexY::value;
        using tindexK = typename PixelTypeLhs::template channel_index_by_name<channel_name::K>;
        const int chiK = tindexK::value;

        if (!is_rhs_rgbw::value && is_rhs_rgb::value) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;

            const uint32_t chC = helpers::channel_fp16<PixelTypeLhs, chiC>(source);
            const uint32_t chM = helpers::channel_fp16<PixelTypeLhs, chiM>(source);
            const uint32_t chY = helpers::channel_fp16<PixelTypeLhs, chiY>(source);
            const uint32_t chK = helpers::channel_fp16<PixelTypeLhs, chiK>(source);

            const uint32_t ik = helpers::fp16_one - chK;
            const uint32_t r = helpers::fp16_mul(helpers::fp16_one - chC, ik);
            const uint32_t g = helpers::fp16_mul(helpers::fp16_one - chM, ik);
            const uint32_t b = helpers::fp16_mul(helpers::fp16_one - chY, ik);
            const auto sr = helpers::fp16_to_channel<trchR>(r);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, sr);
            const auto sg = helpers::fp16_to_channel<trchG>(g);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, sg);
            const auto sb = helpers::fp16_to_channel<trchB>(b);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, sb);
            good = true;
        }
    } else if (is_rgbw::value) {
        // source color model is RGBW
        using tindexR = typename PixelTypeLhs::template channel_index_by_name<channel_name::R>;
        // using tchR = typename PixelTypeLhs::template channel_by_index_unchecked<tindexR::value>;
        const int chiR = tindexR::value;

        using tindexG = typename PixelTypeLhs::template channel_index_by_name<channel_name::G>;
        // using tchG = typename PixelTypeLhs::template channel_by_index_unchecked<tindexG::value>;
        const int chiG = tindexG::value;

        using tindexB = typename PixelTypeLhs::template channel_index_by_name<channel_name::B>;
        // using tchB = typename PixelTypeLhs::template channel_by_index_unchecked<tindexB::value>;
        const int chiB = tindexB::value;

        using tindexW = typename PixelTypeLhs::template channel_index_by_name<channel_name::W>;
        // using tchW = typename PixelTypeLhs::template channel_by_index_unchecked<tindexW::value>;
        const int chiW = tindexW::value;

        if (!is_rhs_rgbw::value && is_rhs_rgb::value) {
            // destination color model is RGB
            using trindexR = typename PixelTypeRhs::template channel_index_by_name<channel_name::R>;
            using trchR = typename PixelTypeRhs::template channel_by_index_unchecked<trindexR::value>;

            using trindexG = typename PixelTypeRhs::template channel_index_by_name<channel_name::G>;
            using trchG = typename PixelTypeRhs::template channel_by_index_unchecked<trindexG::value>;

            using trindexB = typename PixelTypeRhs::template channel_index_by_name<channel_name::B>;
            using trchB = typename PixelTypeRhs::template channel_by_index_unchecked<trindexB::value>;
            // white contributes 1/3 to each of R,G,B ; 1/3 in Q16 == 21845
            const uint32_t chR = helpers::channel_fp16<PixelTypeLhs, chiR>(source);
            const uint32_t chG = helpers::channel_fp16<PixelTypeLhs, chiG>(source);
            const uint32_t chB = helpers::channel_fp16<PixelTypeLhs, chiB>(source);
            const uint32_t chW = helpers::channel_fp16<PixelTypeLhs, chiW>(source);
            const uint32_t wAdd = helpers::fp16_mul(chW, 21845);
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexR::value>(native_value, helpers::fp16_to_channel<trchR>(helpers::fp16_clampu((int64_t)chR + wAdd)));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexG::value>(native_value, helpers::fp16_to_channel<trchG>(helpers::fp16_clampu((int64_t)chG + wAdd)));
            helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexB::value>(native_value, helpers::fp16_to_channel<trchB>(helpers::fp16_clampu((int64_t)chB + wAdd)));
            good = true;
        }
    }
    // TODO: add more source color models
    if (good) {
        // now do the alpha channels
        if (has_alpha) {
            using tindexA = typename PixelTypeLhs::template channel_index_by_name<channel_name::A>;
            const int chiA = tindexA::value;
            using tchA = typename PixelTypeLhs::template channel_by_index_unchecked<chiA>;

            // we need to blend it
            if (nullptr != background) {
                // first set the result
                result->native_value = native_value;
                // now blend it
                return result->blend(*background, source.template channel_unchecked<chiA>() * tchA::scaler, result);
            }
            if (rhas_alpha) {
                using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                const int chirA = trindexA::value;
                using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<chirA>;
                auto chA = helpers::convert_channel_depth<tchA, trchA>(source.template channel_unchecked<chiA>());
                helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexA::value>(native_value, chA);
            }
        } else {
            if (rhas_alpha) {
                using trindexA = typename PixelTypeRhs::template channel_index_by_name<channel_name::A>;
                using trchA = typename PixelTypeRhs::template channel_by_index_unchecked<trindexA::value>;
                helpers::set_channel_direct_unchecked<PixelTypeRhs, trindexA::value>(native_value, trchA::default_);
            }
        }
        // finally, set the result
        result->native_value = native_value;
        return gfx_result::success;
    } else {
        // if we couldn't convert directly
        // do a chain conversion
        rgba_pixel<HTCW_MAX_WORD> tmp1;
        PixelTypeRhs tmp2;
        gfx_result rr = convert(source, &tmp1);
        if (gfx_result::success != rr) {
            return rr;
        }
        rr = convert(tmp1, &tmp2, background);
        if (gfx_result::success != rr) {
            return rr;
        }
        *result = tmp2;
        return gfx_result::success;
    }
    // TODO: Add any additional metachannel processing (like alpha above) here
    return gfx_result::not_supported;
}
// converts a pixel to the destination pixel type. background is optional and is for alpha blending
template <typename PixelTypeLhs, typename PixelTypeRhs>
constexpr inline static PixelTypeRhs convert(PixelTypeLhs lhs, const PixelTypeRhs* background = nullptr) {
    PixelTypeRhs result;
    gfx_result r = convert(lhs, &result, background);
    if (gfx_result::success != r) {
        result.native_value = 0;
    }
    return result;
}
}  // namespace gfx
#endif