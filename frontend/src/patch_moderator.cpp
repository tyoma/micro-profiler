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

#include <frontend/patch_moderator.h>

#include <frontend/profiling_cache.h>
#include <frontend/keyer.h>
#include <scheduler/task.h>
#include <sdb/integrated_index.h>

using namespace scheduler;
using namespace std;

namespace micro_profiler
{
	using namespace tables;

	struct patch_moderator::cached_patch_command : cached_patch
	{
		patch_state state;
	};

	patch_moderator::patch_moderator(shared_ptr<profiling_session> session,
			shared_ptr<profiling_cache_tasks> db_mapping, shared_ptr<profiling_cache> cache,
			queue &worker, queue &apartment)
		: _worker(worker), _apartment(apartment)
	{
		const auto changes = make_shared<changes_log>();
		auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());
		const auto patches_ = micro_profiler::patches(session);
		const auto mappings_ = micro_profiler::mappings(session);
		auto on_new_mapping = [this, cache, patches_, changes, db_mapping] (tables::module_mappings::const_iterator record) {
			restore_and_apply(cache, patches_, changes, record->module_id, *db_mapping);
		};
		auto on_patch_upsert = [changes, &changes_symbol_idx] (tables::patches::const_iterator p) {
			if (p->state.error | p->state.requested)
				return;

			auto r = changes_symbol_idx[keyer::symbol_id()(*p)];

			if (r.is_new())
			{
				(*r).scope_id = 0; // TODO: support patch-scopes in the future.
				(*r).state = (p->state.active) ? patch_moderator::patch_added : patch_moderator::patch_removed;
			}
			else if (!p->state.active)
			{
				if (patch_moderator::patch_added == (*r).state)
				{
					r.remove();
					return;
				}
				else if (patch_moderator::patch_saved == (*r).state)
				{
					(*r).state = patch_moderator::patch_removed;
				}
			}
			r.commit();
		};

		_connection.push_back(patches_->created += on_patch_upsert);
		_connection.push_back(patches_->modified += on_patch_upsert);
		_connection.push_back(patches_->invalidate += [this, cache, mappings_, changes, db_mapping] {
			persist(cache, *mappings_, changes, *db_mapping);
		});
		_connection.push_back(session->mappings.created += on_new_mapping);

		for (auto i = session->mappings.begin(); i != session->mappings.end(); ++i)
			on_new_mapping(i);
	}

	void patch_moderator::restore_and_apply(shared_ptr<profiling_cache> cache, shared_ptr<tables::patches> patches,
		shared_ptr<changes_log> changes, id_t module_id, profiling_cache_tasks &db_mapping)
	{
		db_mapping.persisted_module_id(module_id)
			.continue_with([cache] (const async_result<id_t> &cached_module_id) {
				return cache->load_default_patches(*cached_module_id);
			}, _worker)
			.continue_with([patches, changes, module_id] (const async_result< vector<cached_patch> > &loaded) {
				vector<unsigned int> rva;
				auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());

				for (auto i = begin(*loaded); i != end(*loaded); ++i)
				{
					auto r = changes_symbol_idx[make_tuple(module_id, i->rva)];

					rva.push_back(i->rva);
					(*r).state = patch_moderator::patch_saved;
					r.commit();
				}
				patches->apply(module_id, range<const unsigned int, size_t>(rva.data(), rva.size()));
			}, _apartment);
	}

	void patch_moderator::persist(shared_ptr<profiling_cache> cache, const tables::module_mappings &mappings,
		shared_ptr<changes_log> changes, profiling_cache_tasks &db_mapping)
	{
		auto &segmented_changes_idx = sdb::multi_index(*changes, keyer::module_id());

		for (auto m = mappings.begin(); m != mappings.end(); ++m)
		{
			const auto module_id = m->module_id;
			const auto added = make_shared< vector<unsigned int> >();
			const auto removed = make_shared< vector<unsigned int> >();

			for (auto p = segmented_changes_idx.equal_range(module_id); p.first != p.second; ++p.first)
				if (patch_added == p.first->state)
					added->push_back(p.first->rva);
				else if (patch_removed == p.first->state)
					removed->push_back(p.first->rva);
			if (added->empty() && removed->empty())
				continue;
			db_mapping.persisted_module_id(module_id)
				.continue_with([cache, added, removed] (const async_result<id_t> &cached_module_id) {
					cache->update_default_patches(*cached_module_id, *added, *removed);
				}, _worker);
		}
		changes->clear();
	}
}
