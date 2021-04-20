#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include "include/gfx_bitmap.hpp"
using namespace gfx;
template <typename PixelType>
void dump_channel_values(const PixelType& p) {
    printf("channels = { 0x%X",(int)p.template channel<0>());
    if(p.channels>1) {
        printf(", 0x%X",(int)p.template channel_unchecked<1>());
        if(p.channels>2) {
            printf(", 0x%X",(int)p.template channel_unchecked<2>());
            if(p.channels>3) {
                printf(", 0x%X",(int)p.template channel_unchecked<3>());
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

    // predefine many different pixel types you can try
    // Note that this generates compile warnings for
    // unused instantiations
    using bgr888 = pixel<
        channel_traits<channel_name::B,8>,
        channel_traits<channel_name::G,8>,
        channel_traits<channel_name::R,8>
    >;    

    using rgb565 = pixel<
        channel_traits< channel_name::R,5>,
        channel_traits<channel_name::G,6>,
        channel_traits<channel_name::B,5>
    >;

    using gsc8 = pixel<
        channel_traits<channel_name::L,8>
    >;

    using mono1 = pixel<
        channel_traits<channel_name::L,1>
    >;
    using gsc2 = pixel<
        channel_traits<channel_name::L,2>
    >;
    using gsc4 = pixel<
        channel_traits<channel_name::L,4>
    >;
    using rgb888 = pixel<
            channel_traits<channel_name::R,8>,
            channel_traits<channel_name::G,8>,
            channel_traits<channel_name::B,8>
    >;
    using rgb666 = pixel<
            channel_traits<channel_name::R,6>,
            channel_traits<channel_name::G,6>,
            channel_traits<channel_name::B,6>
    >;
    using rgb232 = pixel<
            channel_traits<channel_name::R,2>,
            channel_traits<channel_name::G,3>,
            channel_traits<channel_name::B,2>
    >;
    using rgb333 = pixel<
            channel_traits<channel_name::R,3>,
            channel_traits<channel_name::G,3>,
            channel_traits<channel_name::B,3>
    >;
    // you can use rgb_pixel<> and rgba_pixel<>
    // helpers to create an rgb/rgba pixel
    // of any bit depth (up to the max)
    using rgb101210 = rgb_pixel<32>;
    
    using rgb212221 = rgb_pixel<64>;
    

    using yuv888 = pixel<
            channel_traits<channel_name::Y,8>,
            channel_traits<channel_name::U,8>,
            channel_traits<channel_name::V,8>
    >;

    // set your pixel type to any one of the above
    // or make your own pixel format     
    using bmp_type = bitmap<rgb_pixel<16>::type>;

    // we declare this to make it easier to change 
    const size16 bmp_size(15,15);

    // declare our buffer
    uint8_t buf[bmp_type::sizeof_buffer(bmp_size)];

    // declare our bitmap, using the bmp_size and the buffer
    bmp_type bmp(bmp_size,buf);

    // for each pixel in the bitmap, working from top to bottom,
    // left to right, if the pixel falls on an edge, make it white
    // otherwise, alternate the colors between dark_blue and purple
    bool col = false;
    for(int y=0;y<bmp.dimensions().height;++y) {
        for(int x=0;x<bmp.dimensions().width;++x) {
            if(x==0||y==0||x==bmp.bounds().right()||y==bmp.bounds().bottom()) {
                bmp[point16(x,y)]=color<typename bmp_type::pixel_type>::white;
            } else {
                bmp[point16(x,y)]=col?
                    color<typename bmp_type::pixel_type>::dark_blue:
                    color<typename bmp_type::pixel_type>::purple;
            }
            col = !col;
        }    
    }
    
    // display our initial bitmap
    dump_bitmap(bmp);
    printf("\r\n");
    
    // create rect for our inner square
    rect16 r = bmp.bounds().inflate(-3,-3);
    // clear it
    bmp.clear(r);
    // now fill a rect inside that
    bmp.fill(r.inflate(-1,-1),color<bmp_type::pixel_type>::medium_aquamarine);
    
    // display the bitmap
    dump_bitmap(bmp);
    printf("\r\n");

    // create a second bitmap 4 times the size
    const size16 bmp2_size(bmp_size.width*2,bmp_size.height*2);
    uint8_t buf2[bmp_type::sizeof_buffer(bmp2_size)];
    bmp_type bmp2(bmp2_size,buf2);
    // clear it
    bmp2.clear(bmp2.bounds());

    // now blt portions of the first bitmap to it to create a tile
    // effect
    bmp.blt(rect16(point16(8,8),size16(7,7)),bmp2,point16(0,0));
    bmp.blt(rect16(point16(0,8),size16(7,7)),bmp2,point16(22,0));
    bmp.blt(bmp.bounds(),bmp2,point16(7,7));
    bmp.blt(rect16(point16(8,0),size16(7,7)),bmp2,point16(0,22));
    bmp.blt(rect16(point16(0,0),size16(7,7)),bmp2,point16(22,22));
    // display the bitmap
    dump_bitmap(bmp2);
        
    return 0;
}