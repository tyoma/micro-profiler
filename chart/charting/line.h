#pragma once

#include "display.h"
#include "models.h"

#include <agge/clipper.h>
#include <agge/rasterizer.h>
#include <agge/stroke.h>

namespace charting
{
	class line_xy : public series
	{
	public:
		struct sample { real_t x; real_t y; };
		typedef sequential_model<sample> model;

	public:
		line_xy(const std::shared_ptr<model> &model);

		virtual void draw(surface &surface, renderer &renderer);

	private:
		void rasterize();
		void on_invalidated(size_t *index);

	private:
		agge::rasterizer< agge::clipper<int> > _rasterizer;
		agge::stroke _stroke;
		std::shared_ptr<model> _model;
		wpl::slot_connection _model_connection;
	};
}
