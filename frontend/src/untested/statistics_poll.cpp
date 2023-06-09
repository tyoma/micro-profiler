//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/statistics_poll.h>

#include <frontend/database.h>

using namespace std;

namespace micro_profiler
{
	statistics_poll::statistics_poll(shared_ptr<const tables::statistics> statistics, scheduler::queue &apartment_queue)
		: _statistics(statistics), _apartment_queue(apartment_queue)
	{	}

	void statistics_poll::enable(bool value)
	{
		if (value == enabled())
			return;
		if (value)
			_invalidation = _statistics->invalidate += [this] {	on_invalidate();	}, on_invalidate();
		else
			_invalidation.reset();
	}

	void statistics_poll::on_invalidate()
	{	_apartment_queue.schedule([this] {	_statistics->request_update();	}, mt::milliseconds(25));	}
}
