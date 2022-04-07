
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
using namespace gfx;
// This doesn't work because GFX has dependencies and pio test apparently doesn't support that.

void test_pixel() {
    rgb_pixel<24> px;
    px.channel<0>(255);
    px.channel<1>(255);
    px.channel<2>(255);
    TEST_ASSERT_EQUAL(px.value(), 0xFFFFFF);
    rgb_pixel<16> px2;
    convert(px,&px2);
    TEST_ASSERT_EQUAL(px2.value(), 0xFFFF);
    TEST_ASSERT_EQUAL(px2.channel<0>(),31);
    TEST_ASSERT_EQUAL(px2.channel<1>(),63);
    TEST_ASSERT_EQUAL(px2.channel<2>(),31);
    rgba_pixel<32> px3;
    TEST_ASSERT_EQUAL(px3.channel<3>(),255);
}
int main(int argc, char** argv) {

    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_pixel);
    UNITY_END(); // stop unit testing
}


