#ifndef HTCW_GFX_CORE_HPP
#define HTCW_GFX_CORE_HPP
#include <stdint.h>
namespace gfx {
    struct gfx_caps {
        uint32_t blt_to : 1;
        uint32_t frame_write : 1;
        uint32_t frame_read : 1;
        uint32_t async_frame_read : 1;
        uint32_t async_frame_write : 1;
        uint32_t frame_write_partial : 1;
        uint32_t frame_read_partial : 1;
        uint32_t async_frame_write_partial : 1;
        uint32_t async_frame_read_partial : 1;
    };
    enum struct gfx_result {
        success = 0,
        invalid_argument = 1,
        not_supported = 2,
        device_error = 3,
        out_of_memory = 4
    };
}
#endif