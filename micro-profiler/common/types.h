#pragma once

#include <functional>

namespace micro_profiler
{
#pragma pack(push, 1)
	struct guid_t
	{
		unsigned char values[16];
	};
#pragma pack(pop)

	typedef std::function<bool(const void *buffer, size_t size)> channel_t;
	typedef std::function<channel_t ()> frontend_factory;
}
