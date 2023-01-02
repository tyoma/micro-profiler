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

namespace scheduler
{
	template <typename T>
	class task;
}

namespace micro_profiler
{
	namespace tables
	{
		struct cached_patch;
	}

	struct profiling_cache_tasks
	{
		virtual scheduler::task<id_t> persisted_module_id(unsigned int hash) = 0;
	};

	struct profiling_cache
	{
		virtual std::shared_ptr<module_info_metadata> load_metadata(unsigned int hash) = 0;
		virtual void store_metadata(const module_info_metadata &metadata) = 0;

		virtual std::vector<tables::cached_patch> load_default_patches(id_t cached_module_id) = 0;
		virtual void update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
			std::vector<unsigned int> remove_rva) = 0;
	};
}
