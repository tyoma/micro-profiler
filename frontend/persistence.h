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

#include "profiling_session.h"
#include "serialization.h"

#include <views/serialization.h>

namespace strmd
{
	template <> struct version<micro_profiler::profiling_session> {	enum {	value = 5	};	};
}

namespace micro_profiler
{
	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, profiling_session &data, unsigned int ver)
	{
		statistic_types::map_detailed s;
		auto read_legacy_statistics = [&data, &s] {
			auto &by_node = views::unique_index<keyer::callnode>(*data.statistics);

			for (auto i = s.begin(); i != s.end(); ++i)
			{
				auto r0 = by_node[call_node_key(i->first.second, 0, i->first.first)];
				const auto parent_id = (*r0).id;

				static_cast<function_statistics &>(*r0) = i->second;
				r0.commit();
				for (auto j = i->second.callees.begin(); j != i->second.callees.end(); ++j)
				{
					auto r1 = by_node[call_node_key(j->first.second, parent_id, j->first.first)];

					static_cast<function_statistics &>(*r1) = j->second;
					r1.commit();
				}
			}
		};

		archive(data.process_info);
		archive(*data.module_mappings);
		archive(*data.modules);

		if (ver >= 5)
		{
			archive(*data.statistics);
		}
		else if (ver >= 4)
		{
			archive(s);
			read_legacy_statistics();
		}
		else if (ver >= 3)
		{
			scontext::detailed_threaded context = {	&s, 0, 0	};

			archive(s, context);
			read_legacy_statistics();
		}

		if (ver >= 4)
			archive(*data.threads);

		//if (ver >= 5)
		//	archive(static_cast<containers::unordered_map<unsigned int /*persistent_id*/, tables::image_patches> &>(const_cast<tables::patches &>(*data.patches)));
	}
}
