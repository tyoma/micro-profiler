#include <charting/line.h>

#include "helpers.h"

#include <agge/bitmap.h>
#include <agge/blenders_simd.h>
#include <agge/platform/win32/bitmap.h>
#include <agge/renderer.h>
#include <agge/stroke_features.h>

using namespace agge;
using namespace std;
using namespace std::placeholders;

namespace charting
{
	namespace
	{
		template <int precision>
		struct calculate_alpha
		{
			uint8_t operator ()(int area) const
			{
				area >>= precision + 1;
				if (area < 0)
					area = -area;
				if (area > 255)
					area = 255;
				return static_cast<uint8_t>(area);
			}
		};

		class path_adaptor
		{
		public:
			path_adaptor(const shared_ptr<line_xy::model> &model)
				: _model(model), _i(0), _count(model->get_count())
			{	}

			void rewind(unsigned)
			{
				_i = 0;
			}

			int vertex(real_t* x, real_t* y)
			{
				if (_i != _count)
				{
					line_xy::sample p = _model->get_sample(_i);
					*x = p.x, *y = p.y;
					++_i;
					return _i == 1 ? path_command_move_to : path_command_line_to;
				}
				return path_command_stop;
			}

		private:
			shared_ptr<line_xy::model> _model;
			size_t _i, _count;
		};
		
	}


	line_xy::line_xy(const shared_ptr<model> &model)
		: _model(model)
	{
		_stroke.set_cap(caps::butt());
		_stroke.set_join(joins::bevel());
		_stroke.width(2.0f);
		_model_connection = _model->invalidated += bind(&line_xy::on_invalidated, this, _1);
		on_invalidated(0);
	}

	void line_xy::draw(surface &surface, renderer &renderer)
	{
		renderer(surface, 0, _rasterizer, solid_color_brush<simd::blender_solid_color>(255, 255, 255, 255),
			calculate_alpha<_rasterizer._1_shift>());
	}

	void line_xy::rasterize()
	{
		path_adaptor path(_model);
		path_generator_adapter<path_adaptor, agge::stroke> path_stroke(path, _stroke);
		
		_rasterizer.reset();
		add_path(_rasterizer, path_stroke);
		_rasterizer.sort();
	}

	void line_xy::on_invalidated(size_t * /*index*/)
	{
		rasterize();
		invalidate();
	}
}
