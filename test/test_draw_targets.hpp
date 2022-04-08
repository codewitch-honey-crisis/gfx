#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
static const uint8_t mono_bmp_buf[] = {0x80,0x00,0x00,0x00,
                                       0x00,0x00,0x00,0x00};

void test_bmp_metrics() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    bmp_type bmp(sz,bmp_buf);
    test_dimensions(bmp,sz);
    test_bounds(bmp,gfx::rect16(0,0,sz.width-1,sz.height-1));
}
void test_bmp_source_point() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    constexpr static const size_t len = bmp_type::sizeof_buffer({64,64});
    uint8_t* buf = (uint8_t*)malloc(len);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf,0,len);
    buf[0]=0xFF;
    buf[1]=0xFF;
    bmp_type bmp(sz,buf);
    typename bmp_type::pixel_type px;
    bmp.point({0,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());
    bmp.point({1,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
    bmp.point({0,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
    free(buf);
}
void test_const_bmp_metrics() {
    using bmp_type = gfx::const_bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    bmp_type bmp(sz,mono_bmp_buf);
    test_dimensions(bmp,sz);
    test_bounds(bmp,gfx::rect16(0,0,sz.width-1,sz.height-1));
}
void test_const_bmp_source_point() {
    using bmp_type = gfx::const_bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    bmp_type bmp(sz,mono_bmp_buf);
    typename bmp_type::pixel_type px;
    bmp.point({0,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());
    bmp.point({1,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
    bmp.point({0,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
}
void test_large_bmp_metrics() {
    using bmp_type = gfx::large_bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {8,8};
    bmp_type bmp(sz,1);
    test_dimensions(bmp,sz);
    test_bounds(bmp,gfx::rect16(0,0,sz.width-1,sz.height-1));
}
void test_large_bmp_source_destination_point() {
    using bmp_type = gfx::large_bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    bmp_type bmp(sz,1);
    bmp.clear(bmp.bounds());
    bmp.point({0,0},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({0,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());
    bmp.point({1,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
    bmp.point({0,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());
}

void test_bmp_source_copy_to() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t* buf = (uint8_t*)malloc(len);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf,0,len);
    buf[0]=0xFF;
    buf[1]=0xFF;
    bmp_type bmp(sz,buf);
    copy_to_helper<bmp_type,bmp_type::caps::copy_to>::test_copy_to(bmp);
    free(buf);
}
void test_bmp_destination_point() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t* buf = (uint8_t*)malloc(len);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf,0,len);
    bmp_type bmp(sz,buf);
    bmp.point({1,1},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({1,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    free(buf);
}
void test_bmp_destination_fill() {
    using bmp_type = gfx::bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t* buf = (uint8_t*)malloc(len);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf,0,len);
    bmp_type bmp(sz,buf);
    bmp.fill({1,1,sz.width-2,sz.height-2},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({1,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({1,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());    
    bmp.point({sz.width/2,sz.height/2},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({sz.width-2,sz.height-2},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({sz.width-1,sz.height-1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());    
    free(buf);
}
void test_large_bmp_destination_fill() {
    using bmp_type = gfx::large_bitmap<gfx::rgb_pixel<16>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {64,64};
    bmp_type bmp(sz,1);
    bmp.clear(bmp.bounds());
    bmp.fill({1,1,sz.width-2,sz.height-2},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({1,1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({1,0},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());    
    bmp.point({sz.width/2,sz.height/2},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({sz.width-2,sz.height-2},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::white.value());    
    bmp.point({sz.width-1,sz.height-1},&px);
    TEST_ASSERT_EQUAL(px.value(),bmp_color::black.value());    
}
void test_const_bmp_source_copy_to() {
    using bmp_type = gfx::const_bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    bmp_type bmp(sz,mono_bmp_buf);
    copy_to_helper<bmp_type,bmp_type::caps::copy_to>::test_copy_to(bmp);
}
void test_view_metrics() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<1>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    using view_type =gfx::viewport<bmp_type>;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    bmp_type bmp(sz,bmp_buf);
    view_type view(bmp);
    test_dimensions(view,sz);
    test_bounds(view,gfx::rect16(0,0,sz.width-1,sz.height-1));
}
void test_view_rotate() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<8>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    using view_type =gfx::viewport<bmp_type>;
    constexpr static const gfx::size16 sz = {7,7};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    bmp_type bmp(sz,bmp_buf);
    bmp.clear(bmp.bounds());
    view_type view(bmp);
    view.center({3,3});
    view.rotation(270);
    view.point({0,0},bmp_color::gray);
    view.point({sz.width-1,0},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({0,sz.height-1},&px);
    TEST_ASSERT_EQUAL(bmp_color::gray.value(),px.value());
    bmp.point({0,0},&px);
    TEST_ASSERT_EQUAL(bmp_color::white.value(),px.value());
    bmp.point({sz.width-1,0},&px);
    TEST_ASSERT_EQUAL(bmp_color::black.value(),px.value());
}
void test_view_offset() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<8>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    using view_type =gfx::viewport<bmp_type>;
    constexpr static const gfx::size16 sz = {7,7};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    bmp_type bmp(sz,bmp_buf);
    bmp.clear(bmp.bounds());
    view_type view(bmp);
    view.offset({1,1});
    view.point({0,0},bmp_color::gray);
    view.point({sz.width-2,0},bmp_color::white);
    typename bmp_type::pixel_type px;
    bmp.point({1,1},&px);
    TEST_ASSERT_EQUAL(bmp_color::gray.value(),px.value());
    bmp.point({sz.width-1,1},&px);
    TEST_ASSERT_EQUAL(bmp_color::white.value(),px.value());
    bmp.point({0,0},&px);
    TEST_ASSERT_EQUAL(bmp_color::black.value(),px.value());
}
void test_sprite_metrics() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<8>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    using mask_type = gfx::bitmap<gfx::gsc_pixel<1>>;
    using mask_color = gfx::color<typename mask_type::pixel_type>;
    using sprite_type = gfx::sprite<typename bmp_type::pixel_type>;;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    constexpr static const size_t mask_len = mask_type::sizeof_buffer(sz);
    uint8_t mask_buf[len];
    bmp_type bmp(sz,bmp_buf);
    mask_type mask(sz,mask_buf);
    sprite_type spr(sz,bmp_buf,mask_buf);
    test_dimensions(spr,sz);
    test_bounds(spr,gfx::rect16(0,0,sz.width-1,sz.height-1));   
}
void test_sprite_hit_test() {
    using bmp_type = gfx::bitmap<gfx::gsc_pixel<8>>;
    using bmp_color = gfx::color<typename bmp_type::pixel_type>;
    using mask_type = gfx::bitmap<gfx::gsc_pixel<1>>;
    using mask_color = gfx::color<typename mask_type::pixel_type>;
    using sprite_type = gfx::sprite<typename bmp_type::pixel_type>;;
    constexpr static const gfx::size16 sz = {8,8};
    constexpr static const size_t len = bmp_type::sizeof_buffer(sz);
    uint8_t bmp_buf[len];
    constexpr static const size_t mask_len = mask_type::sizeof_buffer(sz);
    uint8_t mask_buf[len];
    bmp_type bmp(sz,bmp_buf);
    mask_type mask(sz,mask_buf);
    mask.clear(mask.bounds());
    mask.fill(mask.bounds().inflate(-1,-1),mask_color::white);
    sprite_type spr(sz,bmp_buf,mask_buf);
    TEST_ASSERT_FALSE(spr.hit_test({0,0}));
    TEST_ASSERT_TRUE(spr.hit_test({1,1}));
}
