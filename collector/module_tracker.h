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

#pragma once

#include <common/primitives.h>
#include <common/protocol.h>
#include <deque>
#include <mt/mutex.h>

namespace micro_profiler
{
	class module_tracker
	{
	public:
		typedef unsigned int instance_id_t;
	public:
		module_tracker();

		void load(const void *in_image_address);
		void unload(const void *in_image_address);
		
		void get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_);

	private:
		typedef std::unordered_map<unsigned int, module_info> modules_registry_t;

	private:
		const module_info *get_registered_module(instance_id_t id) const;

	private:
		mt::mutex _mtx;
		std::vector<instance_id_t> _lqueue, _uqueue;
		modules_registry_t _modules_registry;
		unsigned int _next_instance_id;
	};
}
