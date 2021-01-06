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

#include <cstddef>
#include <mt/chrono.h>
#include <string>

typedef struct _GUID GUID;

namespace micro_profiler
{
	typedef unsigned char byte;
	typedef unsigned long long int count_t;
	typedef long long timestamp_t;
	typedef unsigned long long int long_address_t;

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

	struct thread_info
	{
		unsigned int native_id;
		std::string description; // If the platform supports it, contains thread description.
		mt::milliseconds start_time; // Relative to the process start time.
		mt::milliseconds end_time; // Relative to the process start time.
		mt::milliseconds cpu_time;
		bool complete;
	};



	inline guid_t::operator const GUID &() const
	{	return *reinterpret_cast<const GUID *>(values);	}


	inline overhead::overhead(timestamp_t inner_, timestamp_t outer_)
		: inner(inner_), outer(outer_)
	{	}
}
