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
#include <unordered_set>

using namespace std;

namespace micro_profiler
{
	module_tracker::module_tracker()
		: _next_instance_id(0u), _next_persistent_id(1u)
	{	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		unordered_set<unsigned> in_snapshot;

		enumerate_process_modules([&] (const mapped_module &mm) {
			file_id fid(mm.path);
			const unsigned int &persistent_id = _files_registry[fid];
			const bool is_new = !persistent_id;

			if (is_new)
				_files_registry[fid] = _next_persistent_id++;

			module_info &mi = _modules_registry[persistent_id];

			if (!mi.mapping)
			{
				mapped_module_identified mmi = mapped_module_identified::from(_next_instance_id++, persistent_id, mm);

				mi.path = mm.path;
				mi.mapping.reset(new mapped_module_identified(mmi));
				_lqueue.push_back(mmi);
			}
			in_snapshot.insert(persistent_id);
		});

		for (modules_registry_t::iterator i = _modules_registry.begin(); i != _modules_registry.end(); ++i)
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

	module_tracker::metadata_ptr module_tracker::get_metadata(unsigned int persistent_id) const
	{
		modules_registry_t::const_iterator i = _modules_registry.find(persistent_id);

		return i != _modules_registry.end() ? load_image_info(i->second.path.c_str()) : 0;
	}
}
