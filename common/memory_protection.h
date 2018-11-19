#pragma once

#include "primitives.h"

#include <wpl/base/concepts.h>

namespace micro_profiler
{
	class scoped_unprotect : wpl::noncopyable
	{
	public:
		scoped_unprotect(range<byte> region);
		~scoped_unprotect();

	private:
		void *_address;
		size_t _size;
		unsigned _previous_access;
	};
}
