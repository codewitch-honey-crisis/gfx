cmake_minimum_required(VERSION 3.24)
project(htcw_gfx VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(htcw_gfx src/gfx_drawing.cpp
    src/gfx_encoding.cpp
    src/gfx_font.cpp
    src/gfx_image.cpp
    src/gfx_math.cpp
    src/gfx_open_font.cpp
    src/gfx_palette.cpp
    src/gfx_pixel.cpp
    src/gfx_svg_common.cpp
    src/gfx_svg_builder.cpp
    src/gfx_svg_doc.cpp
    src/gfx_svg_parser.cpp
    src/gfx_svg_rasterizer.cpp
    src/gfx_vlw_font.cpp
    src/miniz.c
    src/pngle.c
)
target_link_libraries(htcw_gfx htcw_bits htcw_io htcw_data htcw_ml)
target_include_directories(htcw_gfx PUBLIC
"${PROJECT_SOURCE_DIR}/include"
"${PROJECT_SOURCE_DIR}/src"
"${PROJECT_BINARY_DIR}")
