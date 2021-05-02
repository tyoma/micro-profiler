#include "mocks.h"

#pragma warning(disable: 4355)

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			namespace
			{
				struct thread_callbacks_stub_ : thread_callbacks
				{
					virtual void at_thread_exit(const atexit_t &/*handler*/) override
					{	}
				} thread_callbacks_stub;
			}

			unsigned thread_callbacks::invoke_destructors()
			{
				unsigned n = 0;

				for (containers::unordered_map< mt::thread::id, vector<atexit_t> >::iterator i = _destructors.begin();
					i != _destructors.end(); ++i)
				{
					while (!i->second.empty())
					{
						atexit_t d = i->second.back();

						i->second.pop_back();
						d();
						n++;
					}
				}
				return n;
			}

			unsigned thread_callbacks::invoke_destructors(mt::thread::id thread_id)
			{
				unsigned n = 0;
				vector<atexit_t> handlers;

				{
					mt::lock_guard<mt::mutex> lock(_mutex);
					handlers.swap(_destructors[thread_id]);
				}

				while (!handlers.empty())
				{
					atexit_t d = handlers.back();

					handlers.pop_back();
					d();
					n++;
				}
				return n;
			}

			void thread_callbacks::at_thread_exit(const atexit_t &handler)
			{
				mt::lock_guard<mt::mutex> lock(_mutex);
				_destructors[mt::this_thread::get_id()].push_back(handler);
			}



			thread_monitor::thread_monitor()
				: micro_profiler::thread_monitor(thread_callbacks_stub), _next_id(1)
			{	}

			thread_monitor::thread_id thread_monitor::get_id(mt::thread::id native_id) const
			{
				mt::lock_guard<mt::mutex> lock(_mtx);
				containers::unordered_map<mt::thread::id, thread_monitor::thread_id>::const_iterator i = _ids.find(native_id);

				return i != _ids.end() ? i->second : 0;
			}

			thread_monitor::thread_id thread_monitor::get_this_thread_id() const
			{	return get_id(mt::this_thread::get_id());	}

			void thread_monitor::add_info(thread_id id, const thread_info &info)
			{	_threads[id] = info;	}

			thread_monitor::thread_id thread_monitor::register_self()
			{
				mt::lock_guard<mt::mutex> lock(_mtx);
				return _ids.insert(make_pair(mt::this_thread::get_id(), _next_id++)).first->second;
			}

			void thread_monitor::update_live_info(thread_info &/*info*/, unsigned int /*native_id*/) const
			{	}


			tracer::tracer()
				: calls_collector(*this, 10000, *this, *this)
			{	}

			void tracer::read_collected(acceptor &a)
			{
				mt::lock_guard<mt::mutex> l(_mutex);

				for (TracesMap::const_iterator i = _traces.begin(); i != _traces.end(); ++i)
				{
					a.accept_calls(i->first, &i->second[0], i->second.size());
				}
				_traces.clear();
			}
		}
	}
}
