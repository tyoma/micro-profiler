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

#include "function_list.h"
#include "serialization.h"
#include "symbol_resolver.h"

namespace micro_profiler
{
	template <typename ContextT, typename ArchiveT>
	inline void snapshot_save(ArchiveT &archive, const functions_list &model)
	{
		ContextT context;
		archive(model, context);
	}

	template <typename ContextT, typename ArchiveT>
	inline std::shared_ptr<functions_list> snapshot_load(ArchiveT &archive)
	{
		struct dummy_
		{
			static void dummy_request(unsigned int)
			{	}

			static void dummy_threads_request(const std::vector<unsigned int> &)
			{	}
		};

		ContextT context;
		std::shared_ptr<symbol_resolver> resolver(new symbol_resolver(&dummy_::dummy_request));
		std::shared_ptr<threads_model> threads(new threads_model(&dummy_::dummy_threads_request));
		std::shared_ptr<functions_list> fl(functions_list::create(1, resolver, threads));

		archive(*fl, context);
		return fl;
	}
}
