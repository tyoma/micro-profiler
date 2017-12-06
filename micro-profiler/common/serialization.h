//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "protocol.h"

#include <strmd/deserializer.h>

namespace micro_profiler
{
	class analyzer;
}

namespace strmd
{
	template <> struct is_arithmetic<wchar_t> { static const bool value = true; };
	template <> struct is_container<micro_profiler::analyzer> { static const bool value = true; };
	template <> struct is_container<micro_profiler::statistics_map_detailed_2> { static const bool value = true; };

	template <> struct container_reader<micro_profiler::statistics_map>
	{
		template <typename ArchiveT>
		void operator()(ArchiveT &archive, size_t count, micro_profiler::statistics_map &data)
		{
			std::pair<const void *, micro_profiler::function_statistics> value;

			while (count--)
			{
				archive(value);
				data[value.first] += value.second;
			}
		}
	};

	template <> struct container_reader<micro_profiler::statistics_map_detailed_2>
	{
		template <typename ArchiveT>
		void operator()(ArchiveT &archive, size_t count, micro_profiler::statistics_map_detailed_2 &data)
		{
			std::pair<const void *, micro_profiler::function_statistics> value;

			while (count--)
			{
				archive(value);
				micro_profiler::function_statistics_detailed &entry = data[value.first];
				entry += value.second;
				if (archive.process_container(entry.callees))
				{
					micro_profiler::update_parent_statistics(data, value.first, entry);
					data.entry_updated(value.first);
				}
			}
		}
	};

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, const void *&data)
	{	archive(reinterpret_cast<uintptr_t &>(data));	}
}

namespace micro_profiler
{
	typedef strmd::varint packer;

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, function_statistics &data)
	{
		archive(data.times_called);
		archive(data.max_reentrance);
		archive(data.inclusive_time);
		archive(data.exclusive_time);
		archive(data.max_call_time);
	}

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, function_statistics_detailed &data)
	{
		archive(static_cast<function_statistics &>(data));
		archive(data.callees);
	}

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, commands &data)
	{	archive(reinterpret_cast<int &>(data));	}
}
