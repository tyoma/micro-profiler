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

#include <scheduler/private_queue.h>

#include <mt/mutex.h>
#include <mt/thread.h>
#include <scheduler/scheduler.h>

using namespace std;

namespace scheduler
{
	struct private_queue::control_block
	{
		control_block()
			: alive(true)
		{	}

		mt::recursive_mutex mutex;
		bool alive;
	};

	struct private_worker_queue::control_block
	{
		control_block()
			: alive(true)
		{	}

		mt::mutex mutex;
		bool alive;
	};


	private_queue::private_queue(queue &apartment_queue)
		: _apartment_queue(apartment_queue), _control_block(make_shared<control_block>())
	{	}

	private_queue::~private_queue()
	{
		mt::lock_guard<mt::recursive_mutex> l(_control_block->mutex);
		_control_block->alive = false;
	}

	void private_queue::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{
		auto cb = _control_block;
		function<void ()> captured([task, cb] {
			mt::lock_guard<mt::recursive_mutex> l(cb->mutex);

			if (cb->alive)
				task();
		});

		task = function<void ()>();
		_apartment_queue.schedule(move(captured), defer_by);
	}


	private_worker_queue::private_worker_queue(queue &worker_queue, queue &apartment_queue)
		: _worker_queue(worker_queue), _apartment_queue(apartment_queue), _control_block(make_shared<control_block>())
	{	}

	private_worker_queue::~private_worker_queue()
	{
		mt::lock_guard<mt::mutex> lock(_control_block->mutex);

		_control_block->alive = false;
	}

	void private_worker_queue::schedule(async_task &&task)
	{
		auto cb = _control_block;
		function<void ()> wrapped_task([this, task, cb] {
			mt::lock_guard<mt::mutex> lock(cb->mutex);

			if (cb->alive)
			{
				completion c(*this);

				task(c);
			}
		});

		_worker_queue.schedule(move(wrapped_task));
	}

	void private_worker_queue::deliver(function<void ()> &&progress)
	{
		auto cb = _control_block;
		function<void ()> wrapped_progress([this, progress, cb] {
			if (cb->alive)
				progress();
		});

		_apartment_queue.schedule(move(wrapped_progress));
	}


	private_worker_queue::completion::completion(private_worker_queue &owner)
		: _owner(owner)
	{	}

	void private_worker_queue::completion::deliver(function<void ()> &&progress)
	{	_owner.deliver(move(progress));	}
}
