#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gfx_cpp14.hpp>

using namespace gfx;
using palette_type = ega_palette<rgb_pixel<16>>;
int main(int argc, char** argv) {
    palette_type palette;
    printf("done\r\n");
    ega_palette<rgb_pixel<16>> ega_pal;
    typename palette_type::pixel_type plan[64];
    helpers::dither_prepare(&ega_pal);
    helpers::dither_mixing_plan(&ega_pal,color<rgb_pixel<16>>::beige,plan);
    helpers::dither_unprepare();    
    for(int y=0;y<8;++y) {
        for(int x=0;x<8;++x) {
            printf("%X ",plan[y*8+x].channel<0>());
        }
        printf("\r\n");
    }

    return 0;
}
