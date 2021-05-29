#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include <string.h>
#include <gfx_bitmap.hpp>
#include <gfx_drawing.hpp>
#include <gfx_color_cpp14.hpp>
#include "../fonts/Bm437_Acer_VGA_8x8.h"
using namespace gfx;

#define PATH_CHAR '/'

// prints a bitmap as 4-bit grayscale ASCII
template <typename BitmapType>
void dump_bitmap(const BitmapType& bmp) {
    static const char *col_table = " .,-~;*+!=1%O@$#";
    using gsc4 = pixel<channel_traits<channel_name::L,4>>;
    for(int y = 0;y<bmp.dimensions().height;++y) {
        for(int x = 0;x<bmp.dimensions().width;++x) {
            typename BitmapType::pixel_type px;
            bmp.point(point16(x,y),&px);
            const auto px2 = convert<gsc4,typename BitmapType::pixelType>(px);
            size_t i =px2.template channel<0>();
            printf("%c",col_table[i]);
            
        }
        printf("\r\n");
    }
}

int main(int argc, char** argv) {

    // our type definitions
    // this makes things easier
    using bmp_type = bitmap<gsc_pixel<1>>;
    using color = color<typename bmp_type::pixel_type>;

    const font& f = Bm437_Acer_VGA_8x8_FON;
    const char* text = "ESP32 GFX Test";
    ssize16 tsz = f.measure_text(ssize16(strlen(text)*f.average_width(),f.height()),text);
    bmp_type bmp((size16)tsz,malloc(bmp_type::sizeof_buffer((size16)tsz)));
    // draw stuff
    bmp.clear(bmp.bounds()); // comment this out and check out the uninitialized RAM. It looks neat.

    // now draw
    draw::text(bmp,tsz.bounds(),text,f,color::white);

    // display our mess
    dump_bitmap(bmp);
    printf("\r\n");
    size16 bmp2_size(f.average_width(),f.height());
    bmp_type bmp2(bmp2_size,malloc(bmp_type::sizeof_buffer(bmp2_size)));
    bmp2.clear(bmp2.bounds());
    draw::bitmap(bmp2,(srect16)bmp2.bounds(),bmp,rect16(point16(4,0),bmp2.dimensions()));
    // display our mess
    dump_bitmap(bmp2);
    return 0;
}