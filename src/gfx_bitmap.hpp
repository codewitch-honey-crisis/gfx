#ifndef HTCW_GFX_BITMAP
#define HTCW_GFX_BITMAP
#include <string.h>
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"
#include "gfx_draw_common.hpp"
#include "gfx_palette.hpp"

namespace gfx {

    enum struct bitmap_resize {
        crop = 0,
        resize_fast = 1,
        resize_bilinear = 2,
        resize_bicubic = 3
    };
    
    class bitmap_base {
    protected:
        size16 m_dimensions;
        uint8_t* m_begin;
    public:
        inline bitmap_base(size16 dimensions, void* data) : m_dimensions(dimensions),m_begin((uint8_t*)data) {
        }
        inline bitmap_base() : m_dimensions(size16::zero()),m_begin(nullptr) {
        }
        inline size16 dimensions() const { return m_dimensions; }
        inline rect16 bounds() const { return m_dimensions.bounds(); }
        inline uint8_t* begin() const { return m_begin; }
        inline const uint8_t* cbegin() const { return m_begin; }
    };
    class const_bitmap_base {
    protected:
        size16 m_dimensions;
        const uint8_t* m_cbegin;
    public:
        inline const_bitmap_base(size16 dimensions, const void* data) : m_dimensions(dimensions),m_cbegin((const uint8_t*)data) {
        }
        inline size16 dimensions() const { return m_dimensions; }
        inline rect16 bounds() const { return m_dimensions.bounds(); }
        inline const uint8_t* cbegin() const { return m_cbegin; }
    };
    struct const_blt_span {
        virtual size16 span_dimensions() const=0;
        virtual size_t pixel_width() const = 0;
        virtual const gfx_cspan cspan(point16 location) const = 0;
    };
    struct blt_span : public const_blt_span {
        virtual gfx_span span(point16 location) const = 0;
    };
    namespace helpers {
        template<typename Source,typename Destination, bool AllowBlt=true>
        struct bmp_copy_to_helper {
            static inline gfx_result copy_to(const Source& src,const rect16& srcr,Destination& dst,const rect16& dstr) {
                size_t dy=0,dye=dstr.height();
                size_t dx,dxe = dstr.width();
                gfx_result r;
                // if(gfx_result::success!=r) {
                //     return r;
                // }
                int sox = srcr.left(),soy=srcr.top();
                int dox = dstr.left(),doy=dstr.top();
                while(dy<dye) {
                    dx=0;
                    
                    while(dx<dxe) {
                        typename Source::pixel_type spx;
                        r=src.point(point16(sox+dx,soy+dy),&spx);
                        if(gfx_result::success!=r)
                            return r;
                        typename Destination::pixel_type dpx;
                        if(Source::pixel_type::template has_channel_names<channel_name::A>::value) {
                            r=blend_helper<Source,Destination>::do_blend(src,spx,dst,point16(dox+dx,doy+dy),&dpx);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                        } else {    
                            r=convert_palette(dst,src,spx,&dpx,nullptr);
                            if(gfx_result::success!=r) {
                                return r;
                            }
                             
                        }
                        r=dst.point(point16(dox+dx,doy+dy),dpx);
                        if(gfx_result::success!=r)
                            return r;
                        ++dx;
                    }
                    ++dy;
                }
                return gfx_result::success;
                
            }
        };
        // blts a portion of this bitmap to the destination at the specified location.
        // out of bounds regions are cropped
        template<typename Source>
        struct bmp_copy_to_helper<Source,Source,true> {
            static gfx_result copy_to(const Source& src,const rect16& srcr,Source& dst,const rect16& dstr) {
                size_t dy=0,dye=dstr.height();
                size_t dxe = dstr.width();
                if(Source::pixel_type::byte_aligned) {
                const size_t line_len = dstr.width()*Source::pixel_type::packed_size;
                while(dy<dye) {
                    uint8_t* psrc = src.cbegin()+(((srcr.top()+dy)*src.dimensions().width+srcr.left())*Source::pixel_type::packed_size);
                    uint8_t* pdst = dst.begin()+(((dstr.top()+dy)*dst.dimensions().width+dstr.left())*Source::pixel_type::packed_size);
                    memcpy(pdst,psrc,line_len);
                    ++dy;
                }
                } else {
                    // unaligned version
                    const size_t line_len_bits = dstr.width()*Source::pixel_type::bit_depth;
                    const size_t line_block_pels = line_len_bits>(128*8)?(128*8)/Source::pixel_type::bit_depth:dstr.width();
                    const size_t line_block_bits = line_block_pels*Source::pixel_type::bit_depth;
                    const size_t buf_size = (line_block_bits+7)/8;
                    uint8_t buf[64];
                    buf[buf_size-1]=0;
                    while(dy<dye) {
                        size_t dx=0;
                        while(dx<dxe) {
                            const size_t src_offs = ((srcr.top()+dy)*src.dimensions().width+srcr.left()+dx)*Source::pixel_type::bit_depth;
                            const auto dpx = (dstr.top()+dy)*dst.dimensions().width+dstr.left()+dx;
                            const size_t dst_offs =dpx*Source::pixel_type::bit_depth;
                            const size_t src_offs_bits = src_offs % 8;
                            const size_t dst_offs_bits = dst_offs % 8;
                            uint8_t* psrc = src.cbegin()+(src_offs/8);
                            uint8_t* pdst = dst.begin()+(dst_offs/8);
                            const size_t block_pels = (line_block_pels+dx>dstr.width())?dstr.width()-dx:line_block_pels;
                            if(0==block_pels) 
                                break;
                            const size_t block_bytes = (block_pels*Source::pixel_type::bit_depth+7)/8;
                            const size_t block_bits = block_pels*Source::pixel_type::bit_depth;
                            const int shift = dst_offs_bits-src_offs_bits;
                            // TODO: Make this more efficient:
                            memcpy(buf,psrc,block_bytes+(dpx+block_pels<=dst.size_pixels()));
                            if(0<shift) {
                                bits::shift_right(buf,0,(sizeof(buf))*8,shift);
                            } else if(0>shift) {
                                bits::shift_left(buf,0,(sizeof(buf))*8,-shift);
                            }
                            bits::set_bits(dst_offs_bits,block_bits, pdst,buf);
                            dx+=block_pels;    
                        }
                        ++dy;
                    }
                }
                // TODO:
                // clear any bits we couldn't copy 
                // on account of being out of bounds?
                return gfx_result::success;    
            }
        };

