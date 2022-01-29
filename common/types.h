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

#pragma once

#include <algorithm> // for std::max/min
#include <cstddef>
#include <mt/chrono.h>
#include <stdexcept>
#include <string>

typedef struct _GUID GUID;

namespace micro_profiler
{
	typedef unsigned char byte;
	typedef unsigned long long int count_t;
	typedef long long timestamp_t;
	typedef unsigned long long int long_address_t;
	typedef unsigned int id_t;

#pragma pack(push, 1)
	struct guid_t
	{
		byte values[16];

		operator const GUID &() const;
	};
#pragma pack(pop)

	struct overhead
	{
		overhead(timestamp_t inner_, timestamp_t outer_);

		timestamp_t inner; // The overhead observed between entry/exit measurements of a leaf function.
		timestamp_t outer; // The overhead observed by a parent function (in addition to inner) when making a call.
	};

	struct initialization_data
	{
		std::string executable;
		timestamp_t ticks_per_second;
		unsigned int injected;
	};

	struct thread_info
	{
		unsigned long long native_id;
		std::string description; // If the platform supports it, contains thread description.
		mt::milliseconds start_time; // Relative to the process start time.
		mt::milliseconds end_time; // Relative to the process start time.
		mt::milliseconds cpu_time;
		bool complete;
	};

	class buffering_policy
	{
	public:
		enum {	buffer_size = 384 /*entries*/,	};

	public:
		buffering_policy(size_t max_allocation, double max_empty_factor, double min_empty_factor);

		size_t max_buffers() const;
		size_t max_empty() const;
		size_t min_empty() const;

	private:
		size_t _max_buffers, _max_empty, _min_empty;
	};



	inline guid_t::operator const GUID &() const
	{	return *reinterpret_cast<const GUID *>(values);	}


	inline overhead::overhead(timestamp_t inner_, timestamp_t outer_)
		: inner(inner_), outer(outer_)
	{	}


	inline buffering_policy::buffering_policy(size_t max_allocation, double max_empty_factor, double min_empty_factor)
		: _max_buffers((std::max<size_t>)(max_allocation / buffer_size + !!(max_allocation % buffer_size), 1u))
	{
		if (max_empty_factor < 0 || max_empty_factor > 1 || min_empty_factor < 0 || min_empty_factor > 1
				|| min_empty_factor > max_empty_factor)
			throw std::invalid_argument("");
		_min_empty = (std::min<size_t>)(static_cast<size_t>(min_empty_factor * _max_buffers), _max_buffers - 1u);
		_max_empty = (std::max<size_t>)(static_cast<size_t>(max_empty_factor * _max_buffers), 1u);
	}
	
	inline size_t buffering_policy::max_buffers() const
	{	return _max_buffers;	}

	inline size_t buffering_policy::max_empty() const
	{	return _max_empty;	}

	inline size_t buffering_policy::min_empty() const
	{	return _min_empty;	}
}
