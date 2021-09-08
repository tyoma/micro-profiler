//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/module.h>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct locked_mapping
		{
			mapped_module_identified module_;
			shared_ptr<void> lock;
		};
	}

	module_tracker::module_info::module_info(const string &path_)
		: path(path_)
	{	}

	module_tracker::module_tracker()
		: _next_instance_id(0u), _next_persistent_id(1u)
	{	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		unordered_set<unsigned> in_snapshot;

		enumerate_process_modules([&] (const mapped_module &mm) {
			const auto persistent_id = register_path(mm.path);
			auto &mi = _modules_registry.find(persistent_id)->second; // Guaranteed to present after register_path().

			if (!mi.mapping)
			{
				mapped_module_identified mmi = mapped_module_identified::from(_next_instance_id++, persistent_id, mm);

				mi.mapping.reset(new mapped_module_identified(mmi));
				_lqueue.push_back(mmi);
			}
			in_snapshot.insert(persistent_id);
		});

		for (auto i = _modules_registry.begin(); i != _modules_registry.end(); ++i)
		{
			if (i->second.mapping && !in_snapshot.count(i->first))
			{
				_uqueue.push_back(i->second.mapping->instance_id);
				i->second.mapping.reset();
			}
		}

		loaded_modules_.clear();
		unloaded_modules_.clear();
		swap(loaded_modules_, _lqueue);
		swap(unloaded_modules_, _uqueue);
	}

	shared_ptr<mapped_module_identified> module_tracker::lock_mapping(unsigned int persistent_id)
	{
		const auto i = _modules_registry.find(persistent_id);

		if (_modules_registry.end() == i)
			throw invalid_argument("invalid persistent id");
		auto l = make_shared<locked_mapping>();

		l->module_ = *i->second.mapping;
		l->lock = load_library(i->second.path);
		return shared_ptr<mapped_module_identified>(l, &l->module_);
	}

	module_tracker::metadata_ptr module_tracker::get_metadata(unsigned int persistent_id) const
	{
		modules_registry_t::const_iterator i = _modules_registry.find(persistent_id);

		return i != _modules_registry.end() ? load_image_info(i->second.path.c_str()) : 0;
	}

	unsigned int module_tracker::register_path(const string &path)
	{
		file_id fid(path);
		auto i = _files_registry.find(fid);

		if (_files_registry.end() == i)
		{
			auto persistent_id = _next_persistent_id++;

			i = _files_registry.insert(make_pair(fid, persistent_id)).first;
			_modules_registry.insert(make_pair(persistent_id, path));
		}
		return i->second;
	}
}
