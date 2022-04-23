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

#pragma warning(disable: 4244 4180)

#include <frontend/columns_layout.h>

#include <common/formatting.h>
#include <frontend/constructors.h>
#include <frontend/helpers.h>
#include <frontend/model_context.h>
#include <frontend/primitives.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables.h>

using namespace agge;

namespace micro_profiler
{
	namespace
	{
		const auto secondary = style::height_scale(0.85);

		auto by_name = [] (const statistics_model_context &context, const call_statistics &lhs, const call_statistics &rhs) {
			return strcmp(context.resolver->symbol_name_by_va(lhs.address).c_str(), context.resolver->symbol_name_by_va(rhs.address).c_str());
		};

		auto by_caller_name = [] (const statistics_model_context &context, const call_statistics &lhs, const call_statistics &rhs) -> int {
			const auto lhs_parent = context.by_id(lhs.parent_id);
			const auto rhs_parent = context.by_id(rhs.parent_id);

			if (auto r = compare(lhs_parent, rhs_parent))
				return r;
			return lhs_parent
				? strcmp(context.resolver->symbol_name_by_va(lhs_parent->address).c_str(), context.resolver->symbol_name_by_va(rhs_parent->address).c_str())
				: 0;
		};

		auto by_threadid = [] (const statistics_model_context &context, const call_statistics &lhs_, const call_statistics &rhs_) -> int {
			const auto lhs = context.by_thread_id(lhs_.thread_id);
			const auto rhs = context.by_thread_id(rhs_.thread_id);

			if (auto r = micro_profiler::compare(lhs, rhs))
				return r;
			return lhs ? micro_profiler::compare(lhs->native_id, rhs->native_id) : 0;
		};

