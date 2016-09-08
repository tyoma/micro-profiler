#pragma once

#include <agge/path.h>
#include <agge/types.h>

namespace charting
{
	template <typename BlenderT>
	class solid_color_brush : public BlenderT
	{
	public:
		solid_color_brush(agge::uint8_t r, agge::uint8_t g, agge::uint8_t b, agge::uint8_t a)
			: BlenderT(make_pixel(r, g, b), a)
		{	}

	private:
		typename BlenderT::pixel make_pixel(agge::uint8_t r, agge::uint8_t g, agge::uint8_t b)
		{
			typename BlenderT::pixel p = { b, g, r, 0 };
			return p;
		}
	};


	template <typename LinesSinkT, typename PathT>
	inline void add_path(LinesSinkT &sink, PathT &path)
	{
		using namespace agge;

		real_t x, y;

		path.rewind(0);
		for (int command; command = path.vertex(&x, &y), path_command_stop != command; )
		{
			if (path_command_line_to == (command & path_command_mask))
				sink.line_to(x, y);
			else if (path_command_move_to == (command & path_command_mask))
				sink.move_to(x, y);
			if (command & path_flag_close)
				sink.close_polygon();
		}
	}

}