        // represents an in-memory bitmap
        template<typename PixelType,typename PaletteType>
        class bitmap_impl_base : public bitmap_base {
        protected:
            const PaletteType* m_palette;
        public:
            // the type of the bitmap, itself
            using type = bitmap_impl_base;
            // the type of the pixel used for the bitmap
            using pixel_type = PixelType;
            using palette_type = PaletteType;
            using caps = gfx::gfx_caps< true,PixelType::byte_alignment!=0, false,true>;

        private:
            gfx_result point_impl(point16 location,pixel_type rhs) {
                if(nullptr==begin()) {
                    return gfx_result::out_of_memory;
                }
                constexpr static const size_t bit_depth = pixel_type::bit_depth;
                size_t offs = (location.y*dimensions().width+location.x);
                switch(bit_depth) {
                    case 8: {
                        uint8_t* p = ((uint8_t*)begin())+offs;
                        *p=rhs.native_value;
                        break;
                    }
                    case 16: {
                        uint16_t *p = ((uint16_t*)begin())+offs;
#ifndef HTCW_GFX_NO_SWAP
                        *p=rhs.swapped();    
#else
                        *p=rhs.native_value;    
#endif
                        break;
                    } case 32: {
                        uint32_t *p = ((uint32_t*)begin())+offs;
#ifndef HTCW_GFX_NO_SWAP
                        *p=rhs.swapped();    
#else
                        *p=rhs.native_value;    
#endif
                        break;
                    }
                    default: {
#ifndef HTCW_GFX_NO_SWAP
                        typename pixel_type::int_type v = rhs.swapped();
#else
                        typename pixel_type::int_type v = rhs.native_value;
#endif
                        offs *= bit_depth;
                        if(pixel_type::byte_aligned) {
                            memcpy(begin()+offs/8,&v,sizeof(typename pixel_type::int_type));                              
                            break;
                        }
                        const size_t offs_bits = offs % 8;
                        //const size_t siz = pixel_type::packed_size+(((int)pixel_type::pad_right_bits)<=offs_bits)+1;
                        // now set the pixel
                        uint8_t tmp[9];
                        memcpy(tmp,&v,sizeof(typename pixel_type::int_type));    
                        // below doesn't work with strict aliasing:
                        //*((typename pixel_type::int_type*)tmp)=rhs.swapped();
                        bits::shift_right(tmp,0,pixel_type::bit_depth+offs_bits,offs_bits);
                        bits::set_bits(offs_bits,pixel_type::bit_depth,begin()+offs/8,tmp);
                        break;
                    }
                }

                return gfx_result::success;
            }
        public:
            // constructs a new bitmap with the specified size and buffer
            bitmap_impl_base(size16 dimensions,void* buffer,const palette_type* palette=nullptr) : bitmap_base(dimensions,(uint8_t*)buffer),m_palette(palette) {}
            bitmap_impl_base() : bitmap_base(),m_palette(nullptr) {
                
            }
            bitmap_impl_base(const type& rhs)=default;
            type& operator=(const type& rhs)=default;
            bitmap_impl_base(type&& rhs)=default;
            type& operator=(type&& rhs) {
                m_dimensions = rhs.m_dimensions;
                m_palette = rhs.m_palette;
                m_begin = rhs.m_begin;
                return *this;
            }
            inline bool initialized() const {
                return nullptr!=m_begin;
            }
            gfx_result point(point16 location,pixel_type* out_pixel) const {
                if(nullptr==begin()) {
                    return gfx_result::out_of_memory;
                }
                if(nullptr==out_pixel)
                    return gfx_result::invalid_argument;
                if(location.x>=dimensions().width||location.y>=dimensions().height) {
                    *out_pixel = pixel_type();
                    return gfx_result::success;
                }
                constexpr static const size_t bit_depth = pixel_type::bit_depth;
                size_t offs = (location.y*dimensions().width+location.x);

                switch(bit_depth) {
                    case 8: {
                        const typename pixel_type::int_type *p = ((typename pixel_type::int_type*)begin())+offs;
                        out_pixel->native_value=*p;
                        break;
                    }
                    case 16: {
                        const typename pixel_type::int_type *p = ((typename pixel_type::int_type*)begin())+offs;
#ifndef HTCW_GFX_NO_SWAP
                        out_pixel->swapped(*p);
#else
                        out_pixel->native_value=*p;
#endif

                        break;
                    } case 32: {
                        const typename pixel_type::int_type *p = ((typename pixel_type::int_type*)begin())+offs;
#ifndef HTCW_GFX_NO_SWAP
                        out_pixel->swapped(*p);
#else
                        out_pixel->native_value=*p;
#endif
                        break;
                    }
                    default: {
                        offs *= bit_depth;
                        pixel_type result;
                        typename pixel_type::int_type r = 0;
                        if(pixel_type::byte_aligned) {
                            memcpy(&r,begin()+offs/8,pixel_type::packed_size);
#ifndef HTCW_GFX_NO_SWAP
                            r=bits::swap(r);
#endif
                            r&=pixel_type::mask;    
                            result.native_value = r;
                            *out_pixel=result;
                            break;
                        }
                        const size_t offs_bits = offs % 8;
                        const size_t siz = pixel_type::packed_size+(((int)pixel_type::pad_right_bits)<=offs_bits)+1;
                        uint8_t tmp[9];
                        tmp[siz]=0;
                        memcpy(tmp,begin()+offs/8,siz);
                        if(0<offs_bits)
                            bits::shift_left(tmp,0,siz*8,offs_bits);
                        
                        memcpy(&r,tmp,pixel_type::packed_size);
#ifndef HTCW_GFX_NO_SWAP
                        r=bits::swap(r);
#endif
                        r&=pixel_type::mask;
                        result.native_value=r;
                        *out_pixel=result;
                        break;
                    }
                }

            
                return gfx_result::success;
            }
            gfx_result point(point16 location,pixel_type rhs) {
                if(nullptr==begin()) {
                    return gfx_result::out_of_memory;
                }
                // clip
                if(location.x>=dimensions().width||location.y>=dimensions().height)
                    return gfx_result::success;
                /*if(pixel_type::template has_channel_names<channel_name::A>::value) {
                    pixel_type bgpx;
                    gfx_result r=point(location,&bgpx);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    r=convert_palette(*this,*this,rhs,&rhs,&bgpx);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                }*/
                return point_impl(location,rhs);
            }
            pixel_type point(point16 location) const {
                pixel_type result;
                point(location,&result);
                return result;
            }
            
         
            // indicates the size of the bitmap, in pixels
            inline size_t size_pixels() const {
                return m_dimensions.height*m_dimensions.width;
            }
            // indicates the size of the bitmap in bytes
            inline size_t size_bytes() const {
                return (size_pixels()*((PixelType::bit_depth+7)/8));
            }
          
