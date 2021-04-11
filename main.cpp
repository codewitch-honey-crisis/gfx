#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include "include/gfx_pixel.hpp"
using namespace gfx;
int main() {

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
    using rgb888 = pixel<
            channel_traits<channel_name::R,8>,
            channel_traits<channel_name::G,8>,
            channel_traits<channel_name::B,8>
        >;
    auto pixel = rgb888(rand()%256,rand()%256,rand()%256);
    // see if the two pixel types have the same
    // channel names in the same order
    printf("rgb888::equals<rgb565> = %s\r\n",
        rgb888::equals<rgb565>::value?"true":"false");

    // same as above, different pixel types
    printf("bgr888::equals<rgb565> = %s\r\n",
        bgr888::equals<rgb565>::value?"true":"false");

    // see if the two pixel types have the same
    // channel names in any order
    printf("bgr888::unordered_equals<rgb565> = %s\r\n",
        bgr888::unordered_equals<rgb565>::value?"true":"false");

    // check for an RGB color model
    printf("bgr888::has_channel_names<R,G,B> = %s\r\n",
        bgr888::has_channel_names<
            channel_name::R,
            channel_name::G,
            channel_name::B
            >::value?"true":"false");

    printf("\r\n");

    // example checking for black and white
    // color model
    bool isBW = mono1::has_channel_names<
            channel_name::L>::value &&
            mono1::channels==1;


    printf("channel<0>::bits_to_left = %d\r\n",(int)mono1::channel_by_index<0>::bits_to_left);
    printf("channel<0>::bits_to_right = %d\r\n",(int)mono1::channel_by_index<0>::bits_to_right);
    printf("channel<0>::total_bits_to_right = %d\r\n",(int)mono1::channel_by_index<0>::total_bits_to_right);
    printf("\r\n");
    
    mono1 m(1);
    uint8_t mcb[sizeof(m)];
    memcpy(mcb,&m,sizeof(mcb));
    for(size_t i = 0;i<sizeof(mcb);++i) {
        
        printf("%02X",mcb[i]);
    }
    printf("\r\n\r\n");
    
    printf("channel<0>::bits_to_left = %d\r\n",(int)rgb888::channel_by_index<0>::bits_to_left);
    printf("channel<0>::bits_to_right = %d\r\n",(int)rgb888::channel_by_index<0>::bits_to_right);
    printf("channel<0>::total_bits_to_right = %d\r\n",(int)rgb888::channel_by_index<0>::total_bits_to_right);
    printf("channel<1>::bits_to_left = %d\r\n",(int)rgb888::channel_by_index<1>::bits_to_left);
    printf("channel<1>::bits_to_right = %d\r\n",(int)rgb888::channel_by_index<1>::bits_to_right);
    printf("channel<1>::total_bits_to_right = %d\r\n",(int)rgb888::channel_by_index<1>::total_bits_to_right);
    printf("channel<2>::bits_to_left = %d\r\n",(int)rgb888::channel_by_index<2>::bits_to_left);
    printf("channel<2>::bits_to_right = %d\r\n",(int)rgb888::channel_by_index<2>::bits_to_right);
    printf("channel<2>::total_bits_to_right = %d\r\n",(int)rgb888::channel_by_index<2>::total_bits_to_right);
    printf("\r\n");
    auto cvt=rgb888(true,1,0,.3333);
    uint8_t cb[sizeof(cvt)];
    auto cval = cvt.value();
    memcpy(cb,&cval,sizeof(cb));
    for(size_t i = 0;i<sizeof(cb);++i) {
        
        printf("%02X",cb[i]);
    }
    printf("\r\n\r\n");
    printf("rgb565(true,1,0,.3333).convert<rgb888>().channel<0>() = %d\r\n",(int)cvt.channel<0>());
    printf("rgb565(true,1,0,.3333).convert<rgb888>().channel<1>() = %d\r\n",(int)cvt.channel<1>());
    printf("rgb565(true,1,0,.3333).convert<rgb888>().channel<2>() = %d\r\n",(int)cvt.channel<2>());
    printf("\r\n");

    rgb888 rgb=rgb888(true,1,0,.3333);
    printf("rgb888(true,1,0,.3333).channel<0>() = %d\r\n",(int)rgb.channel<0>());
    printf("rgb888(true,1,0,.3333).channel<1>() = %d\r\n",(int)rgb.channel<1>());
    printf("rgb888(true,1,0,.3333).channel<2>() = %d\r\n",(int)rgb.channel<2>());
    printf("\r\n");

    printf("rgb888(true,1,0,.3333) memory layout (big endian): 0x");
    uint8_t pb[sizeof(rgb)];
    auto rval = rgb.value();
    memcpy(pb,&rval,sizeof(pb));
    for(size_t i = 0;i<sizeof(pb);++i) {
        
        printf("%02X",pb[i]);
    }
    printf("\r\n\r\n");

    printf("color<gsc8>::red.channel<0>() = %d\r\n",(int)color<gsc8>::red.channel<0>());
    printf("color<gsc8>::green.channel<0>() = %d\r\n",(int)color<gsc8>::green.channel<0>());
    printf("color<gsc8>::blue.channel<0>() = %d\r\n",(int)color<gsc8>::blue.channel<0>());
    printf("\r\n");
    
    printf("color<mono1>::red.channel<0>() = %d\r\n",(int)color<mono1>::red.channel<0>());
    printf("color<mono1>::green.channel<0>() = %d\r\n",(int)color<mono1>::green.channel<0>());
    printf("color<mono1>::blue.channel<0>() = %d\r\n",(int)color<mono1>::blue.channel<0>());
    printf("\r\n");
    return 0;

}