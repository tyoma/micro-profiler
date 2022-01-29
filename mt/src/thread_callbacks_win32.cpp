//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <mt/tls.h>

#include <list>
#include <vector>
#include <windows.h>

using namespace std;

namespace mt
{
	class thread_callbacks_impl : public thread_callbacks
	{
	public:
		void notify_thread_exit() throw();

		virtual void at_thread_exit(const atexit_t &handler);

	private:
		typedef vector<atexit_t> destructors_t;

	private:
		mt::mutex _mutex;
		list<destructors_t> _all_destructors;
		mt::tls<destructors_t> _thread_destructors;
	};



	void thread_callbacks_impl::notify_thread_exit() throw()
	{
		if (destructors_t *destructors = _thread_destructors.get())
		{
			while (!destructors->empty())
			{
				atexit_t d(destructors->back());

				destructors->pop_back();
				d();
			}
		}
	}

	void thread_callbacks_impl::at_thread_exit(const atexit_t &handler)
	{
		destructors_t *destructors = _thread_destructors.get();

		if (!destructors)
		{
			mt::lock_guard<mt::mutex> lock(_mutex);

			destructors = &*_all_destructors.insert(_all_destructors.end(), destructors_t());
			_thread_destructors.set(destructors);
		}
		destructors->push_back(handler);
	}


	thread_callbacks_impl &get_thread_callbacks_impl()
	{
		static thread_callbacks_impl callbacks;
		return callbacks;
	}

	thread_callbacks &get_thread_callbacks()
	{	return get_thread_callbacks_impl();	}
}

BOOL WINAPI DllMain(HINSTANCE /*hinstance*/, DWORD reason, LPVOID /*reserved*/)
{
	if (DLL_THREAD_DETACH == reason)
		mt::get_thread_callbacks_impl().notify_thread_exit();
	return TRUE;
}
