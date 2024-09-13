//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

namespace tasker
{
	struct queue;
	template <typename T> class task;
}

namespace micro_profiler
{
	struct profiling_cache;
	struct profiling_cache_tasks;
	struct profiling_session;

	class patch_moderator
	{
	public:
		patch_moderator(std::shared_ptr<profiling_session> session, std::shared_ptr<profiling_cache_tasks> mapping,
			std::shared_ptr<profiling_cache> cache, tasker::queue &worker, tasker::queue &apartment);

	private:
		enum patch_state {	patch_saved, patch_added, patch_removed,	};
		struct cached_patch_command;
		typedef sdb::table<cached_patch_command> changes_log;

	private:
		void restore_and_apply(std::shared_ptr<profiling_cache> cache, std::shared_ptr<tables::patches> patches,
			std::shared_ptr<changes_log> changes, const tables::module_mapping &mapping, profiling_cache_tasks &tasks);
		void persist(std::shared_ptr<profiling_cache> cache, const tables::module_mappings &mappings,
			std::shared_ptr<changes_log> changes, profiling_cache_tasks &tasks);

	private:
		tasker::queue &_worker, &_apartment;
		std::vector<wpl::slot_connection> _connection;
	};
}
