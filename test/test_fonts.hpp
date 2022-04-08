#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
#include "Bm437_Acer_VGA_8x8.h"
#include "Bm437_Acer_VGA_8x8_FON.h"
#include "Maziro.h"

void test_font_read() {
    Bm437_Acer_VGA_8x8_FON_stream.seek(0);
    gfx::font f;
    TEST_ASSERT_EQUAL((int)gfx::gfx_result::success,(int)gfx::font::read(&Bm437_Acer_VGA_8x8_FON_stream,&f));
    TEST_ASSERT_EQUAL(8,(int) f.height());
    TEST_ASSERT_EQUAL(8,(int) f.average_width());
}
void test_font_measure_text() {
    const gfx::font& f = Bm437_Acer_VGA_8x8_FON;
    const char* test1 = "test\t1234\r\n5678";
    gfx::ssize16 sz1(32767,32767);
    sz1 = f.measure_text(sz1,test1,4);
    TEST_ASSERT_EQUAL(64+(8*4),(int)sz1.width);
    TEST_ASSERT_EQUAL(16,(int)sz1.height);
    const char* test2 = "test1234";
    gfx::ssize16 sz2(32,32767);
    sz2 = f.measure_text(sz2,test2,4);
    TEST_ASSERT_EQUAL(32,(int)sz2.width);
    TEST_ASSERT_EQUAL(16,(int)sz2.height);
}
void test_open_font_open() {
    Maziro_ttf_char_stream.seek(0);
    gfx::open_font of;
    TEST_ASSERT_EQUAL((int)gfx::gfx_result::success,(int)gfx::open_font::open(&Maziro_ttf_char_stream,&of));
    float scale = of.scale(40);
    TEST_ASSERT_GREATER_THAN(0,scale*1000);
}