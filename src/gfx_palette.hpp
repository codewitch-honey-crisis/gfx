#ifndef HTCW_GFX_PALETTE_HPP
#define HTCW_GFX_PALETTE_HPP
#include "gfx_pixel.hpp"
namespace gfx {
        // represents a palette/CLUT for indexed pixels
    template<typename PixelType,typename MappedPixelType>
    struct palette final {
        static_assert(PixelType::template has_channel_names<channel_name::index>::value,"Pixel must be indexed");
        static_assert(!MappedPixelType::template has_channel_names<channel_name::index>::value,"Mapped pixel must not be indexed");
        using type = palette;
        using pixel_type = PixelType;
        using mapped_pixel_type = MappedPixelType;
    private:
        using tindex = typename pixel_type::template channel_index_by_name<channel_name::index>;
        using tch = typename pixel_type::template channel_by_index_unchecked<tindex::value>;
    public:
        constexpr static const bool writable = true;
        constexpr static const size_t size = tch::max-tch::min+1;
    private:
        mapped_pixel_type m_colors[size];
    public:
       palette() {}
        palette(const palette& rhs)  {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
        }
        palette& operator=(const palette& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
            return *this;
        }
        palette(palette&& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
        }
        palette& operator=(palette&& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
            return *this;
        }
        ~palette() {}
        // maps an indexed pixel to a color value
        gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            const size_t i = pixel.template channel_unchecked<tindex::value>()-tch::min;
            *mapped_pixel=m_colors[i];
            return gfx_result::success;
        }
        // sets an indexed pixel to a mapped color value
        gfx_result map(pixel_type pixel,mapped_pixel_type mapped_pixel) {
            const size_t i = pixel.template channel_unchecked<tindex::value>()-tch::min;
            m_colors[i]=mapped_pixel;
            return gfx_result::success;
        }
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) {
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            if(0==size) {
                return gfx_result::invalid_argument;
            }
            double least = m_colors[0].difference(mapped_pixel);
            if(0.0==least) {
                pixel->native_value = 0;
                return gfx_result::success;
            }
            int ii=0;
            for(int i = 1;i<size;++i) {
                double cmp = m_colors[i].difference(mapped_pixel);
                if(0.0==cmp) {
                    ii=i;
                    least = 0.0;
                    break;
                }
                if(cmp<least) {
                    least = cmp;
                    ii=i;
                }
            }
            pixel->native_value=ii;
            return gfx_result::success;
        }
    };
    // specialization for unindexed pixels
    template<typename PixelType>
    struct palette<PixelType,PixelType> final {
        static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"Pixel must not be indexed");
        using type = palette;
        using pixel_type = PixelType;
        using mapped_pixel_type = PixelType;
        palette() {}
        palette(const palette& rhs)=default;
        palette& operator=(const palette& rhs)=default;
        palette(palette&& rhs)=default;
        palette& operator=(palette&& rhs)=default;
        ~palette() {}
        constexpr static const bool writable = false;
        constexpr static const size_t size = 0;
        // maps an indexed pixel to a color value
        inline gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            *mapped_pixel=pixel;
            return gfx_result::success;
        }
        /*
        // sets an indexed pixel to a mapped color value
        inline gfx_result map(pixel_type pixel,mapped_pixel_type mapped_pixel) {
            return gfx_result::not_supported;
        }
        */
    };
    template<typename MappedPixelType> 
    struct ega_palette {
        static_assert(!MappedPixelType::template has_channel_names<channel_name::index>::value,"Mapped pixel must not be indexed");
    private:
        constexpr static rgb_pixel<24> index_to_mapped(int idx) {
            const uint8_t red   = 85 * (((idx >> 1) & 2) | (idx >> 5) & 1);
            const uint8_t green = 85 * (( idx       & 2) | (idx >> 4) & 1);
            const uint8_t blue  = 85 * (((idx << 1) & 2) | (idx >> 3) & 1);
            return rgb_pixel<24>(red,green,blue);    
        }
    public:
        using type = ega_palette;
        using pixel_type = pixel<channel_traits<channel_name::index,4>>;
        using mapped_pixel_type = MappedPixelType;
        constexpr static const bool writable = false;
        constexpr static const size_t size = 16;
        gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            const uint8_t idx = pixel.native_value;
            const uint8_t red   = 85 * (((idx >> 1) & 2) | (idx >> 5) & 1);
            const uint8_t green = 85 * (( idx       & 2) | (idx >> 4) & 1);
            const uint8_t blue  = 85 * (((idx << 1) & 2) | (idx >> 3) & 1);
            rgb_pixel<24> rgb(red,green,blue);
            if(!convert(rgb,mapped_pixel)) {
                return gfx_result::not_supported;
            }
            return gfx_result::success;
        }
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) {
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            double least = index_to_mapped(0).difference(mapped_pixel);
            if(0.0==least) {
                pixel->native_value = 0;
                return gfx_result::success;
            }
            int ii=0;
            for(int i = 1;i<size;++i) {
                double cmp = index_to_mapped(i).difference(mapped_pixel);
                if(0.0==cmp) {
                    ii=i;
                    least = 0.0;
                    break;
                }
                if(cmp<least) {
                    least = cmp;
                    ii=i;
                }
            }
            pixel->native_value=ii;
            return gfx_result::success;
        }
    };

}
#endif