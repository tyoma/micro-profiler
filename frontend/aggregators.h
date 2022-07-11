#pragma once

#include "keyer.h"

namespace micro_profiler
{
	namespace aggregator
	{
		struct void_
		{
			template <typename R, typename I>
			void operator ()(R &, I, I) const
			{	}
		};

		struct sum_flat
		{
			sum_flat(const calls_statistics_table &hierarchy)
				: _by_id(views::unique_index<keyer::id>(hierarchy))
			{	}

			template <typename I>
			void operator ()(call_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated.parent_id = 0;
				static_cast<function_statistics &>(aggregated) = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					add(aggregated, *i, [this] (id_t id) {	return _by_id.find(id);	});
			}

		private:
			const views::immutable_unique_index<calls_statistics_table, keyer::id> &_by_id;
		};
	}
}
