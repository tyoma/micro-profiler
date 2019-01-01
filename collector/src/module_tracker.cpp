//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

using namespace std;

namespace micro_profiler
{
	module_tracker::module_tracker()
		: _next_instance_id(0)
	{	}

	void module_tracker::load(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		instance_id_t id = _next_instance_id++;
		module_info m = get_module_info(in_image_address);

		m.instance_id = id;
		_modules_registry.insert(make_pair(id, m));
		_lqueue.push_back(id);
	}

	void module_tracker::unload(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		const long_address_t addr = get_module_info(in_image_address).load_address;
		instance_id_t id;

		for (modules_registry_t::iterator i = _modules_registry.begin(); i != _modules_registry.end(); ++i)
			if (addr == i->second.load_address)
			{
				id = i->first;
				break;
			}

		_uqueue.push_back(id);
	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		loaded_modules_.clear();
		unloaded_modules_.clear();

		mt::lock_guard<mt::mutex> l(_mtx);

		for (vector<instance_id_t>::const_iterator i = _lqueue.begin(); i != _lqueue.end(); ++i)
			loaded_modules_.push_back(*get_registered_module(*i));
		for (vector<instance_id_t>::const_iterator i = _uqueue.begin(); i != _uqueue.end(); ++i)
			unloaded_modules_.push_back(*i);
		_lqueue.clear();
		_uqueue.clear();
	}

	const module_info *module_tracker::get_registered_module(instance_id_t id) const
	{
		modules_registry_t::const_iterator i = _modules_registry.find(id);

		return &i->second;
	}
}
