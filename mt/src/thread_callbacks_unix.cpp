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

#include <mt/thread_callbacks.h>

#include <mt/mutex.h>

#include <list>
#include <pthread.h>
#include <vector>

using namespace std;

namespace mt
{
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


	thread_callbacks &get_thread_callbacks()
	{
		static thread_callbacks_impl callbacks;
		return callbacks;
	}
}
