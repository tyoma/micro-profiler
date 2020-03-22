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

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	class thread_monitor_impl : public thread_monitor
	{
	public:
		void notify_thread_exit();

		virtual unsigned int register_self();
		virtual thread_info get_info(unsigned int id);
	};



	unsigned int thread_monitor_impl::register_self()
	{	return GetCurrentThreadId();	}

	thread_info thread_monitor_impl::get_info(unsigned int /*id*/)
	{	throw 0;	}


	shared_ptr<thread_monitor> create_thread_monitor()
	{	return shared_ptr<thread_monitor>(new thread_monitor_impl);	}
}
