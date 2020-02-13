//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include <common/module.h>
#include <common/primitives.h>
#include <common/protocol.h>
#include <memory>
#include <mt/mutex.h>

namespace micro_profiler
{
	struct symbol_info;

	template <typename SymbolT>
	struct image_info;

	class mapped_module_ex : public mapped_module
	{
	public:
		typedef unsigned int instance_id_t;

	public:
		mapped_module_ex(instance_id_t instance_id, const mapped_module &mm);

		std::shared_ptr< image_info<symbol_info> > get_image_info() const;

	public:
		instance_id_t _instance_id;
	};

	class module_tracker
	{
	public:
		module_tracker();

		void load(const void *in_image_address);
		void unload(const void *in_image_address);
		
		void get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_);

		std::shared_ptr<const mapped_module_ex> get_module(mapped_module_ex::instance_id_t id) const;

	private:
		typedef std::unordered_map< mapped_module_ex::instance_id_t, std::shared_ptr<mapped_module_ex> > modules_registry_t;

	private:
		mt::mutex _mtx;
		std::vector<mapped_module_ex::instance_id_t> _lqueue, _uqueue;
		modules_registry_t _modules_registry;
		unsigned int _next_instance_id;
	};
}
