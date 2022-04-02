#ifndef HTCW_GFX_SPRITE
#define HTCW_GFX_SPRITE
#include "gfx_core.hpp"
#include "gfx_positioning.hpp"
#include "gfx_pixel.hpp"
#include "gfx_palette.hpp"
#include "gfx_bitmap.hpp"

namespace gfx {
    
    template<typename PixelType,typename PaletteType = palette<PixelType,PixelType>>
    class sprite final {
        using bitmap_type = bitmap<PixelType,PaletteType>;
        bitmap_type m_bitmap;
        using mask_type = bitmap<gsc_pixel<1>>;
        mask_type m_mask;
        
    public:
        using type = sprite;
        using pixel_type = PixelType;
        using palette_type = PaletteType;
        using caps = gfx_caps<true,false,false,false,false,true,true>;
        sprite(size16 size, void* buf, void* mask_buf,const palette_type* palette=nullptr) : 
                m_bitmap(size,buf,palette),m_mask(size,mask_buf) {
        }

        inline size16 dimensions() const { return m_bitmap.dimensions(); }
        inline rect16 bounds() const { return dimensions().bounds(); }

        inline gfx_result point(point16 location,PixelType* out_color) const {
            return m_bitmap.point(location,out_color);
        }
        inline palette_type* palette() const {
            return m_bitmap.palette();
        }
        inline const uint8_t* begin() const {
            return m_bitmap.begin();
        }
        template<typename Destination>
        inline gfx_result copy_to(const rect16& src_rect,Destination& dst,point16 location) const {
            return m_bitmap.copy_to(src_rect,dst,location);
        }
        // indicates whether the location within the sprite square is actually part of the sprite
        inline bool hit_test(point16 location) const {
            gsc_pixel<1> px;
            m_mask.point(location,&px);
            return px.native_value;
        }
    };
    namespace helpers {
        template<typename Source,bool HasPalette> struct sprite_from_helper {
            using type = sprite<typename Source::pixel_type>;
            inline static type create_from(const Source& source,size16 size,void* buffer,void* mask_buffer)  {
                return type(size,buffer,mask_buffer);
            }
        };
        template<typename Source> struct sprite_from_helper<Source,true> {
            using type = sprite<typename Source::pixel_type, typename Source::palette_type>;
            inline static type create_from(const Source& source,size16 size,void* buffer,void* mask_buffer)  {
                return type(size,buffer,source.palette());
            }
        };  
    }
    // retrieves a type of sprite based on the draw target
    template<typename Source> using sprite_type_from=typename helpers::sprite_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::type;
    // creates a sprite based on the draw target
    template<typename Source> inline static sprite_type_from<Source> create_sprite_from(const Source& source,size16 size,void* sprite_buf,void* mask_buf) {
        return helpers::sprite_from_helper<Source,Source::pixel_type::template has_channel_names<channel_name::index>::value>::create_from(source,size,sprite_buf,mask_buf);
    }
}

#endif // HTCW_GFX_SPRITE