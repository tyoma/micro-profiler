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

#pragma once

#include <functional>
#include <memory>
#include <mt/chrono.h>

namespace scheduler
{
	struct queue;

	class private_queue
	{
	public:
		explicit private_queue(queue &apartment_queue);
		~private_queue();

		void schedule(std::function<void ()> &&task, mt::milliseconds defer_by = mt::milliseconds(0));

	private:
		struct control_block;

	private:
		private_queue(const private_queue &other);
		void operator =(const private_queue &rhs);

	private:
		queue &_apartment_queue;
		std::shared_ptr<control_block> _control_block;
	};


	class private_worker_queue
	{
	public:
		class completion;

		typedef std::function<void (completion &completion_)> async_task;

	public:
		private_worker_queue(queue &worker_queue, queue &apartment_queue);
		~private_worker_queue();

		void schedule(async_task &&task);

	private:
		struct control_block;

	private:
		private_worker_queue(const private_worker_queue &other);
		void operator =(const private_worker_queue &rhs);

		void deliver(std::function<void ()> &&progress);

	private:
		queue &_worker_queue, &_apartment_queue;
		const std::shared_ptr<control_block> _control_block;

	private:
		friend class completion;
	};

	class private_worker_queue::completion
	{
	public:
		void deliver(std::function<void ()> &&progress);

	private:
		completion(private_worker_queue &owner);

		completion(const completion &other);
		void operator =(const completion &rhs);

	private:
		private_worker_queue &_owner;

	private:
		friend class private_worker_queue;
	};
}
