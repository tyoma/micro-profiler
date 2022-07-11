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

namespace strmd
{
	template <> struct version<micro_profiler::profiling_session> {	enum {	value = 6	};	};
}

namespace micro_profiler
{
	namespace views
	{
		template <typename S, typename P, int v, typename T, typename C>
		inline void serialize(strmd::deserializer<S, P, v> &archive, table<tables::record<T>, C> &data, bool)
		{	archive(views::unique_index(data, keyer::external_id()));	}

		template <typename S, typename P, typename T, typename C>
		inline void serialize(strmd::serializer<S, P> &, table<tables::record<T>, C> &, bool)
		{	}
	}

	namespace tables
	{
		template <typename S, typename P, int v>
		inline void serialize(strmd::deserializer<S, P, v> &archive, statistics &data, bool address_and_thread)
		{
			scontext::additive dummy;
			auto &index = views::unique_index<keyer::callnode>(data);

			archive(index, scontext::nested_context(scontext::root_context(dummy, index, address_and_thread), 0u));
		}
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, profiling_session &data, unsigned int ver)
	{
		archive(data.process_info);

		if (ver >= 6)
			archive(data.mappings);
		else if (ver >= 3)
			archive(data.mappings, static_cast<const bool &>(true));

		archive(data.modules);

		if (ver >= 5)
			archive(data.statistics);
		else if (ver >= 4)
			archive(data.statistics, static_cast<const bool &>(true));
		else if (ver >= 3)
			archive(data.statistics, static_cast<const bool &>(false));

		if (ver >= 6)
			archive(data.threads);
		else if (ver >= 4)
			archive(data.threads, static_cast<const bool &>(true));

		//if (ver >= 5)
		//	archive(static_cast<containers::unordered_map<unsigned int /*persistent_id*/, tables::image_patches> &>(const_cast<tables::patches &>(*data.patches)));
	}
}
