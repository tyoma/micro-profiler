#include "mocks.h"

#include <test-helpers/helpers.h>
#include <ut/assert.h>

#pragma warning(disable: 4355)

using namespace std;

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


			module_helper::module_helper()
				: on_get_executable([] {	return "";	}), on_locate([] (const void *a) {	return platform().locate(a);	})
			{	}

			void module_helper::emulate_mapped(const image &image_)
			{	emulate_mapped(module::platform().locate(image_.base_ptr()));	}

			void module_helper::emulate_mapped(const mapping &mapping_)
			{
				mt::lock_guard<mt::mutex> l(_mtx);

				_mapped.push_back(mapping_);
				for (auto i = begin(_listeners); i != end(_listeners); ++i)
					(*i)->mapped(mapping_);
			}

			void module_helper::emulate_unmapped(void *base)
			{
				mt::lock_guard<mt::mutex> l(_mtx);
				auto m = find_if(begin(_mapped), end(_mapped), [base] (mapping m) {	return m.base == base;	});

				assert_not_equal(end(_mapped), m);
				_mapped.erase(m);
				for (auto i = begin(_listeners); i != end(_listeners); ++i)
					(*i)->unmapped(base);
			}

			shared_ptr<module::dynamic> module_helper::load(const string &path)
			{	return on_load(path);	}

			string module_helper::executable()
			{	return on_get_executable();	}

			module::mapping module_helper::locate(const void *address)
			{	return on_locate(address);	}

			shared_ptr<module::mapping> module_helper::lock_at(void *address)
			{	return on_lock_at(address);	}

			shared_ptr<void> module_helper::notify(events &consumer)
			{
				mt::lock_guard<mt::mutex> l(_mtx);
				auto i = _listeners.insert(_listeners.end(), &consumer);

				for (auto m = begin(_mapped); m != end(_mapped); ++m)
					consumer.mapped(*m);
				return shared_ptr<void>(*i, [this, i] (void *) {	_listeners.erase(i);	});
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
		}
	}
}
