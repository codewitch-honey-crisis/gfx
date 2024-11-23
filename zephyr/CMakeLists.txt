# Copyright (c) 2023 honey the codewitch
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HTCW_GFX)
zephyr_library()
#include_directories(${ZEPHYR_BASE}/lib/???/htcw_bits)
zephyr_include_directories(../include/)
zephyr_library_include_directories(../include/)
zephyr_interface_library_named(HTCW_GFX)

    # zephyr_library_sources_ifdef(CONFIG_PARAM_DEF, file.cpp)
    zephyr_library_sources(
    ../src/gfx_bitmap.cpp
    ../src/gfx_canvas_path.cpp
    ../src/gfx_canvas.cpp    
    ../src/gfx_draw_common.cpp
    ../src/gfx_encoding.cpp
    ../src/gfx_font.cpp
    ../src/gfx_image.cpp
    ../src/gfx_jpg_image.cpp
    ../src/gfx_math.cpp
    ../src/gfx_palette.cpp
    ../src/gfx_pixel.cpp
    ../src/gfx_png_image.cpp
    ../src/gfx_svg.cpp
    ../src/gfx_tt_font.cpp
    ../src/gfx_tvg.cpp
    ../src/gfx_vector_core.cpp
    ../src/gfx_vlw_font.cpp
    ../src/gfx_win_font.cpp
    ../src/miniz.c
    ../src/plutovg-blend.cpp
    ../src/plutovg-canvas.cpp
    ../src/plutovg-font.cpp
    ../src/plutovg-ft-math.cpp
    ../src/plutovg-ft-raster.cpp
    ../src/plutovg-ft-stroker.cpp
    ../src/plutovg-matrix.cpp
    ../src/plutovg-paint.cpp
    ../src/plutovg-path.cpp
    ../src/pngle.c
    ../src/tjpgd.c
    )
zephyr_library_link_libraries(HTCW_GFX)
target_link_libraries(HTCW_GFX INTERFACE zephyr_interface)

endif()