//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "module.h"
#include "image_info.h"
#include "types.h"

#include <vector>

namespace micro_profiler
{
	enum commands {
		init = 0,
		modules_loaded = 1,
		update_statistics = 2,
		modules_unloaded = 3,
		module_metadata = 4,
		request_metadata = 5, // + instance_id
		update_statistics_threaded = 6,
		request_threads_info = 7,
		threads_info = 8,
	};

	// init
	struct initialization_data
	{
		std::string executable;
		timestamp_t ticks_per_second;
	};

	// modules_loaded
	typedef std::vector<mapped_module_identified> loaded_modules;

	// modules_unloaded
	typedef std::vector<unsigned int> unloaded_modules;

	// module_metadata
	struct module_info_metadata
	{
		std::vector<symbol_info> symbols;
		std::vector< std::pair<unsigned int /*file_id*/, std::string /*file*/> > source_files;
	};
}
