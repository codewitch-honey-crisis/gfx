#include "test_common.hpp"
#include <gfx_cpp14.hpp>
using namespace gfx;
void test_multiple_linkage() {
    auto rgb = color<rgb_pixel<24>>::alice_blue;
    cmyk_pixel<32> cymk;
    convert(rgb,&cymk);
}