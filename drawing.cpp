#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include "include/gfx_bitmap.hpp"
#include "include/gfx_drawing.hpp"
using namespace gfx;
template <typename PixelType>
void dump_channel_values(const PixelType& p) {
    printf("channels = { %d (0x%X)",(int)p.template channel<0>(),(int)p.template channel<0>());
    if(p.channels>1) {
        printf(", %d (0x%X)",(int)p.template channel_unchecked<1>(),(int)p.template channel_unchecked<1>());
        if(p.channels>2) {
            printf(", %d (0x%X)",(int)p.template channel_unchecked<2>(),(int)p.template channel_unchecked<2>());
            if(p.channels>3) {
                printf(", %d (0x%X)",(int)p.template channel_unchecked<3>(),(int)p.template channel_unchecked<3>());
            }
        }
    }
    printf(" }\r\n");
}
template <typename BitmapType>
void dump_bitmap(const BitmapType& bmp) {
    static const char *col_table = " .,-~;*+!=1%O@$#";
    using gsc4 = pixel<channel_traits<channel_name::L,4>>;
    for(int y = 0;y<bmp.dimensions().height;++y) {
        for(int x = 0;x<bmp.dimensions().width;++x) {
            const typename BitmapType::pixel_type px = bmp[point16(x,y)];
            char sz[2];
            sz[1]=0;
            const auto px2 = px.template convert<gsc4>();
            size_t i =px2.template channel<0>();
            sz[0] = col_table[i];
            printf("%s",sz);
            
        }
        printf("\r\n");
    }
}
int main() {
    static const size_t bit_depth = 24;
    
    // our type definitions
    using bmp_type = bitmap<rgb_pixel<bit_depth>>;
    using color = color<typename bmp_type::pixel_type>;

    size16 bmp_size(32,32);
    
    // declare the bitmap
    uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
    bmp_type bmp(bmp_size,bmp_buf);

    // draw stuff
    bmp.clear(bmp.bounds()); // comment this out and check out the uninitialized RAM. It looks neat.

    // bounding info for the face
    srect16 bounds(0,0,bmp_size.width-1,(bmp_size.height-1)/(4/3.0));
    rect16 ubounds(0,0,bounds.x2,bounds.y2);
    // draw the face
    draw::filled_ellipse(bmp,bounds,color::yellow);
    // draw the left eye
    srect16 eye_bounds_left(spoint16(bounds.width()/5,bounds.height()/5),ssize16(bounds.width()/5,bounds.height()/3));
    draw::filled_ellipse(bmp,eye_bounds_left,color::black);
    // draw the right eye
    srect16 eye_bounds_right(
        spoint16(
            bmp_size.width-eye_bounds_left.x1-eye_bounds_left.width(),
            eye_bounds_left.y1
        ),eye_bounds_left.dimensions());
    draw::filled_ellipse(bmp,eye_bounds_right,color::black);
    
    // draw the mouth
    srect16 mouth_bounds=bounds.inflate(-bounds.width()/7,-bounds.height()/8).normalize();
    // we need to clip part of the circle we'll be drawing
    srect16 mouth_clip(mouth_bounds.x1,mouth_bounds.y1+mouth_bounds.height()/(float)1.6,mouth_bounds.x2,mouth_bounds.y2);
    draw::ellipse(bmp,mouth_bounds,color::black,&mouth_clip);
   
    // now blt the bitmaps
    const size_t count  =3; // 3 faces
    // our second bitmap. Not strictly necessary because one
    // can draw to the same bitmap they copy from
    // but doing so can cause garbage if the regions
    // overlap. This way is safer but takes more RAM
    using bmp2_type = bitmap<typename bmp_type::pixel_type>;
    size16 bmp2_size(bmp_size.width*count,bmp_size.height);
    uint8_t buf2[bmp2_type::sizeof_buffer(bmp2_size)];
    bmp2_type bmp2(bmp2_size,buf2);

    // if we don't do the following we'll get uninitialized garbage.
    // it looks neat though:
    bmp2.clear(bmp2.bounds());
    srect16 r = bounds;
    // how much to shrink each iteration:
    spoint16 shrink(-bounds.width()/8,-bounds.height()/8);
    for(size_t i = 0;i<count;++i) {
        // draw the bitmap
        draw::bitmap(bmp2,r,bmp,ubounds,bitmap_flags::resize);
        // move the rect, flip and shrink it
        r=r.offset(r.width(),0);
        r=r.inflate(shrink.x,shrink.y);
        r=r.flip_vertical();     
    }
    // display our mess
    dump_bitmap(bmp2);
    
    return 0;
}