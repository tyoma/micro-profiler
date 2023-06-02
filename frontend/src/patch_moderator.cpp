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

#include <frontend/keyer.h>
#include <frontend/profiling_cache.h>
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
			shared_ptr<profiling_cache_tasks> tasks, shared_ptr<profiling_cache> cache,
			queue &worker, queue &apartment)
		: _worker(worker), _apartment(apartment)
	{
		const auto changes = make_shared<changes_log>();
		auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());
		const auto patches_ = micro_profiler::patches(session);
		const auto mappings_ = micro_profiler::mappings(session);
		auto on_new_mapping = [this, cache, patches_, changes, tasks] (tables::module_mappings::const_iterator m) {
			restore_and_apply(cache, patches_, changes, *m, *tasks);
		};
		auto on_patch_upsert = [changes, &changes_symbol_idx] (tables::patches::const_iterator p) {
			if (is_errored(p->state) || p->in_transit)
				return;

			auto r = changes_symbol_idx[keyer::symbol_id()(*p)];

			if (r.is_new())
			{
				(*r).scope_id = 0; // TODO: support patch-scopes in the future.
				(*r).state = is_active(p->state) ? patch_moderator::patch_added : patch_moderator::patch_removed;
			}
			else if (!is_active(p->state))
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
		_connection.push_back(patches_->invalidate += [this, cache, mappings_, changes, tasks] {
			persist(cache, *mappings_, changes, *tasks);
		});
		_connection.push_back(session->mappings.created += on_new_mapping);

		for (auto i = session->mappings.begin(); i != session->mappings.end(); ++i)
			on_new_mapping(i);
	}

	void patch_moderator::restore_and_apply(shared_ptr<profiling_cache> cache, shared_ptr<tables::patches> patches,
		shared_ptr<changes_log> changes, const tables::module_mapping &mapping, profiling_cache_tasks &tasks)
	{
		tasks.persisted_module_id(mapping.hash)
			.continue_with([cache] (const async_result<id_t> &cached_module_id) {
				return cache->load_default_patches(*cached_module_id);
			}, _worker)
			.continue_with([patches, changes, mapping] (const async_result< vector<cached_patch> > &loaded) {
				vector<unsigned int> rva;
				auto &changes_symbol_idx = sdb::unique_index(*changes, keyer::symbol_id());

				for (auto i = begin(*loaded); i != end(*loaded); ++i)
				{
					auto r = changes_symbol_idx[make_tuple(mapping.module_id, i->rva)];

					rva.push_back(i->rva);
					(*r).state = patch_moderator::patch_saved;
					r.commit();
				}
				patches->apply(mapping.module_id, make_range(rva));
			}, _apartment);
	}

	void patch_moderator::persist(shared_ptr<profiling_cache> cache, const tables::module_mappings &mappings,
		shared_ptr<changes_log> changes, profiling_cache_tasks &tasks)
	{
		auto &segmented_changes_idx = sdb::multi_index(*changes, keyer::module_id());

		for (auto m = mappings.begin(); m != mappings.end(); ++m)
		{
			const auto added = make_shared< vector<unsigned int> >();
			const auto removed = make_shared< vector<unsigned int> >();

			for (auto p = segmented_changes_idx.equal_range(m->module_id); p.first != p.second; ++p.first)
				if (patch_added == p.first->state)
					added->push_back(p.first->rva);
				else if (patch_removed == p.first->state)
					removed->push_back(p.first->rva);
			if (added->empty() && removed->empty())
				continue;
			tasks.persisted_module_id(m->hash)
				.continue_with([cache, added, removed] (const async_result<id_t> &cached_module_id) {
					cache->update_default_patches(*cached_module_id, *added, *removed);
				}, _worker);
		}
		changes->clear();
	}
}
