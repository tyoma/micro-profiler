#include "graphics.h"

#include <agge/blenders.h>
#include <agge/blenders_simd.h>
#include <agge/figures.h>
#include <agge/filling_rules.h>
#include <agge/path.h>

using namespace agge;

namespace micro_profiler
{
	color color::make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		color c = { r, g, b, a };
		return c;
	}

	void fill(wpl::ui::gcontext &ctx, wpl::ui::gcontext::rasterizer_ptr &rasterizer, color c)
	{
		typedef blender_solid_color<simd::blender_solid_color, order_bgra> blender_t;

		rect_i rc = ctx.update_area();

		rasterizer->reset();
		add_path(*rasterizer, rectangle((real_t)rc.x1, (real_t)rc.y1, (real_t)rc.x2, (real_t)rc.y2));
		ctx(rasterizer, blender_t(c.r, c.g, c.b, c.a), winding<>());
	}
}
