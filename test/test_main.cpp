
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
static const uint8_t mono_bmp_buf[] = {0x80,0x00,0x00,0x00,
                                       0x00,0x00,0x00,0x00};
using namespace gfx;
template<typename Source, bool CopyTo>
struct copy_to_helper {};
template<typename Source>
struct copy_to_helper<Source,false> {
    static void test_copy_to(const Source& source) {

    }
};
template<typename Source>
struct copy_to_helper<Source,true> {
    static void test_copy_to(const Source& source) {
        size16 sz = source.dimensions();
        size_t len = (sz.width*sz.height*Source::pixel_type::bit_depth+7)/8;
        uint8_t* buf = (uint8_t*)malloc(len);
        TEST_ASSERT_NOT_NULL(buf);
        auto bmp = create_bitmap_from<Source>(source,sz,buf);
        source.copy_to(source.bounds(),bmp,{0,0});
        point16 pt;
        for(pt.y=0;pt.y<source.dimensions().height;++pt.y) {
            for(pt.x=0;pt.x<source.dimensions().width;++pt.x) {
                typename Source::pixel_type spx;
                source.point(pt,&spx);
                typename Source::pixel_type dpx;
                bmp.point(pt,&dpx);
                TEST_ASSERT_EQUAL(spx.value(),dpx.value());
            }
        }
        free(buf);
    }
};

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
void test_bmp_source_point() {
    using bmp_type = bitmap<rgb_pixel<16>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {64,64};
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
void test_const_bmp_source_point() {
    using bmp_type = const_bitmap<gsc_pixel<1>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {8,8};
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

void test_large_bmp_source_destination_point() {
    using bmp_type = large_bitmap<rgb_pixel<16>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {64,64};
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
    using bmp_type = bitmap<rgb_pixel<16>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {64,64};
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
using bmp_type = bitmap<rgb_pixel<16>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {64,64};
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
    using bmp_type = bitmap<rgb_pixel<16>>;
    using bmp_color = color<typename bmp_type::pixel_type>;
    constexpr static const size16 sz = {64,64};
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

int main(int argc, char** argv) {

    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_pixel);
    RUN_TEST(test_bmp_source_point);
    RUN_TEST(test_bmp_source_copy_to);
    RUN_TEST(test_large_bmp_source_destination_point);
    RUN_TEST(test_const_bmp_source_point);
    RUN_TEST(test_bmp_destination_point);
    RUN_TEST(test_bmp_destination_fill);
    // suspend, copy_from, etc are usually driver calls, so they can't be tested here
    
    UNITY_END(); // stop unit testing
}


