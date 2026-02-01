cmake_minimum_required(VERSION 3.24)
if(NOT DEFINED IDF_TARGET)
    project(htcw_gfx VERSION 2.2.16)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")

    include(FetchContent)
    FetchContent_Declare(htcw_bits
        GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_bits.git
        GIT_TAG 1.0.17
        GIT_SHALLOW ON
        FIND_PACKAGE_ARGS 1.0.17
    )

    FetchContent_Declare(htcw_data
        GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_data.git
        GIT_TAG 1.4.5
        GIT_SHALLOW ON
        FIND_PACKAGE_ARGS 1.4.5
    )

    FetchContent_MakeAvailable(htcw_data)

    FetchContent_Declare(htcw_ml
        GIT_REPOSITORY https://github.com/codewitch-honey-crisis/htcw_ml.git
        GIT_TAG 0.2.9
        GIT_SHALLOW ON
        FIND_PACKAGE_ARGS 0.2.9
    )

    FetchContent_MakeAvailable(htcw_ml)

    add_library(htcw_gfx 
        src/source/gfx_bitmap.cpp
        src/source/gfx_canvas.cpp
        src/source/gfx_canvas_path.cpp
        src/source/gfx_draw_common.cpp
        src/source/gfx_encoding.cpp
        src/source/gfx_font.cpp
        src/source/gfx_image.cpp
        src/source/gfx_jpg_image.cpp
        src/source/gfx_math.cpp
        src/source/gfx_palette.cpp
        src/source/gfx_pixel.cpp
        src/source/gfx_png_image.cpp
        src/source/gfx_svg.cpp
        src/source/gfx_tvg.cpp
        src/source/gfx_tt_font.cpp
        src/source/gfx_vector_core.cpp
        src/source/gfx_vlw_font.cpp
        src/source/gfx_win_font.cpp
        src/source/plutovg-blend.cpp
        src/source/plutovg-canvas.cpp
        src/source/plutovg-matrix.cpp
        src/source/plutovg-paint.cpp
        src/source/plutovg-path.cpp
        src/source/plutovg-rasterize.cpp
        src/source/plutovg-font.cpp
        src/source/plutovg-ft-math.cpp
        src/source/plutovg-ft-raster.cpp
        src/source/plutovg-ft-stroker.cpp
        src/source/miniz.c
        src/source/pngle.c
        src/source/tjpgd.c
    )
    target_link_libraries(htcw_gfx htcw_bits htcw_io htcw_data htcw_ml)
    target_include_directories(htcw_gfx PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_BINARY_DIR}")
else()
    idf_component_register(
        SRCS 
            "./src/source/gfx_bitmap.cpp"
            "./src/source/gfx_canvas.cpp"
            "./src/source/gfx_canvas_path.cpp"
            "./src/source/gfx_draw_common.cpp"
            "./src/source/gfx_encoding.cpp"
            "./src/source/gfx_font.cpp"
            "./src/source/gfx_image.cpp"
            "./src/source/gfx_jpg_image.cpp"
            "./src/source/gfx_math.cpp"
            "./src/source/gfx_palette.cpp"
            "./src/source/gfx_pixel.cpp"
            "./src/source/gfx_png_image.cpp"
            "./src/source/gfx_svg.cpp"
            "./src/source/gfx_tvg.cpp"
            "./src/source/gfx_tt_font.cpp"
            "./src/source/gfx_vector_core.cpp"
            "./src/source/gfx_vlw_font.cpp"
            "./src/source/gfx_win_font.cpp"
            "./src/source/plutovg-blend.cpp"
            "./src/source/plutovg-canvas.cpp"
            "./src/source/plutovg-matrix.cpp"
            "./src/source/plutovg-paint.cpp"
            "./src/source/plutovg-path.cpp"
            "./src/source/plutovg-rasterize.cpp"
            "./src/source/plutovg-font.cpp"
            "./src/source/plutovg-ft-math.cpp"
            "./src/source/plutovg-ft-raster.cpp"
            "./src/source/plutovg-ft-stroker.cpp"
            "./src/source/miniz.c"
            "./src/source/pngle.c"
            "./src/source/tjpgd.c"
        INCLUDE_DIRS "." "./src"
        REQUIRES htcw_data htcw_ml
    )

    # Add the compile flag if the option is enabled
    if(CONFIG_HTCW_GFX_NO_SWAP)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC HTCW_GFX_NO_SWAP)
    endif()
endif()