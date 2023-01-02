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

#include "profiling_cache.h"

#include <mt/mutex.h>
#include <scheduler/task.h>

namespace micro_profiler
{
	class profiling_cache_sqlite : public profiling_cache, public profiling_cache_tasks
	{
	public:
		profiling_cache_sqlite(const std::string &preferences_db, scheduler::queue &worker);

		static void create_database(const std::string &preferences_db);

		virtual scheduler::task<id_t> persisted_module_id(unsigned int hash) override;

		virtual std::shared_ptr<module_info_metadata> load_metadata(unsigned int hash) override;
		virtual void store_metadata(const module_info_metadata &metadata) override;

		virtual std::vector<tables::cached_patch> load_default_patches(id_t cached_module_id) override;
		virtual void update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
			std::vector<unsigned int> remove_rva) override;

	private:
		std::shared_ptr< scheduler::task_node<id_t> > get_cached_module_id_task(unsigned int hash);
		static void find_module(std::shared_ptr< scheduler::task_node<id_t> > task, const std::string &preferences_db,
			unsigned int hash);

	private:
		mt::mutex _mtx;
		const std::string _preferences_db;
		scheduler::queue &_worker;
		std::unordered_map< unsigned int /*hash*/, std::shared_ptr< scheduler::task_node<id_t> > > _module_mapping_tasks;
	};
}
