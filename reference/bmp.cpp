#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include <string.h>
#include <gfx_cpp14.hpp>
//#include "../fonts/Bm437_Acer_VGA_8x8.h"
#include "../fonts/Maziro.h"
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
            const auto px2 = convert<typename BitmapType::pixel_type,gsc4>(px);
            size_t i =px2.template channel<0>();
            printf("%c",col_table[i]);
            
        }
        printf("\r\n");
    }
}

int main(int argc, char** argv) {

    // our type definitions
    // this makes things easier
    using bmp_type = bitmap<gsc_pixel<4>>;

    const open_font& f = Maziro_ttf;
    const char* text = "ESP32 GFX Test";
    const float sc = f.scale(50);
    ssize16 tsz = {50,210};
    bmp_type bmp((size16)tsz,malloc(bmp_type::sizeof_buffer((size16)tsz)));
    // draw stuff
    bmp.clear(bmp.bounds()); // comment this out and check out the uninitialized RAM. It looks neat.
    view<bmp_type> view(bmp);
    view.rotation = 90;    
    view.offset = {45,5};
    //view.center = {((typename spoint16::value_type)(tsz.height/2)),0};
    // now draw
    srect16 sr = view.translate(tsz.bounds());
    sr = sr.clamp_top_left_to_at_least_zero();
    draw::text(view,sr,{0,0},text,f,sc,color_max::white);

    // display our mess
    dump_bitmap(bmp);
    printf("\r\n");
    
    return 0;
}