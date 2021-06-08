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
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
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
            //printf("nearest (native palette) was %d\r\n",ii);
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
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
            //printf("nearest\r\n");
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            pixel->native_value = mapped_pixel.native_value;
            return gfx_result::success;
        }
    };
    template<typename MappedPixelType> 
    struct ega_palette {
        static_assert(!MappedPixelType::template has_channel_names<channel_name::index>::value,"Mapped pixel must not be indexed");
    private:
        constexpr static gfx_result index_to_mapped(int idx,MappedPixelType* result) {
            const uint8_t red   = 85 * (((idx >> 1) & 2) | ((idx >> 5) & 1));
            const uint8_t green = 85 * (( idx       & 2) | ((idx >> 4) & 1));
            const uint8_t blue  = 85 * (((idx << 1) & 2) | ((idx >> 3) & 1));
            return convert(rgb_pixel<24>(red,green,blue),result);
        }
    public:
        using type = ega_palette;
        using pixel_type = indexed_pixel<4>;
        using mapped_pixel_type = MappedPixelType;
        constexpr static const bool writable = false;
        constexpr static const size_t size = 16;
        gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            return index_to_mapped(pixel.channel<channel_name::index>(),mapped_pixel);
        }
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
            
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            mapped_pixel_type mpx;
            gfx_result r = index_to_mapped(0,&mpx);
            if(gfx_result::success!=r) {
                return r;
            }
            double least = mpx.difference(mapped_pixel);
            if(0.0==least) {
                pixel->native_value = 0;
                return gfx_result::success;
            }
            int ii=0;
            for(int i = 1;i<size;++i) {
                r=index_to_mapped(i,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                double cmp = mpx.difference(mapped_pixel);
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
            pixel->channel<channel_name::index>(ii);
            //printf("nearest was %d\r\n",ii);
            return gfx_result::success;
        }
    };
    template<typename Destination,typename Source>
    inline static gfx_result convert_palette(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result, const typename Destination::pixel_type* background=nullptr);
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_to(const Target& target,typename Target::pixel_type pixel, PixelType* result, const PixelType* background=nullptr);
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_from(Target& target,PixelType pixel, typename Target::pixel_type* result, const typename Target::pixel_type* background=nullptr) ;
    namespace helpers {
        template<typename Target,typename PixelType, bool Indexed>
        struct palette_mapper_impl {
        };
        template<typename Target,typename PixelType>
        struct palette_mapper_impl<Target,PixelType,false> {
            inline static gfx_result pixel_to_indexed(const Target& target,PixelType pixel,const typename Target::pixel_type* background,  typename Target::pixel_type* indexed) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                typename Target::pixel_type px;
                gfx_result r = convert(pixel, &px,background);
                if(gfx_result::success!=r) {
                    return r;
                }
                indexed->native_value = px.native_value;
                return gfx_result::success;
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel,const PixelType* background, PixelType* mapped) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                PixelType px;
                gfx_result r = convert(pixel,&px,background);
                if(gfx_result::success!=r) {
                    return r;
                }
                mapped->native_value = px.native_value;
                return gfx_result::success;
            }
        };
        template<typename Target>
        struct palette_mapper_impl<Target,typename Target::pixel_type,true> {
            inline static gfx_result pixel_to_indexed(const Target& target,typename Target::pixel_type pixel,const typename Target::pixel_type* background, typename Target::pixel_type* indexed) { 
                if(nullptr==background) {
                    indexed->native_value = pixel.native_value;
                    return gfx_result::success;
                }
                typename Target::palette_type::mapped_pixel_type mpx;
                
                gfx_result r;
                //r= palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,pixel,nullptr,&mpx);
                r=convert_palette_to(target,pixel,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                typename Target::palette_type::mapped_pixel_type mbg;
                //r= palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,*background,nullptr,&mbg);
                r=convert_palette_to(target,*background,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert(mpx,&mpx,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                return palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,mpx,nullptr,indexed);
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel,const typename Target::pixel_type* background,typename Target::pixel_type* mapped) {
                return pixel_to_indexed(target,pixel,background,mapped);
            }
        };
        template<typename Target,typename PixelType>
        struct palette_mapper_impl<Target,PixelType,true> {
            inline static gfx_result pixel_to_indexed(const Target& target,PixelType pixel,const typename Target::pixel_type* background,typename Target::pixel_type* indexed) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                const typename Target::palette_type* pal=target.palette();
                if(nullptr==pal) {
                    return gfx_result::no_palette;
                }
                gfx_result r;
                typename Target::palette_type::mapped_pixel_type mpx;
                if(nullptr==background)  {
                    r = convert(pixel,&mpx);    
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    return pal->nearest(mpx,indexed);
                }
                typename Target::palette_type::mapped_pixel_type mbg;
                r=pal->map(*background,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert(pixel,&mpx,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                return pal->nearest(mpx,indexed);
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel, const PixelType* background, PixelType* mapped) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                const typename Target::palette_type* pal=target.palette();
                if(nullptr==pal) {
                    return gfx_result::no_palette;
                }
                typename Target::palette_type::mapped_pixel_type mpx;
                gfx_result r= pal->map(pixel,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return convert(mpx,mapped);
            }
        };
        template<typename Target,typename PixelType>
        struct palette_mapper {
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel, PixelType* mapped,const PixelType* background=nullptr) {
                using thas_index = typename Target::pixel_type::template has_channel_names<channel_name::index>;
                return helpers::palette_mapper_impl<Target,PixelType,thas_index::value>::indexed_to_pixel(target,pixel,background,mapped);
            }
            
            inline static gfx_result pixel_to_indexed(Target& target,PixelType pixel, typename Target::pixel_type* indexed,const typename Target::pixel_type* background=nullptr) {
                using thas_index = typename Target::pixel_type::template has_channel_names<channel_name::index>;
                return helpers::palette_mapper_impl<Target,PixelType,thas_index::value>::pixel_to_indexed(target,pixel,background,indexed);
            }
        };
        template<typename Destination,typename Source,bool IsDestinationIndexed,bool IsSourceIndexed>
        struct palette_converter {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                return gfx::convert(pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,true,false> {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                static_assert(!Source::pixel_type::template has_channel_names<channel_name::index>::value,"Source pixel type must not be indexed");
                return palette_mapper<Destination,typename Source::pixel_type>::pixel_to_indexed(destination,pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,false,true> {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                static_assert(!Destination::pixel_type::template has_channel_names<channel_name::index>::value,"Destination pixel type must not be indexed");
                return palette_mapper<Source,typename Destination::pixel_type>::indexed_to_pixel(source,pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,true,true> {
            static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                gfx_result r;
                rgb_pixel<HTCW_MAX_WORD> rgb;
                if(nullptr==background) {
                    //r=convert_palette_to(source,pixel,&rgb,nullptr);
                    r= palette_mapper<Source,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(source,pixel,&rgb,nullptr);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    return palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::pixel_to_indexed(destination,rgb,result,nullptr);
                }
                rgb_pixel<HTCW_MAX_WORD> bg;
                r= palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(destination,*background,&rgb,&bg);
                r= palette_mapper<Source,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(source,pixel,&rgb,nullptr);
                if(gfx_result::success!=r) {
                    return r;
                }
                return palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::pixel_to_indexed(destination,rgb,result,nullptr);
            }
        }; 
    }
    template<typename Destination,typename Source>
    inline static gfx_result convert_palette(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result, const typename Destination::pixel_type* background) {
        using tshas_index = typename Source::pixel_type::template has_channel_names<channel_name::index>;
        using tdhas_index = typename Destination::pixel_type::template has_channel_names<channel_name::index>;
        return helpers::palette_converter<Destination,Source,tdhas_index::value, tshas_index::value>::convert( destination,source,pixel,result,background);
    }
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_to(const Target& target,typename Target::pixel_type pixel, PixelType* result, const PixelType* background) {
        return helpers::palette_mapper<Target,PixelType>::indexed_to_pixel(target,pixel,result,background);
    }
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_from(Target& target,PixelType pixel, typename Target::pixel_type* result, const typename Target::pixel_type* background) {
        //using tshas_index = typename PixelType::template has_channel_names<channel_name::index>;
        //static_assert(!tshas_index::value,"PixelType must not be indexed");
        return helpers::palette_mapper<Target,PixelType>::pixel_to_indexed(target,pixel,result,background);
    }
}
#endif