#pragma once

#include <wpl/layout.h>
#include <wpl/models.h>

namespace wpl
{
	class factory;
}

namespace micro_profiler
{
	class image_patch_model;
	class symbol_resolver;

	class image_patch_ui : public wpl::stack
	{
	public:
		image_patch_ui(const wpl::factory &factory_, std::shared_ptr<image_patch_model> model, symbol_resolver &resolver);

	private:
		std::shared_ptr<image_patch_model> _model;
		std::vector< std::shared_ptr<const wpl::trackable> > _selection;
		std::vector<wpl::slot_connection> _connections;
	};
}
