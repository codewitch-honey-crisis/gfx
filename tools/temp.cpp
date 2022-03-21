#include <gfx_cpp14.hpp>
using namespace gfx;

int main(int argc,char**argv) {
    large_bitmap<rgb_pixel<16>> bmp({16,16},1);
    bmp.clear(bmp.bounds());
    font f;
    file_stream fs("test.fon");
    font::read(&fs,&f);
    draw::text(bmp,(srect16)bmp.bounds(),"A",f,color<decltype(bmp)::pixel_type>::blue);
    return 0;
}
#include "shim.hpp"