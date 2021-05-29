#pragma once

#include <collector/calls_collector.h>
#include <collector/thread_monitor.h>

#include <common/unordered_map.h>
#include <mt/mutex.h>
#include <mt/thread.h>
#include <mt/thread_callbacks.h>
#include <test-helpers/mock_frontend.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class thread_callbacks : public mt::thread_callbacks
			{
			public:
				unsigned invoke_destructors();
				unsigned invoke_destructors(mt::thread::id thread_id);

			private:
				virtual void at_thread_exit(const atexit_t &handler) override;

			private:
				mt::mutex _mutex;
				containers::unordered_map< mt::thread::id, std::vector<atexit_t> > _destructors;
			};


			class thread_monitor : public micro_profiler::thread_monitor
			{
			public:
				thread_monitor();

				thread_monitor::thread_id get_id(mt::thread::id tid) const;
				thread_monitor::thread_id get_this_thread_id() const;

				void add_info(thread_id id, const thread_info &info);

			public:
				thread_id provide_this_id;

			private:
				virtual thread_id register_self() override;

			private:
				unsigned _next_id;
				containers::unordered_map<mt::thread::id, thread_monitor::thread_id> _ids;
				mutable mt::mutex _mtx;
			};


			class tracer : public calls_collector_i
			{
			public:
				virtual void read_collected(acceptor &a) override;
				virtual void flush() override;

			public:
				std::function<void (acceptor &a)> on_read_collected;
				std::function<void ()> on_flush;
			};



			inline void tracer::read_collected(acceptor &a)
			{
				if (on_read_collected)
					on_read_collected(a);
			}

			inline void tracer::flush()
			{
				if (on_flush)
					on_flush();
			}
		}
	}
}
