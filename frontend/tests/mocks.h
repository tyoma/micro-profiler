#pragma once

#include <common/queue.h>
#include <queue>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class queue : public micro_profiler::queue
			{
			public:
				virtual void post(const task_t &&task, unsigned defer_by = 0);

				void process_all();

			private:
				std::queue<task_t> _tasks;
			};



			void queue::post(const task_t &&task, unsigned /*defer_by*/)
			{	_tasks.emplace(task);	}

			void queue::process_all()
			{
				while (!_tasks.empty())
				{
					_tasks.front()();
					_tasks.pop();
				}
			}
		}
	}
}
