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

#include <common/types.h>

#include <stdexcept>

using namespace std;

namespace micro_profiler
{
	buffering_policy::buffering_policy(size_t max_allocation, double max_empty_factor, double min_empty_factor)
		: _max_buffers((max<size_t>)(max_allocation / buffer_size + !!(max_allocation % buffer_size), 1u))
	{
		if (max_empty_factor < 0 || max_empty_factor > 1 || min_empty_factor < 0 || min_empty_factor > 1
				|| min_empty_factor > max_empty_factor)
			throw invalid_argument("");
		_min_empty = (min<size_t>)(static_cast<size_t>(min_empty_factor * _max_buffers), _max_buffers - 1u);
		_max_empty = (max<size_t>)(static_cast<size_t>(max_empty_factor * _max_buffers), 1u);
	}
}
