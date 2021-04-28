#ifndef HTCW_GFX_DEVICE_HPP
#define HTCW_GFX_DEVICE_HPP
namespace gfx {
    struct device {
        enum struct result {
            success = 0,
            not_supported = 1,
            out_of_memory = 2,
            io_error = 3,
            invalid_argument=4
        };
    };
}
#endif