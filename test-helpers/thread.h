#pragma once

#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		struct running_thread
		{
			virtual void join() = 0;
		};

		namespace this_thread
		{
			std::shared_ptr<running_thread> open();
		};
	}
}
