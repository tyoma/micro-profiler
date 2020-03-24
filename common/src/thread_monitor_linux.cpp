//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <common/thread_monitor.h>

#include <list>
#include <memory>
#include <mt/mutex.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		mt::milliseconds get_time(clockid_t clock)
		{
			timespec t = {};

			::clock_gettime(clock, &t);
			return mt::milliseconds(static_cast<unsigned int>(t.tv_sec * 1000 + t.tv_nsec / 1000000));
		}
	}

	class thread_monitor_impl : public thread_monitor, public enable_shared_from_this<thread_monitor_impl>
	{
	public:
		thread_monitor_impl(thread_callbacks &callbacks);

		virtual thread_id register_self();
		virtual thread_info get_info(thread_id id) const;

	private:
		struct thread_info_ex : thread_info
		{
			static void update(thread_info &destination, clockid_t clock_handle_);

			clockid_t clock_handle;
		};

		typedef unordered_map<thread_id, thread_info_ex> threads_map;
		typedef unordered_map<unsigned int, threads_map::value_type *> running_threads_map;

	private:
		static void thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id);

	private:
		thread_callbacks &_callbacks;
		mutable mt::mutex _mutex;
		mutable threads_map _threads;
		running_threads_map _alive_threads;
		thread_id _next_id;
	};

	class thread_callbacks_impl : public thread_callbacks
	{
	public:
		thread_callbacks_impl();
		~thread_callbacks_impl();

		virtual void at_thread_exit(const atexit_t &handler);

	private:
		typedef vector<atexit_t> destructors_t;

	private:
		static void invoke_destructors(void *destructors);

	private:
		mt::mutex _mutex;
		list<destructors_t> _all_destructors;
		pthread_key_t _thread_destructors;
	};



	thread_callbacks_impl::thread_callbacks_impl()
	{
		if (::pthread_key_create(&_thread_destructors, &invoke_destructors))
			throw bad_alloc();
	}

	thread_callbacks_impl::~thread_callbacks_impl()
	{	::pthread_key_delete(_thread_destructors);	}

	void thread_callbacks_impl::at_thread_exit(const atexit_t &handler)
	{
		destructors_t *dtors = static_cast<destructors_t *>(::pthread_getspecific(_thread_destructors));

		if (!dtors)
		{
			mt::lock_guard<mt::mutex> lock(_mutex);

			dtors = &*_all_destructors.insert(_all_destructors.end(), destructors_t());
			::pthread_setspecific(_thread_destructors, dtors);
		}
		dtors->push_back(handler);
	}

	void thread_callbacks_impl::invoke_destructors(void *destructors_)
	{
		destructors_t *destructors = static_cast<destructors_t *>(destructors_);

		while (!destructors->empty())
		{
			atexit_t d(destructors->back());

			destructors->pop_back();
			d();
		}
	}


	thread_monitor_impl::thread_monitor_impl(thread_callbacks &callbacks)
		: _callbacks(callbacks), _next_id(0)
	{	}

	thread_monitor::thread_id thread_monitor_impl::register_self()
	{
		const unsigned int id = syscall(SYS_gettid);
		mt::lock_guard<mt::mutex> lock(_mutex);
		threads_map::value_type *&p = _alive_threads[id];

		if (!p)
		{
			weak_ptr<thread_monitor_impl> wself = shared_from_this();

			p = &*_threads.insert(make_pair(_next_id++, thread_info_ex())).first;
			p->second.native_id = id;
			p->second.clock_handle = clockid_t();
			::pthread_getcpuclockid(pthread_self(), &p->second.clock_handle);
			p->second.end_time = mt::milliseconds(0);
			_callbacks.at_thread_exit(bind(&thread_monitor_impl::thread_exited, wself, id));
		}
		return p->first;
	}

	thread_info thread_monitor_impl::get_info(thread_id id) const
	{
		mt::lock_guard<mt::mutex> lock(_mutex);
		threads_map::iterator i = _threads.find(id);

		if (i == _threads.end())
			throw invalid_argument("Unknown thread id!");

		thread_info ti = i->second;

		if (i->second.clock_handle)
			thread_info_ex::update(ti, i->second.clock_handle);
		return ti;
	}

	void thread_monitor_impl::thread_exited(const weak_ptr<thread_monitor_impl> &wself, unsigned int native_id)
	{
		if (shared_ptr<thread_monitor_impl> self = wself.lock())
		{
			mt::lock_guard<mt::mutex> lock(self->_mutex);
			running_threads_map::iterator i = self->_alive_threads.find(native_id);

			thread_info_ex::update(i->second->second, i->second->second.clock_handle);
			i->second->second.end_time = get_time(CLOCK_MONOTONIC);
			i->second->second.clock_handle = clockid_t();
			self->_alive_threads.erase(i);
		}
	}


	void thread_monitor_impl::thread_info_ex::update(thread_info &destination, clockid_t clock)
	{	destination.cpu_time = get_time(clock);	}


	thread_callbacks &get_thread_callbacks()
	{
		static thread_callbacks_impl callbacks;
		return callbacks;
	}

	shared_ptr<thread_monitor> create_thread_monitor(thread_callbacks &callbacks)
	{	return shared_ptr<thread_monitor>(new thread_monitor_impl(callbacks));	}
}
