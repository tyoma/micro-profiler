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

#include <frontend/keyer.h>
#include <frontend/profiling_preferences_db.h>
#include <scheduler/task.h>
#include <sdb/integrated_index.h>
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

		template <typename T, typename C>
		void write_records(sql::inserter<T> &&inserter, const C &records)
		{
			for (auto i = begin(records); i != end(records); ++i)
				inserter(*i);
		}
	}

	struct profiling_preferences::cached_patch_command : cached_patch
	{
		patch_state command;
	};

	profiling_preferences::profiling_preferences(shared_ptr<profiling_session> session,
			shared_ptr<database_mapping_tasks> db_mapping, const string &preferences_db, queue &worker, queue &apartment)
		: _preferences_db(preferences_db), _worker(worker), _apartment(apartment)
	{
		const auto changes = make_shared<changes_log>();
		auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());
		const auto db = sql::create_connection(_preferences_db.c_str());
		const auto patches_ = micro_profiler::patches(session);
		const auto mappings_ = micro_profiler::mappings(session);
		auto on_new_mapping = [this, db, patches_, changes, db_mapping] (tables::module_mappings::const_iterator record) {
			restore_and_apply(db, patches_, changes, record->module_id, *db_mapping);
		};
		auto on_patch_upsert = [changes, &changes_symbol_idx] (tables::patches::const_iterator p) {
			if (p->state.active & !p->state.error & !p->state.requested)
			{
				auto r = changes_symbol_idx[keyer::symbol_id()(*p)];

				if (r.is_new())
				{
					(*r).scope_id = 0; // TODO: support patch-scopes in the future.
					(*r).command = profiling_preferences::patch_added;
					r.commit();
				}
			}
		};

		_connection.push_back(patches_->created += on_patch_upsert);
		_connection.push_back(patches_->modified += on_patch_upsert);
		_connection.push_back(patches_->invalidate += [this, db, mappings_, changes, db_mapping] {
			persist(db, *mappings_, changes, *db_mapping);
		});
		_connection.push_back(session->mappings.created += on_new_mapping);

		for (auto i = session->mappings.begin(); i != session->mappings.end(); ++i)
			on_new_mapping(i);
	}

	void profiling_preferences::restore_and_apply(sql::connection_ptr db, shared_ptr<tables::patches> patches,
		shared_ptr<changes_log> changes, id_t module_id, database_mapping_tasks &db_mapping)
	{
		db_mapping.persisted_module_id(module_id)
			.continue_with([db] (const async_result<id_t> &cached_module_id) -> vector<cached_patch> {
				sql::transaction t(db);

				return as_vector(t.select<cached_patch>("patches",
					sql::c(&cached_patch::module_id) == sql::p(*cached_module_id)));
			}, _worker)
			.continue_with([patches, changes, module_id] (const async_result< vector<cached_patch> > &loaded) {
				vector<unsigned int> rva;
				auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());

				for (auto i = begin(*loaded); i != end(*loaded); ++i)
				{
					auto r = changes_symbol_idx[make_tuple(module_id, i->rva)];

					rva.push_back(i->rva);
					(*r).command = profiling_preferences::patch_saved;
					r.commit();
				}
				patches->apply(module_id, range<const unsigned int, size_t>(rva.data(), rva.size()));
			}, _apartment);
	}

	void profiling_preferences::persist(sql::connection_ptr db, const tables::module_mappings &mappings,
		shared_ptr<changes_log> changes, database_mapping_tasks &db_mapping)
	{
		auto &segmented_changes_idx = sdb::multi_index(*changes, keyer::module_id());

		for (auto m = mappings.begin(); m != mappings.end(); ++m)
		{
			const auto module_id = m->module_id;
			const auto added_range = segmented_changes_idx.equal_range(module_id);
			const auto added = make_shared< vector<cached_patch> >();

			for (auto p = added_range.first; p != added_range.second; ++p)
				if (patch_added == p->command)
					added->push_back(*p);
			if (added->empty())
				continue;
			db_mapping.persisted_module_id(module_id)
				.continue_with([db, added] (const async_result<id_t> &cached_module_id) {
					sql::transaction t(db);

					for (auto i = begin(*added); i != end(*added); ++i)
						i->module_id = *cached_module_id;
					write_records(t.insert<tables::cached_patch>("patches"), *added);
					t.commit();
				}, _worker);
		}
		changes->clear();
	}
}
