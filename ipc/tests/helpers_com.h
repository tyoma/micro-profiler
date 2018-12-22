#pragma once

#include <common/noncopyable.h>
#include <common/types.h>
#include <functional>
#include <memory>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			typedef std::function<bool (const void *buffer, size_t size)> stream_function_t;

			bool is_factory_registered(const guid_t &id);
			stream_function_t open_stream(const guid_t &id);
		}
	}
}
