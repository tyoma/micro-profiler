#pragma once

namespace micro_profiler
{
	class noncopyable
	{
		noncopyable(const noncopyable &other);
		const noncopyable &operator =(const noncopyable &rhs);

	protected:
		noncopyable() {	}
	};
}
