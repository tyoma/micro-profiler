#pragma once

#include <memory>
#include <mt/thread.h>

namespace micro_profiler
{
	namespace tests
	{
		struct running_thread
		{
			virtual void join() = 0;
			virtual bool join(mt::milliseconds timeout) = 0;
		};

		namespace this_thread
		{
			std::shared_ptr<running_thread> open();
			unsigned int get_native_id();
			mt::milliseconds get_cpu_time();
			bool set_description(const wchar_t *description);
		};
	}
}
