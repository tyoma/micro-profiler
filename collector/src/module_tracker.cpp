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
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const auto c_dummy = 1;
		const auto c_this_module_path = module::locate(&c_dummy).path;

		struct locked_mapping
		{
			module::mapping_instance module_;
			shared_ptr<void> lock;
		};
	}

	module_tracker::module_info::module_info(const string &path_)
		: path(path_), hash(calculate_hash(path_))
	{	}

	uint32_t module_tracker::module_info::calculate_hash(const string &path_)
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


	module_tracker::module_tracker()
		: _next_instance_id(0u), _next_persistent_id(1u)
	{	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		unordered_set<unsigned> in_snapshot;
		file_id this_module_file(c_this_module_path);

		module::notify([&] (const module::mapping &mm) {
			if (this_module_file == file_id(mm.path))
				return;

			const auto module_id = register_path(mm.path);
			auto &mi = _modules_registry.find(module_id)->second; // Guaranteed to present after register_path().

			if (!mi.mapping)
			{
				module::mapping_ex m = {
					module_id,
					mm.path,
					reinterpret_cast<uintptr_t>(mm.base),
					mi.hash
				};
				const auto mmi = make_shared<module::mapping_instance>(_next_instance_id++, m);

				mi.mapping = mmi;
				_lqueue.push_back(*mmi);
			}
			in_snapshot.insert(module_id);
		}, [] (const void *) {	});

		for (auto i = _modules_registry.begin(); i != _modules_registry.end(); ++i)
		{
			if (i->second.mapping && !in_snapshot.count(i->first))
			{
				_uqueue.push_back(i->second.mapping->first);
				i->second.mapping.reset();
			}
		}

		loaded_modules_.clear();
		unloaded_modules_.clear();
		swap(loaded_modules_, _lqueue);
		swap(unloaded_modules_, _uqueue);
	}

	shared_ptr<module::mapping_instance> module_tracker::lock_mapping(unsigned int module_id)
	{
		const auto i = _modules_registry.find(module_id);

		if (_modules_registry.end() == i)
			throw invalid_argument("invalid persistent id");
		auto l = make_shared<locked_mapping>();

		l->module_ = *i->second.mapping;
		l->lock = module::load(i->second.path);
		return shared_ptr<module::mapping_instance>(l, &l->module_);
	}

	module_tracker::metadata_ptr module_tracker::get_metadata(unsigned int module_id) const
	{
		modules_registry_t::const_iterator i = _modules_registry.find(module_id);

		return i != _modules_registry.end() ? load_image_info(i->second.path.c_str()) : 0;
	}

	shared_ptr<void> module_tracker::notify(events &/*events_*/)
	{
		throw 0;
	}



	unsigned int module_tracker::register_path(const string &path)
	{
		file_id fid(path);
		auto i = _files_registry.find(fid);

		if (_files_registry.end() == i)
		{
			auto module_id = _next_persistent_id++;

			i = _files_registry.insert(make_pair(fid, module_id)).first;
			_modules_registry.insert(make_pair(module_id, path));
		}
		return i->second;
	}
}