            // indicates just past the end of the bitmap buffer
            inline uint8_t* end() const {
                return begin()+size_bytes();
            }
            // indicates just past the end of the bitmap buffer
            inline const uint8_t* cend() const {
                return cbegin()+size_bytes();
            }
            // clears a region of the bitmap
            gfx_result clear(const rect16& dst) {
                if(nullptr==begin()) {
                    return gfx_result::out_of_memory;
                }
                if(!dst.intersects(bounds())) return gfx_result::success;
                rect16 dstr = dst.crop(bounds());
                size_t dy = 0, dye=dstr.height();
                if(pixel_type::byte_aligned) {
                    const size_t line_len = dstr.width()*pixel_type::packed_size;
                    while(dy<dye) {
                    uint8_t* pdst = begin()+(((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::packed_size); 
                    memset(pdst,0,line_len);
                    ++dy;
                    }
                } else {
                    // unaligned version
                    const size_t line_len_bits = dstr.width()*pixel_type::bit_depth;
                    while(dy<dye) {
                        const size_t offs = (((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::bit_depth);
                        const size_t offs_bytes = offs / 8;
                        const size_t offs_bits = offs % 8;
                        uint8_t* pdst = begin()+offs_bytes;
                        bits::set_bits(pdst,offs_bits,line_len_bits,false);
                        ++dy;
                    }
                }
                return gfx_result::success;
            }
            // fills a region of the bitmap with the specified pixel
            gfx_result fill(const rect16& dst,pixel_type pixel) {
                if(!dst.intersects(bounds())) return gfx_result::success;
                if(nullptr==begin()) {
                    return gfx_result::out_of_memory;
                }
#ifndef HTCW_GFX_NO_SWAP
                typename pixel_type::int_type be_val = pixel.swapped();
#else
                typename pixel_type::int_type be_val = pixel.native_value;
#endif
                rect16 dstr = dst.crop(bounds());
                size_t dy = 0, dye=dstr.height();
                if(pixel_type::byte_alignment!=0) {
                    
                    if(pixel_type::byte_alignment==2) {
                        const size_t line_len = dstr.width();
                        while(dy<dye) {
                            uint16_t* pdst =(uint16_t*)(begin()+(((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::byte_alignment)); 
                            for(size_t x = 0;x<line_len;x+=1) {
                                *(pdst++) = be_val;
                            }
                            ++dy;
                        }
                    } else if(pixel_type::byte_alignment==4) {
                        const size_t line_len = dstr.width();
                        while(dy<dye) {
                            uint32_t* pdst =(uint32_t*)(begin()+(((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::byte_alignment)); 
                            for(size_t x = 0;x<line_len;x+=1) {
                                *(pdst++) = be_val;
                            }
                            ++dy;
                        }
                    } else if(pixel_type::byte_alignment==1) {
                        const size_t line_len = dstr.width();
                        while(dy<dye) {
                            uint8_t* pdst =(begin()+(((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::byte_alignment)); 
                            memset(pdst,be_val,line_len);
                            ++dy;
                        }
                    } else {
                        const size_t line_len = dstr.width()*pixel_type::byte_alignment;
                        while(dy<dye) {
                            uint8_t* pdst = begin()+(((dstr.top()+dy)*dimensions().width+dstr.left())*pixel_type::byte_alignment); 
                            for(size_t x = 0;x<line_len;x+=pixel_type::byte_alignment) {
                                memcpy(pdst,&be_val,pixel_type::byte_alignment);
                                pdst+=pixel_type::byte_alignment;
                            }
                            ++dy;
                        }
                    }
                    
                } else if(pixel_type::bit_depth!=1) {
                    // unaligned version
                    uint8_t buf[pixel_type::packed_size+1];
                    // set this to 9 to ensure no match on first iteration:
                    size_t old_offs_bits=9;
                    while(dy<dye) {
                        size_t dx = dstr.left();
                        while(dx<=dstr.right()) {
                            const size_t offs = (((dstr.top()+dy)*dimensions().width+dx)*pixel_type::bit_depth); 
                            const size_t offs_bits = offs % 8;
                            if(offs_bits!=old_offs_bits) {
                                memcpy(buf,&be_val,pixel_type::packed_size);
                                buf[pixel_type::packed_size]=0;
                                if(offs_bits!=0)
                                    bits::shift_right(buf,0,sizeof(buf)*8,offs_bits);
                            }
                            const size_t offs_bytes = offs/8;
                            bits::set_bits(offs_bits,pixel_type::bit_depth,begin()+offs_bytes,buf);
                            ++dx;
                            old_offs_bits=offs_bits;
                        }
                        ++dy;
                    }
                } else { // monochrome specialization
                    const size_t line_len_bits = dstr.width()*pixel_type::bit_depth;
                    bool set = 0!=pixel.native_value;
                    while(dy<dye) {
                        const size_t offs = (dstr.top()+dy)*dimensions().width+dstr.left()*pixel_type::bit_depth;
                        uint8_t* pdst = begin()+(offs/8);
                        bits::set_bits(pdst,offs%8,line_len_bits,set);
                        ++dy;
                    }
                }
                return gfx_result::success;
            }
            template<typename Destination>
            inline gfx_result copy_to(const rect16& src_rect,Destination& dst,point16 location) const {
                if(nullptr==begin())
                    return gfx_result::out_of_memory;
                if(!src_rect.intersects(bounds())) return gfx_result::success;
                rect16 srcr = src_rect.crop(bounds());
                rect16 dstr= rect16(location,srcr.dimensions()).crop(dst.bounds());
                srcr=rect16(srcr.location(),dstr.dimensions());
                return helpers::bmp_copy_to_helper<type,Destination,!(pixel_type::template has_channel_names<channel_name::A>::value)>::copy_to(*this,srcr,dst,dstr);
            }
            const palette_type *palette() const {
                return m_palette;
            }
            // computes the minimum required size for a bitmap buffer, in bytes
            constexpr inline static size_t sizeof_buffer(size16 size) {
                return (size.width*size.height*pixel_type::bit_depth+7)/8;
            }
            // computes the minimum required size for a bitmap buffer, in bytes
            constexpr inline static size_t sizeof_buffer(uint16_t width,uint16_t height) {
                return sizeof_buffer(size16(width,height));
            }
            
            // this check is already guaranteed by asserts in the pixel itself
            // this is more so it won't compile unless PixelType is actually a pixel
            static_assert(PixelType::channels>0,"The type is not a pixel or the pixel is invalid");

            static gfx_result point(size16 dimensions,const void* buffer, point16 location,pixel_type* out_pixel) {
                if(nullptr==buffer) {
                    return gfx_result::out_of_memory;
                }
                if(nullptr==out_pixel)
                    return gfx_result::invalid_argument;
                if(location.x>=dimensions.width||location.y>=dimensions.height) {
                    *out_pixel = pixel_type();
                    return gfx_result::success;
                }
                const size_t bit_depth = pixel_type::bit_depth;
                const size_t offs = (location.y*dimensions.width+location.x)*bit_depth;
                const size_t offs_bits = offs % 8;
                uint8_t tmp[8];
                tmp[pixel_type::packed_size]=0;
                memcpy(tmp,((uint8_t*)buffer)+offs/8,sizeof(tmp));
                if(0<offs_bits)
                    bits::shift_left(tmp,0,sizeof(tmp)*8,offs_bits);
                pixel_type result;
                typename pixel_type::int_type r = 0;
                memcpy(&r,tmp,pixel_type::packed_size);
#ifndef HTCW_GFX_NO_SWAP
                r=bits::swap(r);
#endif
                r&=pixel_type::mask;
                result.native_value=r;
                *out_pixel=result;
                return gfx_result::success;
            }
        };
        // represents an immutable in-memory bitmap
        template<typename PixelType,typename PaletteType>
        class const_bitmap_impl_base : public const_bitmap_base  {      
        protected:
            const PaletteType* m_palette;

        public:
            // the type of the bitmap, itself
            using type = const_bitmap_impl_base;
            // the type of the pixel used for the bitmap
            using pixel_type = PixelType;
            using palette_type = PaletteType;
            using caps = gfx::gfx_caps<true,PixelType::byte_alignment!=0,false,true>;
            
        public:
            // constructs a new bitmap with the specified size and buffer
            const_bitmap_impl_base(size16 dimensions,const void* buffer,const palette_type* palette=nullptr) : const_bitmap_base(dimensions,(const uint8_t*) buffer),m_palette(palette) {}
            // constructs a new bitmap with the specified width, height and buffer
            
            const_bitmap_impl_base(const type& rhs)=default;
            type& operator=(const type& rhs)=default;
            inline bool initialized() const {
                return nullptr!=m_cbegin;
            }
            gfx_result point(point16 location,pixel_type* out_pixel) const {
                if(nullptr==cbegin()) {
                    return gfx_result::invalid_state;
                }
                if(nullptr==out_pixel) {
                    return gfx_result::invalid_argument;
                }
                return bitmap_impl_base<pixel_type,palette_type>::point(dimensions(),m_cbegin,location,out_pixel);
            }
            
            // indicates the size of the bitmap, in pixels
            inline size_t size_pixels() const {
                return m_dimensions.height*m_dimensions.width;
            }
            // indicates the size of the bitmap in bytes
            inline size_t size_bytes() const {
                return (size_pixels()*pixel_type::bit_depth+7)/8;
            }
            // indicates just past the end of the bitmap buffer
            inline const uint8_t* cend() const {
                return cbegin()+size_bytes();
            }
            template<typename Destination>
            inline gfx_result copy_to(const rect16& src_rect,Destination& dst,point16 location) const {
                if(nullptr==cbegin())
                    return gfx_result::invalid_state;
                if(!src_rect.intersects(bounds())) return gfx_result::success;
                rect16 srcr = src_rect.crop(bounds());
                rect16 dstr= rect16(location,srcr.dimensions()).crop(dst.bounds());
                srcr=rect16(srcr.location(),dstr.dimensions());
                return helpers::bmp_copy_to_helper<type,Destination,!(pixel_type::template has_channel_names<channel_name::A>::value)>::copy_to(*this,srcr,dst,dstr);
            }
            const palette_type *palette() const {
                return m_palette;
            }
            // computes the minimum required size for a bitmap buffer, in bytes
            constexpr inline static size_t sizeof_buffer(size16 size) {
                return (size.width*size.height*pixel_type::bit_depth+7)/8;
            }
            // computes the minimum required size for a bitmap buffer, in bytes
            constexpr inline static size_t sizeof_buffer(uint16_t width,uint16_t height) {
                return sizeof_buffer(size16(width,height));
            }
            
            // this check is already guaranteed by asserts in the pixel itself
            // this is more so it won't compile unless PixelType is actually a pixel
            static_assert(PixelType::channels>0,"The type is not a pixel or the pixel is invalid");

        };
    }
    template<typename PixelType,typename PaletteType, bool BltSpans>
    class bitmap_impl : public helpers::bitmap_impl_base<PixelType,PaletteType> {
        using base_type = helpers::bitmap_impl_base<PixelType,PaletteType>;
    public:
    // the type of the pixel used for the bitmap
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx::gfx_caps< true,PixelType::byte_alignment!=0, false,true>;

        // constructs a new bitmap with the specified size and buffer
        bitmap_impl(size16 dimensions,void* buffer,const palette_type* palette=nullptr) : base_type(dimensions,buffer,palette) {}
        bitmap_impl() : base_type() {
            
        }
        bitmap_impl(const bitmap_impl& rhs) : base_type(rhs) {}
        bitmap_impl& operator=(const bitmap_impl& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        bitmap_impl(bitmap_impl&& rhs) : base_type(rhs) {}
        bitmap_impl& operator=(bitmap_impl&& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
    };

    template<typename PixelType,typename PaletteType>
    class bitmap_impl<PixelType,PaletteType,true> : public helpers::bitmap_impl_base<PixelType,PaletteType>, public blt_span {
        using base_type = helpers::bitmap_impl_base<PixelType,PaletteType>;
    public:
    // the type of the pixel used for the bitmap
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx::gfx_caps< true,PixelType::byte_alignment!=0, false,true>;

        // constructs a new bitmap with the specified size and buffer
        bitmap_impl(size16 dimensions,void* buffer,const palette_type* palette=nullptr) : base_type(dimensions,buffer,palette) {}
        bitmap_impl() : base_type() {
            
        }
        bitmap_impl(const bitmap_impl& rhs) : base_type(rhs) {}
        bitmap_impl& operator=(const bitmap_impl& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        bitmap_impl(bitmap_impl&& rhs) : base_type(rhs) {}
        bitmap_impl& operator=(bitmap_impl&& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        virtual size16 span_dimensions() const override {
            return this->m_dimensions;
        }
        virtual size_t pixel_width() const override {
            return pixel_type::byte_alignment;
        }
        virtual gfx_span span(point16 location) const override {
            constexpr static const size_t px_width = PixelType::byte_alignment;
            const size_t stride = this->m_dimensions.width*px_width;
            gfx_span span;
            span.data = this->m_begin + (location.y*stride)+(location.x*px_width);
            //printf("width: %d, loc_x:%d\n",this->m_dimensions.width,location.x);
            span.length = (this->m_dimensions.width-location.x)*px_width;
            return span;
        }
        virtual const gfx_cspan cspan(point16 location) const override {
            constexpr static const size_t px_width = PixelType::byte_alignment;
            const size_t stride = this->m_dimensions.width*px_width;
            gfx_cspan span;
            span.cdata = this->m_begin + (location.y*stride)+(location.x*px_width);
            span.length = (this->m_dimensions.width-location.x)*px_width;
            return span;
        }
    };

    template<typename PixelType,typename PaletteType, bool ByteAligned>
    class const_bitmap_impl : public helpers::const_bitmap_impl_base<PixelType,PaletteType>, public const_blt_span {
        using base_type = helpers::const_bitmap_impl_base<PixelType,PaletteType>;
    public:
    // the type of the pixel used for the bitmap
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx::gfx_caps< true,PixelType::byte_alignment!=0, false,true>;

        // constructs a new const_bitmap with the specified size and buffer
        const_bitmap_impl(size16 dimensions,const void* buffer,const palette_type* palette=nullptr) : base_type(dimensions,buffer,palette) {}
        
        const_bitmap_impl(const const_bitmap_impl& rhs) : base_type(rhs) {}
        const_bitmap_impl& operator=(const const_bitmap_impl& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        const_bitmap_impl(const_bitmap_impl&& rhs) : base_type(rhs) {}
        const_bitmap_impl& operator=(const_bitmap_impl&& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        virtual size16 span_dimensions() const override {
            return this->m_dimensions;
        }
        virtual size_t pixel_width() const override {
            return pixel_type::byte_alignment;
        }
        virtual const gfx_cspan cspan(point16 location) const override {
            constexpr static const size_t px_width = PixelType::byte_alignment;
            const size_t stride = this->m_dimensions.width*px_width;
            gfx_cspan span;
            span.cdata = this->m_cbegin + (location.y*stride)+(location.x*px_width);
            span.length = (this->m_dimensions.width-location.x)*px_width;
            return span;
        }
    };

    template<typename PixelType,typename PaletteType=palette<PixelType,PixelType>>
    class const_bitmap final : public const_bitmap_impl<PixelType,PaletteType,PixelType::byte_alignment!=0> {
        using base_type = const_bitmap_impl<PixelType,PaletteType,PixelType::byte_alignment!=0>;
    public:
        // the type of the pixel used for the bitmap
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx::gfx_caps<true,PixelType::byte_alignment!=0,false,true>;
        // constructs a new bitmap with the specified size and buffer
        const_bitmap(size16 dimensions,const void* buffer,const palette_type* palette=nullptr) : 
            base_type(dimensions,buffer,palette){}
        const_bitmap(const const_bitmap& rhs) : base_type(rhs) {}
        const_bitmap& operator=(const const_bitmap& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_cbegin = rhs.m_cbegin;
            this->m_palette = rhs.m_palette;
        }
    };
    
    template<typename PixelType,typename PaletteType=palette<PixelType,PixelType>>
    class bitmap final : public bitmap_impl<PixelType,PaletteType,PixelType::byte_alignment!=0> {
        using base_type = bitmap_impl<PixelType,PaletteType,PixelType::byte_alignment!=0>;
    public:
        using const_type = const_bitmap<PixelType,PaletteType>;
    // the type of the pixel used for the bitmap
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx::gfx_caps< true,PixelType::byte_alignment!=0, false,true>;

        // constructs a new bitmap with the specified size and buffer
        bitmap(size16 dimensions,void* buffer,const palette_type* palette=nullptr) : base_type(dimensions,buffer,palette) {}
        bitmap() : base_type() {
            
        }
        bitmap(const bitmap& rhs) : base_type(rhs) {}
        bitmap& operator=(const bitmap& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        bitmap(bitmap&& rhs) : base_type(rhs) {}
        bitmap& operator=(bitmap&& rhs) {
            this->m_dimensions = rhs.m_dimensions;
            this->m_palette = rhs.m_palette;
            this->m_begin = rhs.m_begin;
            return *this;
        }
        inline operator const_type() const {
            return const_type(this->m_dimensions,this->m_begin,this->m_palette);
        }
    };
    
    namespace helpers {
        template<typename Source,bool HasPalette> struct bitmap_from_helper {
            using type = bitmap<typename Source::pixel_type>;
            static type create_from(const Source& source,size16 size,void* buffer)  {
                return type(size,buffer);
            }
        };
        template<typename Source> struct bitmap_from_helper<Source,true> {
            using type = bitmap<typename Source::pixel_type, typename Source::palette_type>;
            static type create_from(const Source& source,size16 size,void* buffer)  {
                return type(size,buffer,source.palette());
            }
        };  
    }
    template<typename PixelType,typename PaletteType=palette<PixelType,PixelType>> inline static bitmap<PixelType,PaletteType> create_bitmap(size16 size,void*(allocator)(size_t)=::malloc, const PaletteType* palette=nullptr) {
        using bmp_type = bitmap<PixelType,PaletteType>;
        size_t sz = bmp_type::sizeof_buffer(size);
        return bmp_type(size,allocator(sz),palette);
    }
    
    // retrieves a type of bitmap based on the draw target
    template<typename Source> using bitmap_type_from=typename helpers::bitmap_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::type;
    // creates a bitmap based on the draw target
    template<typename Source> inline static bitmap_type_from<Source> create_bitmap_from(const Source& source,size16 size,void* buf) {
        return helpers::bitmap_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::create_from(source,size,buf);
    }
    template<typename Source> inline static bitmap_type_from<Source> create_bitmap_from(const Source& source,size16 size,void*(allocator)(size_t)=::malloc) {
        size_t sz = helpers::bitmap_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::type::sizeof_buffer(size);
        return helpers::bitmap_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::create_from(source,size,allocator(sz));
    }
    typedef void(*on_direct_read_callback_type)(uint32_t* buffer, const uint8_t* data,int length);
    typedef void(*on_direct_write_callback_type)(uint8_t* data, const uint32_t* buffer,int length);

    namespace helpers {
        
    // DIRECT MODE PIXEL FORMAT CONVERSION SPECIALIZATIONS 

    extern uint32_t xrgba32_to_argb32p(uint32_t pixel);
    extern uint32_t xrgba32_to_argb32(uint32_t pixel);
    extern uint32_t xargb32p_to_rgba32(uint32_t pixel);

    
    extern uint32_t xrgb16_to_argb32p(uint16_t pixel);

    extern uint16_t xargb32p_to_rgb16(uint32_t pxl);
    
    // DIRECT MODE CALLBACK SPECIALIZATIONS 
    extern void xread_callback_rgba32p(uint32_t* buffer, const uint8_t* data,
                                    int length);
    extern void xread_callback_rgba32(uint32_t* buffer, const uint8_t* data,
                                    int length);
    extern void xread_callback_rgb16(uint32_t* buffer, const uint8_t* data,
                                    int length);
    extern void xread_callback_gsc8(uint32_t* buffer, const uint8_t* data,
                                    int length);
    
                                    template <typename Source>
    static void xread_callback_any(uint32_t* buffer, const uint8_t* data,
                                    int length) {
        using vector_pixel = pixel<
            channel_traits<channel_name::A,8,0,255,255>,
            channel_traits<channel_name::R,8>,
            channel_traits<channel_name::G,8>,
            channel_traits<channel_name::B,8>
        >;
        static constexpr const size_t ba = Source::pixel_type::byte_alignment;
        static_assert(ba!=0,"invalid call");
        switch(ba) {
#if HTCW_MAX_WORD == 64
            case 8: {
                const uint64_t* target = (const uint64_t*)data;
                for (int i = 0; i < length; ++i) {
                    typename Source::pixel_type lhs(bits::swap(target[i]),true);
                    vector_pixel rhs;
                    convert(lhs,&rhs);
                    // Convert each RGBA Plain pixel to ARGB 
                    buffer[i] = rhs.native_value;
                }
            }
            break;
#endif
            case 4: {
                const uint32_t* target = (const uint32_t*)data;
                for (int i = 0; i < length; ++i) {
                    typename Source::pixel_type lhs(bits::swap(target[i]),true);
                    vector_pixel rhs;
                    convert(lhs,&rhs);
                    // Convert each RGBA Plain pixel to ARGB 
                    buffer[i] = rhs.native_value;
                }
            }
            break;
            case 2: {
                const uint16_t* target = (const uint16_t*)data;
                for (int i = 0; i < length; ++i) {
                    typename Source::pixel_type lhs(bits::swap(target[i]),true);
                    vector_pixel rhs;
                    convert(lhs,&rhs);
                    // Convert each RGBA Plain pixel to ARGB 
                    buffer[i] = rhs.native_value;
                }
            }
            break;
            case 1: {
                const uint8_t* target = (const uint8_t*)data;
                for (int i = 0; i < length; ++i) {
                    typename Source::pixel_type lhs(target[i],true);
                    vector_pixel rhs;
                    convert(lhs,&rhs);
                    // Convert each RGBA Plain pixel to ARGB 
                    buffer[i] = rhs.native_value;
                }
            }
            break;
            default: {
                const uint8_t* target = (const uint8_t*)data;
                int j=0;
                for (size_t i = 0; i < length*ba; i+=ba) {
                    typename Source::pixel_type::int_type v;
                    memcpy(&v,&target[i],ba);
#ifndef HTCW_GFX_NO_SWAP
                    typename Source::pixel_type lhs(bits::swap(v),true);
#else
                    typename Source::pixel_type lhs(v,true);
#endif
                    vector_pixel rhs;
                    convert(lhs,&rhs);
#ifndef HTCW_GFX_NO_SWAP
                    // Convert each RGBA Plain pixel to ARGB 
                    buffer[j] = rhs.native_value;
#else
                    buffer[j] = rhs.swapped();
#endif
                    ++j;
                }
            }
            break;
        }
    }
    extern void xwrite_callback_rgba32(uint8_t* data, const uint32_t* buffer,
                                    int length);
    extern void xwrite_callback_rgb16(uint8_t* data, const uint32_t* buffer,
                                    int length);
    extern void xwrite_callback_gsc8(uint8_t* data, const uint32_t* buffer,
                                int length);
    template<typename Destination>
    static void xwrite_callback_any(uint8_t* data, const uint32_t* buffer,
                                    int length) {
        using vector_pixel = pixel<
            channel_traits<channel_name::A,8,0,255,255>,
            channel_traits<channel_name::R,8>,
            channel_traits<channel_name::G,8>,
            channel_traits<channel_name::B,8>
        >;
        static constexpr const size_t ba = Destination::pixel_type::byte_alignment;
        static_assert(ba!=0,"invalid call");
        switch(ba) {
#if HTCW_MAX_WORD == 64
            case 8: {
                uint64_t* target = (uint64_t*)data;
                for (int i = 0; i < length; ++i) {
                    vector_pixel lhs(buffer[i],true);
                    typename Destination::pixel_type rhs;
                    convert(lhs,&rhs);
#ifndef HTCW_GFX_NO_SWAP
                    target[i] = rhs.swapped();
#else
                    target[i] = rhs.native_value;
#endif

                }
            }
            break;
#endif
            case 4: {
                uint32_t* target = (uint32_t*)data;
                for (int i = 0; i < length; ++i) {
                    vector_pixel lhs(buffer[i],true);
                    typename Destination::pixel_type rhs;
                    convert(lhs,&rhs);
#ifndef HTCW_GFX_NO_SWAP
                    target[i] = rhs.swapped();
#else
                    target[i] = rhs.native_value;
#endif
                }
            }
            break;
            case 2: {
                uint16_t* target = (uint16_t*)data;
                for (int i = 0; i < length; ++i) {
                    vector_pixel lhs(buffer[i],true);
                    typename Destination::pixel_type rhs;
                    convert(lhs,&rhs);
#ifndef HTCW_GFX_NO_SWAP
                    target[i] = rhs.swapped();
#else
                    target[i] = rhs.native_value;
#endif
                }
            }
            break;
            case 1: {
                uint8_t* target = (uint8_t*)data;
                for (int i = 0; i < length; ++i) {
                    vector_pixel lhs(buffer[i],true);
                    typename Destination::pixel_type rhs;
                    convert(lhs,&rhs);
                    target[i] = rhs.native_value;
                }
            }
            break;
            default: {
                uint8_t* target = (uint8_t*)data;
                int j=0;
                for (size_t i = 0; i < length*ba; i+=ba) {
                    vector_pixel lhs(buffer[j],true);
                    typename Destination::pixel_type rhs;
                    convert(lhs,&rhs);
                    // auto rr = rhs.channel<channel_name::R>();
                    // auto gg = rhs.channel<channel_name::G>();
                    // auto bb = rhs.channel<channel_name::B>();
#ifndef HTCW_GFX_NO_SWAP
                    typename Destination::pixel_type::int_type rhsv=rhs.swapped();
#else
                    typename Destination::pixel_type::int_type rhsv=rhs.native_value;
#endif
                    memcpy(&target[i],&rhsv,ba);
                    ++j;
                }
            }
            break;
        }                      
    }
}
}
#endif