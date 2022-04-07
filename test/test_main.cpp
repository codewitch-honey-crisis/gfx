#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
#include "test_pixels.hpp"
#include "test_draw_targets.hpp"
using namespace gfx;


int main(int argc, char** argv) {

    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_pixel_channels);
    RUN_TEST(test_bmp_metrics);
    RUN_TEST(test_bmp_source_point);
    RUN_TEST(test_bmp_source_copy_to);
    RUN_TEST(test_bmp_destination_point);
    RUN_TEST(test_bmp_destination_fill);
    RUN_TEST(test_const_bmp_metrics);
    RUN_TEST(test_const_bmp_source_point);
    RUN_TEST(test_const_bmp_source_copy_to);
    RUN_TEST(test_large_bmp_metrics);
    RUN_TEST(test_large_bmp_source_destination_point);
    RUN_TEST(test_large_bmp_destination_fill);
    RUN_TEST(test_view_metrics);
    RUN_TEST(test_view_rotate);
    RUN_TEST(test_view_offset);
    RUN_TEST(test_sprite_metrics);
    RUN_TEST(test_sprite_hit_test);
    // suspend, copy_from, etc are usually driver calls, so they can't be tested here
    UNITY_END(); // stop unit testing
}


