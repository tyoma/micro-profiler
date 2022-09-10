#pragma once

#include <common/noncopyable.h>
#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		class process_suspend : noncopyable
		{
		public:
			process_suspend(unsigned pid);
			~process_suspend();

		private:
			std::shared_ptr<void> _hprocess;
		};
	}
}
