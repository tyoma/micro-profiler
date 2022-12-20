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

#include <scheduler/task.h>
#include <sqlite++/database.h>

namespace scheduler
{
	struct queue;
}

namespace micro_profiler
{
	struct profiling_session;

	struct database_mapping_tasks
	{
		virtual scheduler::task<id_t> persisted_module_id(id_t module_id) = 0;
	};

	class profiling_preferences
	{
	public:
		profiling_preferences(const std::string &preferences_db, scheduler::queue &worker, scheduler::queue &apartment);

		void apply_and_track(std::shared_ptr<profiling_session> session, std::shared_ptr<database_mapping_tasks> mapping);

	private:
		static std::vector<patch> load_patches(sql::connection_ptr preferences_db, id_t persistent_id);
		void load_and_apply(std::shared_ptr<tables::patches> patches, id_t module_id,
			scheduler::task<id_t> cached_module_id_task);

	private:
		const std::string _preferences_db;
		scheduler::queue &_worker, &_apartment;
		wpl::slot_connection _module_loaded_connection;
	};
}
