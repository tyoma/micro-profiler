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

			template <typename IndexT, typename T>
			void operator ()(const IndexT &, T &record, id_t key) const
			{	record.id = key;	}
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
		: _last_reported_mapping_id(0u), _module_helper(module_helper),
			_this_module_file(module_helper.locate(&local_dummy).path), _module_notifier(module_helper.notify(*this))
	{	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		auto last_reported_mapping_id = _last_reported_mapping_id;
		auto &mappings_idx = sdb::ordered_index_(_mappings, keyer::id());
		const auto &modules_idx = sdb::unique_index(_modules, keyer::id());
		auto i = mappings_idx.upper_bound(last_reported_mapping_id);

		loaded_modules_.clear();
		for (auto end = mappings_idx.end(); i != end; i++)
		{
			auto &module = modules_idx[i->module_id];
			module::mapping_ex m = {	module.id, module.path, reinterpret_cast<uintptr_t>(i->base), module.hash	};

			loaded_modules_.push_back(make_pair(i->id, m));
			last_reported_mapping_id = i->id;
		}
		unloaded_modules_.clear();
		swap(unloaded_modules_, _uqueue);
		_last_reported_mapping_id = last_reported_mapping_id;
	}

	bool module_tracker::get_module(module_info& info, id_t module_id) const
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		const auto m = sdb::unique_index<keyer::id>(_modules).find(module_id);

		return m ? info = *m, true : false;
	}


	module_tracker::metadata_ptr module_tracker::get_metadata(id_t module_id) const
	{
		string path;
		{
			mt::lock_guard<mt::mutex> l(_mtx);
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
			mt::lock_guard<mt::mutex> l(_mtx);
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
		mt::lock_guard<mt::mutex> l(_mtx);

		for (auto i = begin(_mappings); i != end(_mappings); ++i)
			events_.mapped(i->module_id, i->id, *i);
		return make_shared<bool>();
	}

	void module_tracker::mapped(const module::mapping &mapping_)
	{
		auto &path = mapping_.path;
		file_id file(path);

		if (!(_this_module_file == file))
		{
			mt::lock_guard<mt::mutex> l(_mtx);
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
		}
	}

	void module_tracker::unmapped(void *base)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		auto &mappings_by_base = sdb::unique_index(_mappings, keyer::base());

		if (auto r = mappings_by_base.find(static_cast<byte *>(base)))
		{
			_uqueue.push_back(r->id);
			mappings_by_base[static_cast<byte *>(base)].remove();
		}
	}
}
