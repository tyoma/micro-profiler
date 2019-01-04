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
#include <common/symbol_resolver.h>

using namespace std;

namespace micro_profiler
{
	mapped_module_ex::mapped_module_ex(instance_id_t instance_id, const mapped_module &mm)
		: mapped_module(mm), _instance_id(instance_id)
	{	}

	shared_ptr< image_info<symbol_info_mapped> > mapped_module_ex::get_image_info() const
	{
		shared_ptr< image_info<symbol_info> > i1(load_image_info(path.c_str()));

		return shared_ptr< image_info<symbol_info_mapped> >(new offset_image_info(i1, (size_t)base));
	}

	mapped_module_ex::operator module_info_basic() const
	{
		module_info_basic mi = { _instance_id, reinterpret_cast<size_t>(base), path };
		return mi;
	}


	module_tracker::module_tracker()
		: _next_instance_id(0)
	{	}

	void module_tracker::load(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		mapped_module_ex::instance_id_t id = _next_instance_id++;
		shared_ptr<mapped_module_ex> m(new mapped_module_ex(id, get_module_info(in_image_address)));

		_modules_registry.insert(make_pair(id, m));
		_lqueue.push_back(id);
	}

	void module_tracker::unload(const void *in_image_address)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		const byte *base = get_module_info(in_image_address).base;

		for (modules_registry_t::iterator i = _modules_registry.begin(); i != _modules_registry.end(); ++i)
			if (base == i->second->base)
			{
				_uqueue.push_back(i->first);
				break;
			}
	}

	void module_tracker::get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_)
	{
		loaded_modules_.clear();
		unloaded_modules_.clear();

		mt::lock_guard<mt::mutex> l(_mtx);

		for (vector<mapped_module_ex::instance_id_t>::const_iterator i = _lqueue.begin(); i != _lqueue.end(); ++i)
			loaded_modules_.push_back(*get_module(*i));
		_lqueue.clear();

		swap(unloaded_modules_, _uqueue);
	}

	shared_ptr<const mapped_module_ex> module_tracker::get_module(mapped_module_ex::instance_id_t id) const
	{
		modules_registry_t::const_iterator i = _modules_registry.find(id);

		return i != _modules_registry.end() ? i->second : shared_ptr<mapped_module_ex>();
	}
}
