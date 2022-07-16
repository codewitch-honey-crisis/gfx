#include <stdio.h>
#include <stdlib.h>
#include <unity.h>
#include <gfx_cpp14.hpp>
#include "test_helpers.hpp"
#include "test_pixels.hpp"
#include "test_draw_targets.hpp"
#include "test_images.hpp"
#include "test_fonts.hpp"
#include "test_drawing.hpp"
using namespace gfx;
void setUp() {

}
void tearDown() {
    
}
int main(int argc, char** argv) {
    test_multiple_linkage();
    
    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_pixel_channels);
    RUN_TEST(test_pixel_convert);
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
    
    RUN_TEST(test_image_load);
    
    RUN_TEST(test_font_read);
    RUN_TEST(test_font_measure_text);
    RUN_TEST(test_open_font_open);

    RUN_TEST(test_draw_rectangle);
    RUN_TEST(test_draw_ellipse);
    UNITY_END(); // stop unit testing
}


