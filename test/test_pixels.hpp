#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
void test_pixel_channels() {
    gfx::rgb_pixel<24> px;
    px.channel<0>(255);
    px.channel<1>(255);
    px.channel<2>(255);
    TEST_ASSERT_EQUAL(0xFFFFFF,px.value());
    
}
void test_pixel_convert() {
    gfx::rgb_pixel<24> px;
    px.channel<0>(255);
    px.channel<1>(255);
    px.channel<2>(255);
    gfx::rgb_pixel<16> px2;
    convert(px,&px2);
    TEST_ASSERT_EQUAL(0xFFFF,px2.value());
    TEST_ASSERT_EQUAL(31,px2.channel<0>());
    TEST_ASSERT_EQUAL(63,px2.channel<1>());
    TEST_ASSERT_EQUAL(31,px2.channel<2>());
    gfx::rgba_pixel<32> px3;
    TEST_ASSERT_EQUAL(px3.channel<3>(),255);
    gfx::yuv_pixel<16> yuv;
    convert(yuv,&px2);
    convert(px2,&yuv);
    gfx::rgb_pixel<24> px4;
    gfx::hsv_pixel<24> hsv(true,1,.5,.6);
    convert(hsv,&px4);
    TEST_ASSERT_EQUAL(153,px4.channel<0>());
    TEST_ASSERT_EQUAL(76,px4.channel<1>());
    TEST_ASSERT_EQUAL(76,px4.channel<2>());
    hsv = gfx::hsv_pixel<24>(true,.125,.8,.5);
    convert(hsv,&px4);
    TEST_ASSERT_EQUAL(127,px4.channel<0>());
    TEST_ASSERT_EQUAL(99,px4.channel<1>());
    TEST_ASSERT_EQUAL(25,px4.channel<2>());
    
    convert(px2,&hsv);
    gfx::hsl_pixel<24> hsl(true,.25,.75,.75);
    convert(hsl,&px4);
    convert(px4,&hsl);
    TEST_ASSERT_EQUAL(192,px4.channel<0>());
    TEST_ASSERT_EQUAL(238,px4.channel<1>());
    TEST_ASSERT_EQUAL(143,px4.channel<2>());

    gfx::cmyk_pixel<32> cmyk(true,.3,.4,.2,.1);
    convert(cmyk,&px4);
    convert(px4,&cmyk);
    TEST_ASSERT_EQUAL(161,px4.channel<0>());
    TEST_ASSERT_EQUAL(137,px4.channel<1>());
    TEST_ASSERT_EQUAL(183,px4.channel<2>());
}