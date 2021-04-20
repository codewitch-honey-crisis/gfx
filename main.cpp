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
    static const size_t bit_depth = 64;
    const auto mask_left = bits::mask<bit_depth>::left;
    const auto mask_right = bits::mask<bit_depth>::right;
    const auto max = helpers::order_guard(mask_left);


    // our type definitions
    using bmp_type = bitmap<rgb_pixel<bit_depth>>;
    using color = color<typename bmp_type::pixel_type>;
    const auto chmask = bmp_type::pixel_type::channel_by_index<0>::channel_mask;
    const auto chmax = bmp_type::pixel_type::channel_by_index<0>::max;
    const auto vmask = bmp_type::pixel_type::channel_by_index<0>::value_mask;
    auto px = color::white;//bmp_type::pixel_type(true,1,1,1);
    dump_channel_values(px);
    for(size_t i = 0;i<sizeof(px.native_value);++i) {
        printf("%02X",(int)((uint8_t*)&px.native_value)[i]);
    }
    printf("\r\n");
    //return 0;
    // declare the bitmap
    size16 bmp_size(128,128);
    uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
    bmp_type bmp(bmp_size,bmp_buf);

    // draw stuff
    bmp.clear(bmp.bounds()); // comment this out and check out the uninitialized RAM. It looks neat.
    srect16 b =srect16(spoint16(0,0),ssize16(bmp.dimensions().width,bmp.dimensions().height));
    srect16 cr = b.inflate(-20,-20);
    draw::filled_ellipse(bmp,cr,color::lawn_green);
    draw::ellipse(bmp,cr,color::lavender_blush);
    srect16 r = b.inflate(-4,0);
    srect16 r2=r.inflate(-r.width()*.30,-r.height()*.30);
    srect16 fr2 = r2.inflate(15,5);
    draw::filled_rectangle(bmp,fr2,color::purple);
    draw::rectangle(bmp,fr2,color::hot_pink);
    draw::line(bmp,r,color::white,&r2);
    draw::line(bmp,r.flip_horizontal(),color::white);
    srect16 ar(spoint16(15,15),ssize16(7,5));
    ar=ar.flip_horizontal();
    //ar=ar.flip_vertical();
    draw::filled_rectangle(bmp,ar,color::dark_blue);
    draw::filled_arc(bmp,ar,color::sky_blue);
    draw::arc(bmp,ar,color::white);
    r2=r2.inflate(-10,-17);
    draw::filled_rectangle(bmp,r2,color::dark_blue);
    
    //draw::filled_rounded_rectangle(bmp,r2,pointf(.2,.2),color::tan);
    draw::rounded_rectangle(bmp,r2,pointf(.2,.2),color::white);
    auto src = rect16(point16(15,15),size16(10,10));
    auto dst = srect16(10,0,20,10);
    draw::bitmap( bmp,dst,bmp,src,bitmap_flags::crop);
    bmp[point16(0,0)]=color::white;
    typename bmp_type::pixel_type px2=bmp[point16(0,0)];
    dump_channel_values(px2);
    dump_bitmap(bmp);
    
    
    return 0;
}