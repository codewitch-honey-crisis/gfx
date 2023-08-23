# Copyright (c) 2023 honey the codewitch
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HTCW_GFX)
zephyr_library()
#include_directories(${ZEPHYR_BASE}/lib/???/htcw_bits)
zephyr_include_directories(../include/)
zephyr_library_include_directories(../include/)
zephyr_interface_library_named(HTCW_GFX)
#zephyr_library_sources_ifdef(CONFIG_PARAM_DEF, file.cpp)
zephyr_library_sources(
    ../src/miniz.c
    ../src/pngle.c
    ../src/gfx_drawing.cpp
    ../src/gfx_encoding.cpp
    ../src/gfx_font.cpp
    ../src/gfx_image.cpp
    ../src/gfx_math.cpp
    ../src/gfx_open_font.cpp
    ../src/gfx_palette.cpp
    ../src/gfx_pixel.cpp
    ../src/gfx_svg_doc.cpp
    ../src/gfx_vlw_font.cpp)
zephyr_library_link_libraries(HTCW_GFX)
target_link_libraries(HTCW_GFX INTERFACE zephyr_interface)

endif()