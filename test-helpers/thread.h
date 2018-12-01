#pragma once

#include <memory>
#include <mt/thread.h>

namespace micro_profiler
{
	namespace tests
	{
		struct running_thread
		{
			virtual ~running_thread() throw() { }
			virtual void join() = 0;
			virtual mt::thread::id get_id() const = 0;
			virtual bool is_running() const = 0;
			
			virtual void suspend() = 0;
			virtual void resume() = 0;
		};

		namespace this_thread
		{
			std::shared_ptr<running_thread> open();
		};
	}
}
