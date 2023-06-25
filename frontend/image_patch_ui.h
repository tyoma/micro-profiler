#pragma once

#include "primitives.h"

#include <tuple>
#include <unordered_set>
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

	template <typename KeyT>
	class selection;

	namespace tables
	{
		struct patches;
	}


	class image_patch_ui : public wpl::stack
	{
	public:
		image_patch_ui(const wpl::factory &factory_, std::shared_ptr<image_patch_model> model,
			std::shared_ptr<const tables::patches> patches);

	private:
		void check_selection(const selection<symbol_key> &selection_);

	private:
		std::shared_ptr<image_patch_model> _model;
		std::vector<wpl::slot_connection> _connections;
	};
}
