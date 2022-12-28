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

#include <sqlite++/database.h>

namespace scheduler
{
	struct queue;
	template <typename T> class task;
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
		profiling_preferences(std::shared_ptr<profiling_session> session, std::shared_ptr<database_mapping_tasks> mapping,
			const std::string &preferences_db, scheduler::queue &worker, scheduler::queue &apartment);

	private:
		enum patch_command {	restored_patch, add_patch, remove_patch,	};
		struct cached_patch_command;
		typedef sdb::table<cached_patch_command> changes_log;

	private:
		void restore_and_apply(sql::connection_ptr db, std::shared_ptr<tables::patches> patches,
			std::shared_ptr<changes_log> changes, id_t module_id, database_mapping_tasks &db_mapping);
		void persist(sql::connection_ptr db, const tables::module_mappings &mappings,
			std::shared_ptr<changes_log> changes, database_mapping_tasks &db_mapping);

	private:
		const std::string _preferences_db;
		scheduler::queue &_worker, &_apartment;
		std::vector<wpl::slot_connection> _connection;
	};
}
