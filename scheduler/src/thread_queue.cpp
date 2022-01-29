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

#include <scheduler/thread_queue.h>

using namespace std;

namespace scheduler
{
	thread_queue::thread_queue(const clock &clock_)
		: _underlying(clock_), _stop_requested(false), _thread([this] {	run();	})
	{	}

	thread_queue::~thread_queue()
	{
		_underlying.schedule([this] {	_stop_requested = true;	});
		_thread.join();
	}

	void thread_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	_underlying.schedule(move(task), defer_by);	}

	void thread_queue::run()
	{
		while (!_stop_requested)
		{
			_underlying.wait();
			_underlying.execute_ready(mt::milliseconds(10));
		}
	}
}
