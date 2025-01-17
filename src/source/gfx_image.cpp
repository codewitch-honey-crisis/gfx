#include "gfx_image.hpp"
namespace gfx {
rect16 image::bounds() const {
    return this->dimensions().bounds();
}
}