		auto by_times_called = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.times_called, rhs.times_called);
		};

		auto by_exclusive_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.exclusive_time, rhs.exclusive_time);
		};

		auto by_inclusive_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.inclusive_time, rhs.inclusive_time);
		};

		auto by_avg_exclusive_call_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.exclusive_time, lhs.times_called, rhs.exclusive_time, rhs.times_called);
		};

		auto by_avg_inclusive_call_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.inclusive_time, lhs.times_called, rhs.inclusive_time, rhs.times_called);
		};

		auto by_max_reentrance = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.max_reentrance, rhs.max_reentrance);
		};

		auto by_max_call_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.max_call_time, rhs.max_call_time);
		};


		auto row_ = [] (agge::richtext_t &text, const statistics_model_context &, size_t row, const call_statistics &) {
			micro_profiler::itoa<10>(text, row + 1u);
		};

		auto name = [] (agge::richtext_t &text, const statistics_model_context &context, size_t, const call_statistics &item) {
			text << context.resolver->symbol_name_by_va(item.address).c_str();
		};

		auto caller_name = [] (agge::richtext_t &text, const statistics_model_context &context, size_t, const call_statistics &item) {
			if (const auto parent = context.by_id(item.parent_id))
				text << context.resolver->symbol_name_by_va(parent->address).c_str();
		};

		auto thread_native_id = [] (agge::richtext_t &text, const statistics_model_context &context, size_t, const call_statistics &item_) {
			const auto item = context.by_thread_id(item_.thread_id);

			if (item)
				micro_profiler::itoa<10>(text, item->native_id);
		};

		auto times_called = [] (const statistics_model_context &, const call_statistics &value) {
			return value.times_called;
		};

		auto exclusive_time = [] (const statistics_model_context &context, const call_statistics &value) {
			return context.tick_interval * value.exclusive_time;
		};

		auto inclusive_time = [] (const statistics_model_context &context, const call_statistics &value) {
			return context.tick_interval * value.inclusive_time;
		};

		auto exclusive_time_avg = [] (const statistics_model_context &context, const call_statistics &value) {
			return value.times_called ? context.tick_interval * value.exclusive_time / value.times_called : 0.0;
		};

		auto inclusive_time_avg = [] (const statistics_model_context &context, const call_statistics &value) {
			return value.times_called ? context.tick_interval * value.inclusive_time / value.times_called : 0.0;
		};

		auto max_reentrance = [] (const statistics_model_context &, const call_statistics &value) {
			return value.max_reentrance;
		};

		auto max_call_time = [] (const statistics_model_context &context, const call_statistics &value) {
			return context.tick_interval * value.max_call_time;
		};

		template <typename U>
		struct format_interval_
		{
			FORCE_INLINE void operator ()(agge::richtext_t &text, const statistics_model_context &context, size_t /*row*/, const call_statistics &item) const
			{
				if (context.canonical)
					canonical(text, context, item);
				else
					format_interval(text, underlying(context, item));
			}

			FORCE_NOINLINE void canonical(agge::richtext_t &text, const statistics_model_context &context, const call_statistics &item) const
			{
				char buffer[100];
				const int l = std::sprintf(buffer, "%g", underlying(context, item));

				if (l > 0)
					text.append(buffer, buffer + l);
			}

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
		inline format_interval_<T> format_interval2(const T &underlying)
		{	return initialize< format_interval_<T> >(underlying);	}

		template <typename T>
		inline format_integer_<T> format_integer(const T &underlying)
		{	return initialize< format_integer_<T> >(underlying);	}
	}


	const column_definition<call_statistics, statistics_model_context> c_statistics_columns[] = {
		{	"Index", "#" + secondary, 28, agge::align_far, row_,	},
		{	"Function", "Function\n" + secondary + "qualified name", 384, agge::align_near, name, by_name, true,	},
		{	"ThreadID", "Thread\n" + secondary + "id", 64, agge::align_far, thread_native_id, by_threadid, true,	},
		{	"TimesCalled", "Called\n" + secondary + "times", 64, agge::align_far, format_integer(times_called), by_times_called, false, times_called,	},
		{	"ExclusiveTime", "Exclusive\n" + secondary + "total", 48, agge::align_far, format_interval2(exclusive_time), by_exclusive_time, false, exclusive_time,	},
		{	"InclusiveTime", "Inclusive\n" + secondary + "total", 48, agge::align_far, format_interval2(inclusive_time), by_inclusive_time, false, inclusive_time,	},
		{	"AvgExclusiveTime", "Exclusive\n" + secondary + "average/call", 48, agge::align_far, format_interval2(exclusive_time_avg), by_avg_exclusive_call_time, false, exclusive_time_avg,	},
		{	"AvgInclusiveTime", "Inclusive\n" + secondary + "average/call", 48, agge::align_far, format_interval2(inclusive_time_avg), by_avg_inclusive_call_time, false, inclusive_time_avg,	},
		{	"MaxRecursion", "Recursion\n" + secondary + "max depth", 25, agge::align_far, format_integer(max_reentrance), by_max_reentrance, false, max_reentrance,	},
		{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, agge::align_far, format_interval2(max_call_time), by_max_call_time, false, max_call_time,	},
	};

	const column_definition<call_statistics, statistics_model_context> c_caller_statistics_columns[] = {
		c_statistics_columns[0],
		{	"Function", "Calling Function\n" + secondary + "qualified name", 384, agge::align_near, caller_name, by_caller_name, true,	},
		c_statistics_columns[2],
		c_statistics_columns[3],
	};

	const column_definition<call_statistics, statistics_model_context> c_callee_statistics_columns[] = {
		c_statistics_columns[0],
		{	"Function", "Called Function\n" + secondary + "qualified name", 384, agge::align_near, name, by_name, true,	},
		c_statistics_columns[2],
		c_statistics_columns[3],
		c_statistics_columns[4],
		c_statistics_columns[5],
		c_statistics_columns[6],
		c_statistics_columns[7],
		c_statistics_columns[9],
	};
}
