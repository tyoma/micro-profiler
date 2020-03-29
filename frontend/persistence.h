//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "function_list.h"
#include "serialization.h"
#include "symbol_resolver.h"

namespace micro_profiler
{
	template <typename ArchiveT>
	inline void save(ArchiveT &archive, const functions_list &model)
	{
		archive(static_cast<timestamp_t>(1 / model._tick_interval));
		archive(*model.get_resolver());
		archive(*model._statistics);
	}

	template <typename ArchiveT>
	inline std::shared_ptr<functions_list> load_functions_list(ArchiveT &archive)
	{
		struct dummy_
		{
			static void dummy_request(unsigned int)
			{	}
		};

		timestamp_t ticks_per_second;
		std::shared_ptr<symbol_resolver> resolver(new symbol_resolver(&dummy_::dummy_request));

		archive(ticks_per_second);
		archive(*resolver);

		std::shared_ptr<functions_list> fl(functions_list::create(ticks_per_second, resolver, nullptr));

		archive(*fl->_statistics);
		fl->on_updated();
		return fl;
	}
}
