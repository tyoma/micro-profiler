#pragma once

#include "primitives.h"

#include <common/noncopyable.h>

namespace micro_profiler
{
	class scoped_unprotect : noncopyable
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
