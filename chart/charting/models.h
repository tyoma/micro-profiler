#pragma once

#include "types.h"

#include <wpl/base/signals.h>

namespace charting
{
	template <typename SampleT>
	struct sequential_model
	{
		typedef SampleT sample;

		virtual size_t get_count() const = 0;
		virtual sample get_sample(size_t i) const = 0;

		wpl::signal<void(size_t *index)> invalidated;
	};
}
