#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include "test_common.hpp"
#include <gfx_cpp14.hpp>
// prints a source as 4-bit grayscale ASCII
template <typename Source>
void print_source(const Source& src) {
    static const char *col_table = " .,-~;+=x!1%$O@#";
    using gsc4 = gfx::pixel<gfx::channel_traits<gfx::channel_name::L,4>>;
    for(int y = 0;y<src.dimensions().height;++y) {
        for(int x = 0;x<src.dimensions().width;++x) {
            typename Source::pixel_type px;
            src.point(gfx::point16(x,y),&px);
            const auto px2 = gfx::convert<typename Source::pixel_type,gsc4>(px);
            size_t i =px2.template channel<0>();
            printf("%c",col_table[i]);
        }
        printf("\n");
    }
}
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
        gfx::size16 sz = source.dimensions();
        size_t len = (sz.width*sz.height*Source::pixel_type::bit_depth+7)/8;
        uint8_t* buf = (uint8_t*)malloc(len);
        TEST_ASSERT_NOT_NULL(buf);
        auto bmp = gfx::create_bitmap_from<Source>(source,sz,buf);
        source.copy_to(source.bounds(),bmp,{0,0});
        gfx::point16 pt;
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
template<typename Target>
void test_dimensions(const Target& target,gfx::size16 expected) {
    TEST_ASSERT_EQUAL(target.dimensions().width,expected.width);
    TEST_ASSERT_EQUAL(target.dimensions().height,expected.height);
}
template<typename Target>
void test_bounds(const Target& target,gfx::rect16 expected) {
    gfx::rect16 b = target.bounds();
    TEST_ASSERT_EQUAL(b.x1,expected.x1);
    TEST_ASSERT_EQUAL(b.y1,expected.y1);
    TEST_ASSERT_EQUAL(b.x2,expected.x2);
    TEST_ASSERT_EQUAL(b.y2,expected.y2);
}
void test_fail() {
    TEST_ASSERT_EQUAL(2+2,5);
}