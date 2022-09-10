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
#include <common/path.h>
#include <cwctype>
#include <explorer/process.h>
#include <frontend/constructors.h>
#include <frontend/database.h>
#include <frontend/helpers.h>
#include <frontend/model_context.h>
#include <frontend/primitives.h>
#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>
#include <utfia/iterator.h>
#include <utfia/string.h>

using namespace agge;

namespace micro_profiler
{
	const auto en_dash = "\xE2\x80\x93";
	const auto secondary = style::height_scale(0.85);
	const auto indent_spaces = "    ";

	struct char_compare
	{
		int operator ()(utfia::utf8::codepoint lhs, utfia::utf8::codepoint rhs) const
		{	return micro_profiler::compare(std::towupper(lhs), std::towupper(rhs));	}
	};

	namespace
	{
		auto by_name = [] (const statistics_model_context &context, const call_statistics &lhs, const call_statistics &rhs) {
			return strcmp(context.resolver->symbol_name_by_va(lhs.address).c_str(), context.resolver->symbol_name_by_va(rhs.address).c_str());
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

		auto by_max_call_time = [] (const statistics_model_context &, const call_statistics &lhs, const call_statistics &rhs) {
			return micro_profiler::compare(lhs.max_call_time, rhs.max_call_time);
		};


		auto row_ = [] (agge::richtext_t &text, const statistics_model_context &, size_t row, const call_statistics &) {
			micro_profiler::itoa<10>(text, row + 1u);
		};

		auto name = [] (agge::richtext_t &text, const statistics_model_context &context, size_t, const call_statistics &item) {
			if (!item.address)
			{
				text << "<root>";
				return;
			}

			auto n = item.path([&context] (micro_profiler::id_t id) {	return context.by_id(id);	}).size() - 1u;

			while (n--)
				text << micro_profiler::indent_spaces;
			text << context.resolver->symbol_name_by_va(item.address).c_str();
		};

		auto thread_native_id = [] (agge::richtext_t &text, const statistics_model_context &context, size_t, const call_statistics &item_) {
			if (micro_profiler::threads_model::cumulative == item_.thread_id)
			{
				text << "[cumulative]";
				return;
			}

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


		auto by_process_name = [] (const process_model_context &, const process_info &lhs, const process_info &rhs) {
			return utfia::compare<utfia::utf8>((micro_profiler::operator*)(lhs.path), (micro_profiler::operator*)(rhs.path),
				micro_profiler::char_compare());
		};

		auto by_process_pid = [] (const process_model_context &, const process_info &lhs, const process_info &rhs) {
			return micro_profiler::compare(lhs.pid, rhs.pid);
		};

		auto by_process_ppid = [] (const process_model_context &, const process_info &lhs, const process_info &rhs) {
			return micro_profiler::compare(lhs.parent_pid, rhs.parent_pid);
		};

		auto by_process_cpu_time = [] (const process_model_context &, const process_info &lhs, const process_info &rhs) {
			return micro_profiler::compare(lhs.cpu_time, rhs.cpu_time);
		};

		auto by_process_cpu_usage = [] (const process_model_context &, const process_info &lhs, const process_info &rhs) {
			return micro_profiler::compare(lhs.cpu_usage, rhs.cpu_usage);
		};

		auto process_name = [] (agge::richtext_t &text, const process_model_context &, size_t, const process_info &item) {
			text << (micro_profiler::operator*)(item.path);
		};

		auto process_pid = [] (agge::richtext_t &text, const process_model_context &, size_t, const process_info &item) {
			micro_profiler::itoa<10>(text, item.pid);
		};

		auto process_ppid = [] (agge::richtext_t &text, const process_model_context &, size_t, const process_info &item) {
			micro_profiler::itoa<10>(text, item.parent_pid);
		};

		auto process_cpu_time = [] (agge::richtext_t &text, const process_model_context &, size_t, const process_info &item) {
			if (item.handle)
				micro_profiler::format_interval(text, 0.001 * item.cpu_time.count());
			else
				text << micro_profiler::en_dash;
		};

		auto process_cpu_usage = [] (agge::richtext_t &text, const process_model_context &, size_t, const process_info &item) {
			if (item.handle)
			{
				char buffer[100];
				const int l = std::sprintf(buffer, "%0.1f", 100.0f * item.cpu_usage);

				text.append(buffer, buffer + l);
			}
			else
			{
				text << micro_profiler::en_dash;
			}
		};
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
		{	"MaxCallTime", "Inclusive\n" + secondary + "maximum/call", 121, agge::align_far, format_interval2(max_call_time), by_max_call_time, false, max_call_time,	},
	};

	const column_definition<call_statistics, statistics_model_context> c_caller_statistics_columns[] = {
		c_statistics_columns[0],
		{	"Function", "Calling Function\n" + secondary + "qualified name", 384, agge::align_near, name, by_name, true,	},
		c_statistics_columns[2],
		c_statistics_columns[3],
		c_statistics_columns[4],
		c_statistics_columns[5],
		c_statistics_columns[6],
		c_statistics_columns[7],
		c_statistics_columns[8],
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
		c_statistics_columns[8],
	};


	const column_definition<process_info, process_model_context> c_processes_columns[] = {
		{	"ProcessExe", "Process\n" + secondary + "executable", 384, agge::align_near, process_name, by_process_name, true,	},
		{	"ProcessID", "PID" + secondary, 50, agge::align_far, process_pid, by_process_pid, true,	},
		{	"ParentProcessID", "PID\n" + secondary + "parent", 50, agge::align_far, process_ppid, by_process_ppid, true,	},
		{	"CPUTime", "CPU\n" + secondary + "time (user)", 50, agge::align_far, process_cpu_time, by_process_cpu_time, false,	},
		{	"CPUUsage", "CPU (%)\n" + secondary + "usage (user)", 50, agge::align_far, process_cpu_usage, by_process_cpu_usage, false,	},
	};
}
