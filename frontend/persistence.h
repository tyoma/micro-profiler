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

#include "profiling_session.h"
#include "serialization.h"

namespace strmd
{
	template <> struct version<micro_profiler::profiling_session> {	enum {	value = 4	};	};
}

namespace micro_profiler
{
	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, profiling_session &data, unsigned int ver)
	{
		archive(data.process_info);
		archive(*data.module_mappings);
		archive(*data.modules);

		if (ver >= 4)
		{
			archive(static_cast<statistic_types::map_detailed &>(*data.statistics));
		}
		else if (ver >= 3)
		{
			scontext::detailed_threaded context = { data.statistics.get(), 0, 0 };
			archive(static_cast<statistic_types::map_detailed &>(*data.statistics), context);
		}

		if (ver >= 4)
			archive(*data.threads);

		//if (ver >= 5)
		//	archive(static_cast<containers::unordered_map<unsigned int /*persistent_id*/, tables::image_patches> &>(const_cast<tables::patches &>(*data.patches)));
	}
}
