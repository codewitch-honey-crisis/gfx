#ifndef HTCW_GFX_DRAW_HELPERS_HPP
#define HTCW_GFX_DRAW_HELPERS_HPP
#include "gfx_positioning.hpp"

namespace gfx {
    namespace helpers {
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
                return gfx_result::success;
            }
            inline static gfx_result resume(Destination& dst) {
                return gfx_result::success;
            }
            inline static gfx_result suspend_async(Destination& dst) {
                return gfx_result::success;
            }
            inline static gfx_result resume_async(Destination& dst,bool force=false) {
                return gfx_result::success;
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
            inline static gfx_result resume(Destination& dst) {
                return dst.resume();
            }
            inline static gfx_result suspend_async(Destination& dst) {
                return suspend(dst);
            }
            inline static gfx_result resume_async(Destination& dst,bool force) {
                if(force) {
                    return resume(dst,true);
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