#pragma once

#include <common/noncopyable.h>
#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		struct com_initialize : noncopyable
		{
			com_initialize();
			~com_initialize();
		};

		class com_event
		{
		public:
			com_event();

			void set();
			void wait();

		private:
			std::shared_ptr<void> _handle;
		};
	}
}
