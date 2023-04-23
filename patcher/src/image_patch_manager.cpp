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

#include <patcher/image_patch_manager.h>

#include <common/smart_ptr.h>
#include <patcher/exceptions.h>
#include <sdb/integrated_index.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct module_keyer
		{
			template <typename T> 
			id_t operator ()(const T &record) const
			{	return record.module_id;	}

			template <typename IndexT, typename T>
			void operator ()(const IndexT &, T &record, id_t key) const
			{	record.module_id = key;	}
		};

		struct mapping_keyer
		{
			template <typename T> 
			id_t operator ()(const T &record) const
			{	return record.mapping_id;	}

			template <typename IndexT, typename T>
			void operator ()(const IndexT &, T &record, id_t key) const
			{	record.mapping_id = key;	}
		};

		struct module_rva_keyer
		{
			typedef tuple<id_t /*module_id*/, unsigned int /*rva*/> key_type;

			template <typename T> 
			key_type operator ()(const T &record) const
			{	return make_tuple(record.module_id, record.rva);	}

			template <typename IndexT, typename T>
			void operator ()(const IndexT &, T &record, key_type key) const
			{	record.module_id = get<0>(key), record.rva = get<1>(key);	}
		};



		template <typename T>
		vector< shared_ptr<void> > protect(memory_manager &mm, const T &regions)
		{
			enum {	rwx = protection::read | protection::write | protection::execute	};
			vector< shared_ptr<void> > locks;

			for (auto r = begin(regions); r != end(regions); r++)
				if ((protection::execute & r->protection) && rwx != r->protection)
					locks.push_back(mm.scoped_protect(byte_range(r->address, r->size), rwx, r->protection));
			return locks;
		}
	}

	image_patch_manager::image_patch_manager(patch_factory patch_factory_, mapping_access &mapping_access_,
			memory_manager &memory_manager_)
		: _patch_factory(patch_factory_), _mapping_access(mapping_access_), _memory_manager(memory_manager_),
			_next_id(1), _mapping_subscription(mapping_access_.notify(*this))
	{	}

	image_patch_manager::~image_patch_manager()
	{
		_mapping_subscription.reset();

		auto &patch_idx = sdb::multi_index(_patches, module_keyer());

		for (auto m = _mappings.begin(); m != _mappings.end(); ++m)
			if (const auto locked = lock_module(m->module_id))
				for (auto r = patch_idx.equal_range(m->module_id); r.first != r.second; r.first++)
					if (r.first->patch_)
						r.first->patch_->revert();
	}

	shared_ptr<image_patch_manager::mapping> image_patch_manager::lock_module(id_t module_id)
	{
		mapping_record m;
		auto find_mapping = [&] (mapping_record &match) -> bool {
			// We need to lookup a module record under independent lock, to avoid lock inversion with lock_mapping().
			mt::lock_guard<mt::mutex> l(_mtx);
			const auto pm = sdb::unique_index<module_keyer>(_mappings).find(module_id);

			return pm ? match = *pm, true : false;
		};

		if (find_mapping(m))
			if (const auto l = _mapping_access.lock_mapping(m.mapping_id))
			{
				auto c = make_shared_copy(make_tuple(mapping(*l, m.allocator), l, protect(_memory_manager, l->regions)));

				return make_shared_aspect(c, &get<0>(*c));
			}
		return nullptr;
	}

	void image_patch_manager::query(patch_states &states, id_t module_id)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		auto range = sdb::multi_index(_patches, module_keyer()).equal_range(module_id);

		for (auto i = range.first; i != range.second; ++i)
		{
			patch_state state = {
				i->id, i->rva, i->active ? i->patch_ ? patch_state::active : patch_state::pending : patch_state::dormant,
			};

			states.push_back(state);
		}
	}

	void image_patch_manager::apply(patch_change_results &results, id_t module_id, request_range targets)
	{
		results.reserve(targets.length());

		auto locked = lock_module(module_id);
		mt::lock_guard<mt::mutex> l(_mtx);
		auto &patch_idx = sdb::unique_index(_patches, module_rva_keyer());

		for (auto i = targets.begin(); i != targets.end(); ++i)
		{
			auto patch_record = patch_idx[make_tuple(module_id, *i)]; // postcondition: id, rva, module_id are set.
			auto &p = *patch_record;
			patch_change_result result = {	p.id, p.rva, patch_change_result::error,	};

			// TODO: leave unchanged, if patch is already set.
			if (locked)
				p.patch_ = move(_patch_factory(locked->base + *i, p.id, *locked->allocator));
			p.active = true;
			result.result = patch_change_result::ok;
			patch_record.commit();
			results.push_back(result);
		}
	}

	void image_patch_manager::revert(patch_change_results &results, id_t module_id, request_range targets)
	{
		results.reserve(targets.length());

		auto locked = lock_module(module_id);
		mt::lock_guard<mt::mutex> l(_mtx);
		auto &patch_idx = sdb::unique_index(_patches, module_rva_keyer());

		for (auto i = targets.begin(); i != targets.end(); ++i)
		{
			auto key = make_tuple(module_id, *i);
			patch_change_result result = {	0, *i, patch_change_result::unchanged,	};

			if (patch_idx.find(key))
			{
				auto patch_record = patch_idx[key];
				auto &p = *patch_record;

				if (p.patch_ && locked)
					p.patch_->revert();
				p.patch_.reset();
				p.active = false;
				result.id = p.id;
				result.result = patch_change_result::ok;
				patch_record.commit();
			}
			results.push_back(result);
		}
	}

	void image_patch_manager::mapped(id_t module_id, id_t mapping_id, const module::mapping &mapping)
	{
		auto allocator = _memory_manager.create_executable_allocator(const_byte_range(0, 0), 0); // TODO: pass in proper reference and distance values.
		auto protection_scope = protect(_memory_manager, mapping.regions);
		mt::lock_guard<mt::mutex> l(_mtx);
		auto &patch_idx = sdb::unique_index(_patches, module_rva_keyer());
		auto mapping_record = sdb::unique_index(_mappings, module_keyer())[module_id];

		(*mapping_record).mapping_id = mapping_id;
		(*mapping_record).allocator = allocator;
		mapping_record.commit();
		for (auto r = sdb::multi_index(_patches, module_keyer()).equal_range(module_id); r.first != r.second; r.first++)
		{
			auto patch_record = patch_idx[make_tuple(module_id, r.first->rva)];
			auto &p = *patch_record;

			if (p.active)
				p.patch_ = move(_patch_factory(mapping.base + p.rva, p.id, *allocator));
			patch_record.commit();
		}
	}

	void image_patch_manager::unmapped(id_t mapping_id)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		auto &patch_idx = sdb::unique_index(_patches, module_rva_keyer());
		auto mapping_record = sdb::unique_index(_mappings, mapping_keyer())[mapping_id];
		auto module_id = (*mapping_record).module_id;

		for (auto r = sdb::multi_index(_patches, module_keyer()).equal_range(module_id); r.first != r.second; r.first++)
		{
			auto patch_record = patch_idx[make_tuple(module_id, r.first->rva)];

			(*patch_record).patch_.reset();
			patch_record.commit();
		}
		mapping_record.remove();
	}
}
