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

#include <common/formatting.h>
#include <frontend/tables.h>

namespace micro_profiler
{
	template <typename ResultT, typename T>
	inline ResultT initialize(const T &value)
	{
		ResultT r = {	value	};
		return r;
	}

	template <typename T>
	inline bool get_thread_native_id(T &destination, unsigned int id, const tables::threads &threads)
	{
		const auto i = threads.find(id);

		return i != threads.end() ? destination = static_cast<T>(i->second.native_id), true : false;
	}

	template <typename ResolverT>
	struct by_name
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return resolver(lhs) < resolver(rhs);	}

		ResolverT resolver;
	};

	struct by_threadid
	{
		template <typename T>
		bool operator ()(const T &lhs_, const T &rhs_) const
		{
			unsigned int lhs = 0u, rhs = 0u;
			const auto lhs_valid = get_thread_native_id(lhs, lhs_.thread_id, *threads);
			const auto rhs_valid = get_thread_native_id(rhs, rhs_.thread_id, *threads);

			return !lhs_valid && !rhs_valid ? false : !lhs_valid ? true : !rhs_valid ? false : lhs < rhs;
		}

		std::shared_ptr<const tables::threads> threads;
	};

	struct by_times_called
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.times_called < rhs.times_called;	}
	};

	struct by_times_called_parents
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.second < rhs.second;	}
	};

	struct by_exclusive_time
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.exclusive_time < rhs.exclusive_time;	}
	};

	struct by_inclusive_time
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.inclusive_time < rhs.inclusive_time;	}
	};

	struct by_avg_exclusive_call_time
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{
			return lhs.times_called && rhs.times_called
				? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called
				: lhs.times_called < rhs.times_called;
		}
	};

	struct by_avg_inclusive_call_time
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{
			return lhs.times_called && rhs.times_called
				? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called
				: lhs.times_called < rhs.times_called;
		}
	};

	struct by_max_reentrance
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.max_reentrance < rhs.max_reentrance;	}
	};

	struct by_max_call_time
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.max_call_time < rhs.max_call_time;	}
	};


	struct thread_native_id
	{
		void operator ()(agge::richtext_t &text, size_t /*row*/, const call_statistics &item) const
		{
			unsigned int native_id;

			if (get_thread_native_id(native_id, item.thread_id, *threads))
				itoa<10>(text, native_id);
		}

		std::shared_ptr<const tables::threads> threads;
	};

	struct get_times_called
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return static_cast<double>(value.times_called);	}
	};

	struct exclusive_time
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return tick_interval * value.exclusive_time;	}

		double tick_interval;
	};

	struct inclusive_time
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return tick_interval * value.inclusive_time;	}

		double tick_interval;
	};

	struct exclusive_time_avg
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return value.times_called ? tick_interval * value.exclusive_time / value.times_called : 0.0;	}

		double tick_interval;
	};

	struct inclusive_time_avg
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return value.times_called ? tick_interval * value.inclusive_time / value.times_called : 0.0;	}

		double tick_interval;
	};

	struct max_call_time
	{
		template <typename T>
		double operator ()(const T &value) const
		{	return tick_interval * value.max_call_time;	}

		double tick_interval;
	};

	template <typename U>
	struct format_interval_
	{
		template <typename T>
		void operator ()(agge::richtext_t &text, size_t /*row*/, const T &item) const
		{	format_interval(text, underlying(item));	}

		U underlying;
	};

	template <typename T>
	inline format_interval_<T> format_interval2(double tick_interval)
	{	return initialize< format_interval_<T> >(initialize<T>(tick_interval));	}
}
