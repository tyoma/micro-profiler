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

#include <common/file_id.h>
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

	class module_tracker
	{
	public:
		typedef image_info<symbol_info> metadata_t;
		typedef std::shared_ptr<const metadata_t> metadata_ptr;

	public:
		module_tracker();

		void get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_);

		metadata_ptr get_metadata(mapped_module::instance_id_t persistent_id) const;

	private:
		struct module_info
		{
			std::string path;
			std::shared_ptr<mapped_module> mapping;
		};

		typedef std::unordered_map<file_id, mapped_module::instance_id_t /*persistent_id*/> files_registry_t;
		typedef std::unordered_map<mapped_module::instance_id_t /*persistent_id*/, module_info> modules_registry_t;

	private:
		mutable mt::mutex _mtx;
		files_registry_t _files_registry;
		modules_registry_t _modules_registry;
		std::vector<mapped_module> _lqueue;
		std::vector<mapped_module::instance_id_t> _uqueue;
		mapped_module::instance_id_t _next_instance_id, _next_persistent_id;
	};
}
