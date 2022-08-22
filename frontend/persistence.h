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

#include "database.h"
#include "serialization.h"

#include <sdb/indexed_serialization.h>

namespace strmd
{
	template <> struct version<micro_profiler::profiling_session> {	enum {	value = 7	};	};
}

namespace micro_profiler
{
	namespace tables
	{
		template <typename S, typename P, int v>
		inline void serialize(strmd::deserializer<S, P, v> &archive, statistics &data, bool address_and_thread)
		{
			scontext::additive dummy;
			auto &index = sdb::unique_index<keyer::callnode>(data);

			archive(index, scontext::nested_context(scontext::root_context(dummy, index, address_and_thread), 0u));
		}
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, profiling_session &data, unsigned int ver)
	{
		sdb::scontext::indexed_by<keyer::external_id, void> as_map;

		archive(data.process_info);

		if (ver >= 6)
			archive(data.mappings);
		else if (ver >= 3)
			archive(data.mappings, as_map);

		if (ver >= 7)
			archive(data.modules);
		else
			archive(data.modules, as_map);

		if (ver >= 5)
			archive(data.statistics);
		else if (ver >= 4)
			archive(data.statistics, static_cast<const bool &>(true));
		else if (ver >= 3)
			archive(data.statistics, static_cast<const bool &>(false));

		if (ver >= 6)
			archive(data.threads);
		else if (ver >= 4)
			archive(data.threads, as_map);

		//if (ver >= 5)
		//	archive(static_cast<containers::unordered_map<unsigned int /*persistent_id*/, tables::image_patches> &>(const_cast<tables::patches &>(*data.patches)));
	}
}
