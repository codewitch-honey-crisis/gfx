cmake_minimum_required(VERSION 3.24)
project(htcw_gfx VERSION 2.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

include(FetchContent)
FetchContent_Declare(htcw_bits
    GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_bits.git
    GIT_TAG 1.0.15
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS 1.0.15
)

FetchContent_Declare(htcw_data
    GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_data.git
    GIT_TAG 1.4.2
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS 1.4.2
)

FetchContent_MakeAvailable(htcw_data)

FetchContent_Declare(htcw_ml
    GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_ml.git
    GIT_TAG 0.2.3
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS 0.2.3
)

FetchContent_MakeAvailable(htcw_ml)

add_library(htcw_gfx 
    src/gfx_bitmap.cpp
    src/gfx_canvas.cpp
    src/gfx_canvas_path.cpp
    src/gfx_draw_common.cpp
    src/gfx_encoding.cpp
    src/gfx_font.cpp
    src/gfx_image.cpp
    src/gfx_jpg_image.cpp
    src/gfx_math.cpp
    src/gfx_palette.cpp
    src/gfx_pixel.cpp
    src/gfx_png_image.cpp
    src/gfx_svg.cpp
    src/gfx_tvg.cpp
    src/gfx_tt_font.cpp
    src/gfx_vector_core.cpp
    src/gfx_vlw_font.cpp
    src/gfx_win_font.cpp
    src/plutovg-blend.cpp
    src/plutovg-canvas.cpp
    src/plutovg-matrix.cpp
    src/plutovg-paint.cpp
    src/plutovg-path.cpp
    src/plutovg-rasterize.cpp
    src/plutovg-font.cpp
    src/plutovg-ft-math.cpp
    src/plutovg-ft-raster.cpp
    src/plutovg-ft-stroker.cpp
    src/miniz.c
    src/pngle.c
    src/tjpgd.c
)
target_link_libraries(htcw_gfx htcw_bits htcw_io htcw_data htcw_ml)
target_include_directories(htcw_gfx PUBLIC
"${PROJECT_SOURCE_DIR}/include"
"${PROJECT_SOURCE_DIR}/src"
"${PROJECT_BINARY_DIR}")
