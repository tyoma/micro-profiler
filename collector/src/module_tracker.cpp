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

#include <collector/module_tracker.h>

#include "xxhash32.h"

#include <common/file_stream.h>
#include <common/module.h>
#include <sdb/integrated_index.h>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const auto local_dummy = 0;
	}

	namespace keyer
	{
		struct base
		{
			byte *operator ()(const module::mapping &record) const
			{	return record.base;	}

			template <typename IndexT>
			void operator ()(const IndexT &, module::mapping &record, byte *key) const
			{	record.base = key;	}
		};

		struct id
		{
			template <typename T>
			id_t operator ()(const T &record) const
			{	return record.id;	}
		};

		struct module_id
		{
			id_t operator ()(const module_tracker::mapping &record) const
			{	return record.module_id;	}

			template <typename IndexT>
			void operator ()(const IndexT &, module_tracker::mapping &record, id_t key) const
			{	record.module_id = key;	}
		};

		struct file_id
		{
			micro_profiler::file_id operator ()(const module_tracker::module_info &record) const
			{	return record.file;	}

			template <typename IndexT>
			void operator ()(const IndexT &, module_tracker::module_info &record, const micro_profiler::file_id &key) const
			{	record.file = key;	}
		};
	}


	module_tracker::mapping_history_key::mapping_history_key()
		: last_mapping_id(0), last_unmapped_id(0)
	{	}


	uint32_t module_tracker::calculate_hash(const string &path_)
	{
		enum {	n = 1024	};

		XXHash32 h(0);
		byte buffer[n];
		read_file_stream s(path_);

		for (size_t read = n; read == n; )
		{
			read = s.read_l(buffer, n);
			h.add(buffer, read);
		}
		return h.hash();
	}

	module_tracker::module_tracker(module &module_helper)
		: _module_helper(module_helper), _this_module_file(module_helper.locate(&local_dummy).path),
			_mtx(make_shared<mt::mutex>()), _sinks(make_shared< list<mapping_access::events *> >()),
			_module_notifier(module_helper.notify(*this))
	{	}

	module &module_tracker::helper() const
	{	return _module_helper;	}

	void module_tracker::get_changes(mapping_history_key &key, loaded_modules &mapped_, unloaded_modules &unmapped_) const
	{
		mt::lock_guard<mt::mutex> l(*_mtx);
		auto last_reported_mapping_id = key.last_mapping_id;
		auto last_reported_unmapped_id = key.last_unmapped_id;
		auto &mappings_idx = sdb::ordered_index_(_mappings, keyer::id());
		auto &unmapped_idx = sdb::ordered_index_(_unmapped, keyer::id());
		auto &modules_idx = sdb::unique_index<keyer::id>(_modules);
		auto i = mappings_idx.upper_bound(last_reported_mapping_id);
		auto j = unmapped_idx.upper_bound(last_reported_unmapped_id);

		mapped_.clear();
		unmapped_.clear();
		for (auto e = end(mappings_idx); i != e; last_reported_mapping_id = i->id, i++)
		{
			auto &module = modules_idx[i->module_id];
			module::mapping_ex m = {	module.id, module.path, reinterpret_cast<uintptr_t>(i->base), module.hash	};

			mapped_.push_back(make_pair(i->id, m));
		}
		for (auto e = end(unmapped_idx); j != e; last_reported_unmapped_id = j->id, j++)
			if (j->mapping_id <= key.last_mapping_id)
				unmapped_.push_back(j->mapping_id);

		key.last_mapping_id = last_reported_mapping_id;
		key.last_unmapped_id = last_reported_unmapped_id;
	}

	bool module_tracker::get_module(module_info& info, id_t module_id) const
	{
		mt::lock_guard<mt::mutex> l(*_mtx);
		const auto m = sdb::unique_index<keyer::id>(_modules).find(module_id);

		return m ? info = *m, true : false;
	}


	module_tracker::metadata_ptr module_tracker::get_metadata(id_t module_id) const
	{
		string path;
		{
			mt::lock_guard<mt::mutex> l(*_mtx);
			const auto m = sdb::unique_index<keyer::id>(_modules).find(module_id);

			if (!m)
				throw invalid_argument("invalid persistent id");
			path = m->path;
		}
		return load_image_info(path);
	}

	shared_ptr<module::mapping> module_tracker::lock_mapping(id_t mapping_id)
	{
		void *expected_base;
		file_id file;

		{
			mt::lock_guard<mt::mutex> l(*_mtx);
			const auto m = sdb::unique_index<keyer::id>(_mappings).find(mapping_id);

			if (!m)
				return nullptr;
			expected_base = m->base;
			file = sdb::unique_index<keyer::id>(_modules).find(m->module_id)->file; // Module is guaranteed to present.
		}

		if (const auto candidate = _module_helper.lock_at(expected_base))
			if (candidate->base == expected_base && file_id(candidate->path) == file)
				return candidate;
		return nullptr;
	}

	shared_ptr<void> module_tracker::notify(mapping_access::events &events_)
	{
		auto mtx = _mtx;
		auto sinks = _sinks;
		auto i = [&] () -> list<mapping_access::events *>::iterator {
			mt::lock_guard<mt::mutex> l(*_mtx);

			for (auto i = begin(_mappings); i != end(_mappings); ++i)
				events_.mapped(i->module_id, i->id, *i);
			return sinks->insert(sinks->end(), &events_);
		}();

		return shared_ptr<void>(*i, [mtx, sinks, i] (void *) {
			mt::lock_guard<mt::mutex> l(*mtx);

			sinks->erase(i);
		});
	}

	void module_tracker::mapped(const module::mapping &mapping_)
	{
		auto &path = mapping_.path;
		file_id file(path);

		if (_this_module_file == file)
			return;

		mt::lock_guard<mt::mutex> l(*_mtx);
		auto r = sdb::unique_index(_mappings, keyer::base())[mapping_.base];
		auto r_module = sdb::unique_index(_modules, keyer::file_id())[file];

		if (r_module.is_new())
		{
			(*r_module).path = path;
			(*r_module).hash = calculate_hash(path);
			r_module.commit();
		}
		static_cast<module::mapping &>(*r) = mapping_;
		(*r).module_id = (*r_module).id;
		r.commit();

		for (auto i = begin(*_sinks); i != end(*_sinks); ++i)
			(*i)->mapped((*r).module_id, (*r).id, mapping_);
	}

	void module_tracker::unmapped(void *base)
	{
		mt::lock_guard<mt::mutex> l(*_mtx);
		auto &mappings_idx = sdb::unique_index(_mappings, keyer::base());

		if (!mappings_idx.find(static_cast<byte *>(base)))
			return;

		auto r = mappings_idx[static_cast<byte *>(base)];
		auto mapping_id = (*r).id;
		auto u = _unmapped.create();

		(*u).mapping_id = mapping_id;
		u.commit();
		r.remove();

		for (auto i = begin(*_sinks); i != end(*_sinks); ++i)
			(*i)->unmapped(mapping_id);
	}
}
