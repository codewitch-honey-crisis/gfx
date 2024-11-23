#ifndef HTCW_GFX_DRAW_HPP
#define HTCW_GFX_DRAW_HPP
#include "gfx_draw_common.hpp"
#include "gfx_draw_point.hpp"
#include "gfx_draw_filled_rectangle.hpp"
#include "gfx_draw_line.hpp"
#include "gfx_draw_rectangle.hpp"
#include "gfx_draw_bitmap.hpp"
#include "gfx_draw_icon.hpp"
#include "gfx_draw_text.hpp"
#include "gfx_draw_ellipse.hpp"
#include "gfx_draw_arc.hpp"
#include "gfx_draw_rounded_rectangle.hpp"
#include "gfx_draw_filled_rounded_rectangle.hpp"
#include "gfx_draw_polygon.hpp"
#include "gfx_draw_filled_polygon.hpp"
#include "gfx_draw_sprite.hpp"
#include "gfx_draw_image.hpp"
#include "gfx_draw_canvas.hpp"
namespace gfx {
struct draw : public helpers::xdraw_point, 
            public helpers::xdraw_filled_rectangle, 
            public helpers::xdraw_line, 
            public helpers::xdraw_rectangle,
            public helpers::xdraw_bitmap,
            public helpers::xdraw_icon,
            public helpers::xdraw_text,
            public helpers::xdraw_ellipse,
            public helpers::xdraw_arc,
            public helpers::xdraw_rounded_rectangle,
            public helpers::xdraw_filled_rounded_rectangle,
            public helpers::xdraw_polygon,
            public helpers::xdraw_filled_polygon,
            public helpers::xdraw_sprite,
            public helpers::xdraw_image,
            public helpers::xdraw_canvas
{};
}
#endif