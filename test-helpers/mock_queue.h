#pragma once

#include <tasker/scheduler.h>

#include <deque>
#include <ut/assert.h>
#include <utility>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class queue : public tasker::queue
			{
			public:
				virtual void schedule(std::function<void ()> &&task, mt::milliseconds defer_by = mt::milliseconds(0)) override;

				void run_one();
				void run_till_end();

			public:
				std::deque< std::pair<std::function<void ()>, mt::milliseconds > > tasks;
			};



			inline void queue::schedule(std::function<void ()> &&task, mt::milliseconds defer_by)
			{
				tasks.emplace_back(std::make_pair(std::move(task), defer_by));
				task = std::function<void ()>();
			}

			inline void queue::run_one()
			{
				assert_is_false(tasks.empty());

				auto t = std::move(tasks.front());

				tasks.pop_front();
				t.first();
			}

			inline void queue::run_till_end()
			{
				while (!tasks.empty())
					run_one();
			}
		}
	}
}
