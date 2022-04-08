#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
#include "image3.h"
void test_image_load() {
    // TODO: make this more comprehensive
    image3_jpg_stream.seek(0);
    int counter = 0;
    gfx::jpeg_image::load(
        &image3_jpg_stream,[](gfx::size16 dimensions, gfx::jpeg_image::region_type& region,gfx::point16 location,void* state){
            *((int*)state)=*((int*)state)+1;
            return gfx::gfx_result::success;
        },
        &counter
    );
    TEST_ASSERT_EQUAL(336,counter);
}