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

#include <frontend/profiling_preferences.h>

#include <frontend/profiling_preferences_db.h>
#include <sqlite++/database.h>

using namespace scheduler;
using namespace std;

namespace micro_profiler
{
	using namespace tables;

	namespace
	{
		template <typename T>
		T copy(const T &from)
		{	return from;	}

		template <typename T>
		vector<T> as_vector(sql::reader<T> &&reader)
		{
			vector<T> read;

			for (T entry; reader(entry); )
				read.push_back(entry);
			return move(read);
		}
	}

	profiling_preferences::profiling_preferences(const string &preferences_db, queue &worker, queue &apartment)
		: _preferences_db(preferences_db), _worker(worker), _apartment(apartment)
	{	}

	void profiling_preferences::apply_and_track(shared_ptr<profiling_session> session,
		shared_ptr<database_mapping_tasks> mapping)
	{
		auto on_new_mapping = [this, session, mapping] (tables::module_mappings::const_iterator record) {
			load_and_apply(micro_profiler::patches(session), record->module_id,
				mapping->persisted_module_id(record->module_id));
		};

		_module_loaded_connection = session->mappings.created += on_new_mapping;
		for (auto i = session->mappings.begin(); i != session->mappings.end(); ++i)
			on_new_mapping(i);
	}

	void profiling_preferences::load_and_apply(shared_ptr<tables::patches> patches, id_t module_id,
		task<id_t> cached_module_id_task)
	{
		const auto db = sql::create_connection(_preferences_db.c_str());

		cached_module_id_task.continue_with([db] (const async_result<id_t> &cached_module_id) -> vector<cached_patch> {
			sql::transaction t(db);

			return as_vector(t.select<cached_patch>("patches", sql::c(&cached_patch::module_id) == sql::p(*cached_module_id)));
		}, _worker).continue_with([patches, module_id] (const async_result< vector<cached_patch> > &loaded) {
			vector<unsigned int> rva;

			for (auto i = begin(*loaded); i != end(*loaded); ++i)
				rva.push_back(i->rva);
			patches->apply(module_id, range<const unsigned int, size_t>(rva.data(), rva.size()));
		}, _apartment);
	}
}
