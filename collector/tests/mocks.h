#pragma once

#include <collector/calls_collector.h>
#include <collector/thread_monitor.h>

#include <common/module.h>
#include <common/unordered_map.h>
#include <mt/mutex.h>
#include <mt/thread.h>
#include <mt/thread_callbacks.h>

namespace micro_profiler
{
	namespace tests
	{
		class image;

		namespace mocks
		{
			class module_helper : public module
			{
			public:
				module_helper();

				void emulate_mapped(const image &image_);
				void emulate_mapped(const mapping &mapping_);
				void emulate_unmapped(void *base);

			public:
				std::function<std::shared_ptr<dynamic> (const std::string &path)> on_load;
				std::function<std::string ()> on_get_executable;
				std::function<mapping (const void *address)> on_locate;
				std::function<std::shared_ptr<mapping> (void *address)> on_lock_at;

			private:
				virtual std::shared_ptr<dynamic> load(const std::string &path) override;
				virtual std::string executable() override;
				virtual mapping locate(const void *address) override;
				virtual std::shared_ptr<mapping> lock_at(void *address) override;
				virtual std::shared_ptr<void> notify(events &consumer) override;

			private:
				mt::mutex _mtx;
				std::list<events *> _listeners;
				std::vector<mapping> _mapped;
			};


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
