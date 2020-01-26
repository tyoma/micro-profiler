//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "shadow_stack.h"

namespace micro_profiler
{
	class analyzer : public calls_collector_i::acceptor
	{
	public:
		typedef statistics_map_detailed::const_iterator const_iterator;

	public:
		analyzer(const overhead& overhead_);

		void clear() throw();
		size_t size() const throw();
		const_iterator begin() const throw();
		const_iterator end() const throw();

		virtual void accept_calls(mt::thread::id threadid, const call_record *calls, size_t count);

	private:
		typedef std::unordered_map< mt::thread::id, shadow_stack<statistics_map_detailed> > stacks_container;

	private:
		const analyzer &operator =(const analyzer &rhs);

	private:
		const overhead _overhead;
		statistics_map_detailed _statistics;
		stacks_container _stacks;
	};
}
