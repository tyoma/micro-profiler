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

#include <common/noncopyable.h>
#include <scheduler/private_queue.h>
#include <wpl/signal.h>

namespace micro_profiler
{
	namespace tables
	{
		struct statistics;
	}

	class statistics_poll : noncopyable
	{
	public:
		statistics_poll(std::shared_ptr<const tables::statistics> statistics, scheduler::queue &apartment_queue);

		void enable(bool value);
		bool enabled() const throw();

	private:
		void on_invalidate();

	private:
		const std::shared_ptr<const tables::statistics> _statistics;
		scheduler::private_queue _apartment_queue;
		wpl::slot_connection _invalidation;
	};

	inline bool statistics_poll::enabled() const throw()
	{	return !!_invalidation;	}
}
