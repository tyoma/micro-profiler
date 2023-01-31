#pragma once

#include <deque>
#include <scheduler/scheduler.h>

namespace scheduler
{
	namespace tests
	{
		namespace mocks
		{
			class queue : public scheduler::queue
			{
			public:
				std::deque< std::pair<std::function<void ()>, mt::milliseconds> > tasks;

				void run_one()
				{
					auto t = std::move(tasks.front().first);

					tasks.pop_front();
					t();
				}

				void run_till_end()
				{
					while (!tasks.empty())
						run_one();
				}

			private:
				virtual void schedule(std::function<void ()> &&task, mt::milliseconds defer_by) override
				{	tasks.emplace_back(std::make_pair(std::forward< std::function<void ()> >(task), defer_by));	}
			};
		}
	}
}
