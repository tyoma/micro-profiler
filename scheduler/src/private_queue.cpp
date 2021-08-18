//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <scheduler/private_queue.h>

#include <mt/mutex.h>
#include <scheduler/scheduler.h>

using namespace std;

namespace scheduler
{
	struct private_queue::control_block
	{
		control_block()
			: alive(true)
		{	}

		mt::mutex mutex;
		bool alive;
	};


	private_queue::private_queue(shared_ptr<queue> underlying)
		: _underlying(underlying), _control_block(make_shared<control_block>())
	{	}

	private_queue::~private_queue()
	{
		mt::lock_guard<mt::mutex> l(_control_block->mutex);
		_control_block->alive = false;
	}

	void private_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{
		auto cb = _control_block;
		function<void ()> captured([task, cb] {
			mt::lock_guard<mt::mutex> l(cb->mutex);

			if (cb->alive)
				task();
		});

		task = function<void ()>();
		_underlying->schedule(move(captured), defer_by);
	}
}
