# Copyright (c) 2023 honey the codewitch
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HTCW_GFX)
zephyr_library()
#include_directories(${ZEPHYR_BASE}/lib/???/htcw_bits)
zephyr_include_directories(../src)
zephyr_library_include_directories(../src)
zephyr_interface_library_named(HTCW_GFX)

    # zephyr_library_sources_ifdef(CONFIG_PARAM_DEF, file.cpp)
    zephyr_library_sources(
    ../src/source/gfx_bitmap.cpp
    ../src/source/gfx_canvas_path.cpp
    ../src/source/gfx_canvas.cpp    
    ../src/source/gfx_draw_common.cpp
    ../src/source/gfx_encoding.cpp
    ../src/source/gfx_font.cpp
    ../src/source/gfx_image.cpp
    ../src/source/gfx_jpg_image.cpp
    ../src/source/gfx_math.cpp
    ../src/source/gfx_palette.cpp
    ../src/source/gfx_pixel.cpp
    ../src/source/gfx_png_image.cpp
    ../src/source/gfx_svg.cpp
    ../src/source/gfx_tt_font.cpp
    ../src/source/gfx_tvg.cpp
    ../src/source/gfx_vector_core.cpp
    ../src/source/gfx_vlw_font.cpp
    ../src/source/gfx_win_font.cpp
    ../src/source/miniz.c
    ../src/source/plutovg-blend.cpp
    ../src/source/plutovg-canvas.cpp
    ../src/source/plutovg-font.cpp
    ../src/source/plutovg-ft-math.cpp
    ../src/source/plutovg-ft-raster.cpp
    ../src/source/plutovg-ft-stroker.cpp
    ../src/source/plutovg-matrix.cpp
    ../src/source/plutovg-paint.cpp
    ../src/source/plutovg-path.cpp
    ../src/source/pngle.c
    ../src/source/tjpgd.c
    )
zephyr_library_link_libraries(HTCW_GFX)
target_link_libraries(HTCW_GFX INTERFACE zephyr_interface)

endif()