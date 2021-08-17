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

#include "module.h"
#include "image_info.h"
#include "types.h"

#include <patcher/interface.h>
#include <vector>

namespace micro_profiler
{
	enum messages_id {
		// Notifications...
		init = 0,
		legacy_update_statistics = 2,

		// Requests...
		request_update = 0x100, // responded with [modules_loaded, ]statistics_update[, modules_unloaded] sequence.
		response_modules_loaded = 1,
		response_statistics_update = 6,
		response_modules_unloaded = 3,

		request_module_metadata = 5, // + instance_id
		response_module_metadata = 4,

		request_threads_info = 7,
		response_threads_info = 8,

		request_apply_patches = 10,
		response_patched = 11,

		request_revert_patches = 15,
		response_reverted = 16,

		request_query_patches = 20,
		response_patches_state = 21,
	};

	// response_modules_loaded
	typedef std::vector<mapped_module_identified> loaded_modules;

	// response_modules_unloaded
	typedef std::vector<unsigned int> unloaded_modules;

	// response_module_metadata
	struct module_info_metadata
	{
		std::vector<symbol_info> symbols;
		std::vector< std::pair<unsigned int /*file_id*/, std::string /*file*/> > source_files;
	};

	// request_apply_patches, request_revert_patches
	struct patch_request
	{
		unsigned int image_persistent_id;
		std::vector<unsigned int> functions_rva;
	};

	// response_patched
	typedef patch_manager::apply_results response_patched_data;

	// response_reverted
	typedef patch_manager::revert_results response_reverted_data;
}
