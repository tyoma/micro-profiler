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
			thread_monitor::thread_monitor()
				: _next_id(1)
			{	}

			unsigned thread_monitor::get_id(mt::thread::id native_id) const
			{
				mt::lock_guard<mt::mutex> lock(_mtx);
				unordered_map<mt::thread::id, unsigned>::const_iterator i = _ids.find(native_id);

				return i != _ids.end() ? i->second : 0;
			}

			unsigned thread_monitor::get_this_thread_id() const
			{	return get_id(mt::this_thread::get_id());	}

			unsigned int thread_monitor::register_self()
			{
				mt::lock_guard<mt::mutex> lock(_mtx);
				return _ids.insert(make_pair(mt::this_thread::get_id(), _next_id++)).first->second;
			}

			thread_info thread_monitor::get_info(unsigned int /*id*/) const
			{	throw 0;	}


			tracer::tracer()
				: calls_collector(10000, *this)
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
