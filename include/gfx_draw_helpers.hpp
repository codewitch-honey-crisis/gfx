#ifndef HTCW_GFX_DRAW_HELPERS_HPP
#define HTCW_GFX_DRAW_HELPERS_HPP
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include "gfx_positioning.hpp"

namespace gfx {
    namespace helpers {
        template<typename Destination,typename Source,bool HasAlpha> 
        struct blender {
            static gfx_result point(Destination& destination,point16 pt,Source& source,point16 spt, typename Source::pixel_type pixel) {
                typename Destination::pixel_type px;
               // printf("pixel.native_value = %d\r\n",(int)pixel.native_value);
                gfx_result r=convert_palette(destination,source,pixel,&px);
                //printf("px.native_value = %d\r\n",(int)px.native_value);
                if(gfx_result::success!=r) {
                    return r;
                }
                return destination.point(pt,px);
            }
        };
        
        template<typename Source, typename Destination,bool CanRead> struct blend_helper {
            static constexpr gfx_result do_blend(const Source& src, typename Source::pixel_type spx,Destination& dst, point16 dstpnt,typename Destination::pixel_type* out_px) {
                return convert_palette(dst,src,spx,out_px,nullptr);
            }
        };
        
        template<typename Source, typename Destination> struct blend_helper<Source,Destination,true> {
            static constexpr gfx_result do_blend(const Source& src, typename Source::pixel_type spx,Destination& dst, point16 dstpnt,typename Destination::pixel_type* out_px) {
                gfx_result r=gfx_result::success;
                typename Destination::pixel_type bgpx;
                r=dst.point(point16(dstpnt),&bgpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert_palette(dst,src,spx,out_px,&bgpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return gfx_result::success;
            }
        };
        template<typename Destination,typename Source> 
        struct blender<Destination,Source,true> {
            static gfx_result point(Destination& destination,point16 pt,Source& source,point16 spt, typename Source::pixel_type pixel) {
                
                double alpha = pixel.template channelr<channel_name::A>();
                if(0.0==alpha) return gfx_result::success;
                if(1.0==alpha) return blender<Destination,Source,false>::point(destination,pt,source,spt,pixel);
                typename Source::pixel_type spx;
                gfx_result r=source.point(spt,&spx);
                if(gfx_result::success!=r) {
                    return r;
                }
                rgb_pixel<HTCW_MAX_WORD> bg;
                r=convert_palette_to(source,spx,&bg);
                if(gfx_result::success!=r) {
                    return r;
                }
                rgb_pixel<HTCW_MAX_WORD> fg;
                r=convert_palette_to(source,pixel,&fg);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=fg.blend(bg,alpha,&fg);
                if(gfx_result::success!=r) {
                    return r;
                }
                typename Destination::pixel_type px;
                r=convert_palette_from(destination,fg,&px);
                if(gfx_result::success!=r) {
                    return r;
                }
                return destination.point(pt,px);
            }
        };
        template<typename Destination,bool Batch,bool Async> 
        struct batcher {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                return gfx_result::success;
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                return destination.point(location,pixel);
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                return gfx_result::success;
            }
        };
        template<typename Destination> 
        struct batcher<Destination,false,true> {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                return gfx_result::success;
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                if(async) {
                    return destination.point_async(location,pixel);
                } else {
                    return destination.point(location,pixel);
                }
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                return gfx_result::success;
            }
        };
        template<typename Destination,bool Batch,bool Async, bool Read> 
        struct alpha_batcher {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                return batcher<Destination,Batch,Async>::begin_batch(destination,bounds,async);
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                return batcher<Destination,Batch,Async>::write_batch(destination,location,pixel,async);
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                return batcher<Destination,Batch,Async>::commit_batch(destination,async);
            }
        };
        template<typename Destination,bool Batch,bool Async> 
        struct alpha_batcher<Destination,Batch,Async,true> {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                return batcher<Destination,false,Async>::begin_batch(destination,bounds,async);
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                return batcher<Destination,false,Async>::write_batch(destination,location,pixel,async);
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                return batcher<Destination,false,Async>::commit_batch(destination,async);
            }
        };
        template<typename Destination> 
        struct batcher<Destination,true,false> {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                return destination.begin_batch(bounds);
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                return destination.write_batch(pixel);
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                return destination.commit_batch();
            }
        };
        template<typename Destination> 
        struct batcher<Destination,true,true> {
            inline static gfx_result begin_batch(Destination& destination,const rect16& bounds,bool async) {
                if(async) {
                    return destination.begin_batch_async(bounds);
                } else {
                    return destination.begin_batch(bounds);
                }
            }
            inline static gfx_result write_batch(Destination& destination,point16 location,typename Destination::pixel_type pixel,bool async) {
                if(async) {
                    return destination.write_batch_async(pixel);
                } else {
                    return destination.write_batch(pixel);
                }
            }
            inline static gfx_result commit_batch(Destination& destination,bool async) {
                if(async) {
                    return destination.commit_batch_async();
                } else {
                    return destination.commit_batch();
                }
                
            }
        };
        template<typename Destination,bool Suspend,bool Async>
        struct suspender {
            inline suspender(Destination& dest,bool async=false) {

            }
            inline suspender(const suspender& rhs) = default;
            inline suspender& operator=(const suspender& rhs)=default;
            inline ~suspender() =default;
            inline static gfx_result suspend(Destination& dst) {
                return gfx_result::not_supported;
            }
            inline static gfx_result resume(Destination& dst,bool force=false) {
                return gfx_result::not_supported;
            }
            inline static gfx_result suspend_async(Destination& dst) {
                return gfx_result::not_supported;
            }
            inline static gfx_result resume_async(Destination& dst,bool force=false) {
                return gfx_result::not_supported;
            }
        };
        template<typename Destination>
        struct suspender<Destination,true,false> {
            Destination& destination;
            inline suspender(Destination& dest,bool async=false) : destination(dest) {
                suspend(destination);
            }
            inline suspender(const suspender& rhs) {
                suspend(destination);
            }
            inline suspender& operator=(const suspender& rhs) {
                destination = rhs.destination;
                suspend(destination);
                return *this;
            }
            inline ~suspender() {
                resume(destination);
            }   
            inline static gfx_result suspend(Destination& dst) {
                return dst.suspend();
            }
            inline static gfx_result resume(Destination& dst,bool force=false) {
                if(force) {
                    return dst.resume(true);
                }
                return dst.resume();
            }
            inline static gfx_result suspend_async(Destination& dst) {
                return suspend(dst);
            }
            inline static gfx_result resume_async(Destination& dst,bool force=false) {
                if(force) {
                    return dst.resume(true);
                }
                return resume(dst);
            }
        };
        template<typename Destination>
        struct suspender<Destination,true,true> {
            Destination& destination;
            const bool async;
            inline suspender(Destination& dest,bool async=false) : destination(dest),async(async) {
                if(async) {
                    suspend_async(destination);
                    return;
                }
                suspend(destination);
            }
            inline suspender(const suspender& rhs) {
                destination = rhs.destination;
                if(async) {
                    suspend_async(destination);
                    return;
                }
                suspend(destination);
            }
            inline suspender& operator=(const suspender& rhs) {
                destination = rhs.destination;
                if(async) {
                    suspend_async(destination);
                    return *this;
                }
                suspend(destination);
                return *this;
            }
            inline ~suspender() {
                if(async) {
                    resume_async(destination);
                    return;
                }
                resume(destination);
            }    
            inline static gfx_result suspend(Destination& dst) {
                return dst.suspend();
            }
            inline static gfx_result resume(Destination& dst,bool force=false) {
                if(force) {
                    return dst.resume(true);
                }
                return dst.resume();
            }
            inline static gfx_result suspend_async(Destination& dst) {
                return dst.suspend_async();
            }
            inline static gfx_result resume_async(Destination& dst,bool force=false) {
                if(force) {
                    dst.resume_async(true);
                }
                return dst.resume_async();
            }
        };
    }
}
#endif