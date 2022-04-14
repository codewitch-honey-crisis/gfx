#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
void test_draw_ellipse() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    constexpr static const gfx::size16 bmp_size = {16,16};
    uint8_t* bmp_buf = (uint8_t*)malloc(bmp_type::sizeof_buffer(bmp_size));
    TEST_ASSERT_NOT_NULL(bmp_buf);
    bmp_type bmp(bmp_size,bmp_buf);
    auto border_col = gfx::color<gfx::rgba_pixel<32>>::white;
    border_col.template channelr<3>(.25);
    bmp.clear(bmp.bounds());
    gfx::draw::rectangle(bmp,(gfx::srect16)bmp.bounds(),border_col);
    border_col.template channelr<3>(.5);
    gfx::draw::ellipse(bmp,(gfx::srect16)bmp.bounds().inflate(-1,-1),border_col);
    border_col.template channelr<3>(.75);
    gfx::draw::filled_ellipse(bmp,(gfx::srect16)bmp.bounds().inflate(-4,-4),border_col);
    typename bmp_type::pixel_type cmp=gfx::color<typename bmp_type::pixel_type>::white.blend(gfx::color<typename bmp_type::pixel_type>::black,.5);
    print_source(bmp);
    // TODO: actually test this
    
}
void test_draw_rectangle() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    constexpr static const gfx::size16 bmp_size = {16,16};
    uint8_t* bmp_buf = (uint8_t*)malloc(bmp_type::sizeof_buffer(bmp_size));
    TEST_ASSERT_NOT_NULL(bmp_buf);
    bmp_type bmp(bmp_size,bmp_buf);
    auto border_col = gfx::color<gfx::rgba_pixel<32>>::white;
    border_col.template channelr<3>(.5);
    bmp.clear(bmp.bounds());
    gfx::draw::rectangle(bmp,(gfx::srect16)bmp.bounds(),border_col);
    typename bmp_type::pixel_type cmp=gfx::color<typename bmp_type::pixel_type>::white.blend(gfx::color<typename bmp_type::pixel_type>::black,.5);
    print_source(bmp);
    for(int y = 1;y<bmp_size.height-1;++y) {
        typename bmp_type::pixel_type px;
        bmp.point(gfx::point16(0,y),&px);
        TEST_ASSERT_EQUAL(cmp.value(),px.value());
        for(int x = 1;x<bmp_size.width-1;++x) {
            bmp.point(gfx::point16(x,y),&px);
            TEST_ASSERT_EQUAL(0,px.value());
        }
        bmp.point(gfx::point16(bmp_size.width-1,y),&px);
        TEST_ASSERT_EQUAL(cmp.value(),px.value());
    }
    for(int i = 0;i<2;++i) {
        typename bmp_type::pixel_type px;
        for(int x = 0;x<bmp_size.width-1;++x) {
            bmp.point(gfx::point16(x,i*(bmp_size.height-1)),&px);
            TEST_ASSERT_EQUAL(cmp.value(),px.value());
        }     
    }
    free(bmp_buf);
}
