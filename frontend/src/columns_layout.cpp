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

#pragma warning(disable: 4244)

#include <frontend/columns_layout.h>

#include <common/formatting.h>
#include <frontend/model_context.h>
#include <frontend/primitives.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables.h>

using namespace agge;

namespace micro_profiler
{
	template <typename ResultT>
	inline ResultT initialize()
	{
		ResultT r = {	};
		return r;
	}

	template <typename ResultT, typename Field1T>
	inline ResultT initialize(const Field1T &field1)
	{
		ResultT r = {	field1	};
		return r;
	}

	template <typename T>
	inline bool get_thread_native_id(T &destination, unsigned int id, const tables::threads &threads)
	{
		const auto i = threads.find(id);

		return i != threads.end() ? destination = static_cast<T>(i->second.native_id), true : false;
	}

	struct by_name
	{
		bool operator ()(const statistics_model_context &context, const call_statistics &lhs, const call_statistics &rhs) const
		{	return context.resolver->symbol_name_by_va(lhs.address) < context.resolver->symbol_name_by_va(rhs.address);	}
	};

	struct by_caller_name
	{
		bool operator ()(const statistics_model_context &context, const call_statistics &lhs, const call_statistics &rhs) const
		{
			const auto lhs_parent = context.by_id(lhs.parent_id);
			const auto rhs_parent = context.by_id(rhs.parent_id);

			if (lhs_parent && rhs_parent)
				return context.resolver->symbol_name_by_va(lhs_parent->address) < context.resolver->symbol_name_by_va(rhs_parent->address);
			return lhs_parent < rhs_parent;
		}
	};

	struct by_threadid
	{
		bool operator ()(const statistics_model_context &context, const call_statistics &lhs_, const call_statistics &rhs_) const
		{
			unsigned int lhs = 0u, rhs = 0u;
			const auto lhs_valid = get_thread_native_id(lhs, lhs_.thread_id, *context.threads);
			const auto rhs_valid = get_thread_native_id(rhs, rhs_.thread_id, *context.threads);

			return !lhs_valid && !rhs_valid ? false : !lhs_valid ? true : !rhs_valid ? false : lhs < rhs;
		}
	};

	struct by_times_called
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{	return lhs.times_called < rhs.times_called;	}
	};

	struct by_exclusive_time
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{	return lhs.exclusive_time < rhs.exclusive_time;	}
	};

	struct by_inclusive_time
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{	return lhs.inclusive_time < rhs.inclusive_time;	}
	};

	struct by_avg_exclusive_call_time
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{
			return lhs.times_called && rhs.times_called
				? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called
				: lhs.times_called < rhs.times_called;
		}
	};

	struct by_avg_inclusive_call_time
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{
			return lhs.times_called && rhs.times_called
				? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called
				: lhs.times_called < rhs.times_called;
		}
	};

	struct by_max_reentrance
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{	return lhs.max_reentrance < rhs.max_reentrance;	}
	};

	struct by_max_call_time
	{
		bool operator ()(const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) const
		{	return lhs.max_call_time < rhs.max_call_time;	}
	};


	struct row_
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &, size_t row, const call_statistics &) const
		{	micro_profiler::itoa<10>(text, row);	}
	};

	struct name
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
		{	text << context.resolver->symbol_name_by_va(item.address).c_str();	}
	};

	struct caller_name
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
		{
			if (const auto parent = context.by_id(item.parent_id))
				text << context.resolver->symbol_name_by_va(parent->address).c_str();
		}
	};

	struct thread_native_id
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
		{
			unsigned int native_id;

			if (get_thread_native_id(native_id, item.thread_id, *context.threads))
				itoa<10>(text, native_id);
		}
	};

	struct times_called
	{
		count_t operator ()(const statistics_model_context &, const call_statistics &value) const
		{	return value.times_called;	}
	};

	struct exclusive_time
	{
		double operator ()(const statistics_model_context &context, const call_statistics &value) const
		{	return context.tick_interval * value.exclusive_time;	}
	};

	struct inclusive_time
	{
		double operator ()(const statistics_model_context &context, const call_statistics &value) const
		{	return context.tick_interval * value.inclusive_time;	}
	};

	struct exclusive_time_avg
	{
		double operator ()(const statistics_model_context &context, const call_statistics &value) const
		{	return value.times_called ? context.tick_interval * value.exclusive_time / value.times_called : 0.0;	}
	};

	struct inclusive_time_avg
	{
		double operator ()(const statistics_model_context &context, const call_statistics &value) const
		{	return value.times_called ? context.tick_interval * value.inclusive_time / value.times_called : 0.0;	}
	};

	struct max_reentrance
	{
		count_t operator ()(const statistics_model_context &, const call_statistics &value) const
		{	return value.max_reentrance;	}
	};

	struct max_call_time
	{
		double operator ()(const statistics_model_context &context, const call_statistics &value) const
		{	return context.tick_interval * value.max_call_time;	}
	};

	template <typename U>
	struct format_interval_
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
		{	format_interval(text, underlying(context, item));	}

		U underlying;
	};

	template <typename U>
	struct format_integer_
	{
		void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
		{	itoa<10>(text, underlying(context, item));	}

		U underlying;
	};

	template <typename T>
	inline format_interval_<T> format_interval2()
	{	return initialize< format_interval_<T> >(initialize<T>());	}

	template <typename T>
	inline format_integer_<T> format_integer()
	{	return initialize< format_integer_<T> >(initialize<T>());	}

	const auto secondary = style::height_scale(0.85);

	const column_definition<call_statistics, statistics_model_context> c_statistics_columns[] = {
		{	"Index", "#" + secondary, 28, agge::align_far, row_(),	},
		{	"Function", "Function\n" + secondary + "qualified name", 384, agge::align_near, name(), by_name(), true,	},
		{	"ThreadID", "Thread\n" + secondary + "id", 64, agge::align_far, thread_native_id(), by_threadid(), true,	},
		{	"TimesCalled", "Called\n" + secondary + "times", 64, agge::align_far, format_integer<times_called>(), by_times_called(), false, times_called(),	},
		{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, agge::align_far, format_interval2<exclusive_time>(), by_exclusive_time(), false, exclusive_time(),	},
		{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, agge::align_far, format_interval2<inclusive_time>(), by_inclusive_time(), false, inclusive_time(),	},
		{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, agge::align_far, format_interval2<exclusive_time_avg>(), by_avg_exclusive_call_time(), false, exclusive_time_avg(),	},
		{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, agge::align_far, format_interval2<inclusive_time_avg>(), by_avg_inclusive_call_time(), false, inclusive_time_avg(),	},
		{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, agge::align_far, format_integer<max_reentrance>(), by_max_reentrance(), false, max_reentrance(),	},
		{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, agge::align_far, format_interval2<max_call_time>(), by_max_call_time(), false, max_call_time(),	},
	};

	const column_definition<call_statistics, statistics_model_context> c_caller_statistics_columns[] = {
		c_statistics_columns[0],
		{	"Function", "Calling Function\n" + secondary + "qualified name", 384, agge::align_near, caller_name(), by_caller_name(), true,	},
		c_statistics_columns[2],
		c_statistics_columns[3],
	};

	const column_definition<call_statistics, statistics_model_context> c_callee_statistics_columns[] = {
		c_statistics_columns[0],
		{	"Function", "Called Function\n" + secondary + "qualified name", 384, agge::align_near, name(), by_name(), true,	},
		c_statistics_columns[3],
		c_statistics_columns[4],
		c_statistics_columns[5],
		c_statistics_columns[6],
		c_statistics_columns[7],
		c_statistics_columns[9],
	};
}
