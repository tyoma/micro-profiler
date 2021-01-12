#pragma once

#include <functional>
#include <memory>
#include <mt/chrono.h>

namespace scheduler
{
	namespace tests
	{
		struct message_loop
		{
			virtual void run() = 0;
			virtual void exit() = 0;

			static std::shared_ptr<message_loop> create();
		};

		std::function<mt::milliseconds ()> get_clock();
		std::function<mt::milliseconds ()> create_stopwatch();
	}
}
