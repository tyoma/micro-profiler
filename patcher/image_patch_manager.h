//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "interface.h"

#include <common/auto_increment.h>
#include <functional>
#include <mt/mutex.h>
#include <sdb/table.h>

namespace micro_profiler
{
	class executable_memory_allocator;

	class image_patch_manager : public patch_manager, mapping_access::events, noncopyable
	{
	public:
		typedef std::function<std::unique_ptr<patch> (void *target, std::size_t target_size, id_t id,
			executable_memory_allocator &allocator)> patch_factory;
		struct mapping;

	public:
		image_patch_manager(patch_factory patch_factory_, mapping_access &mappings, virtual_memory_manager &memory_manager_);
		~image_patch_manager();

		virtual std::shared_ptr<mapping> lock_module(id_t module_id);

		virtual void query(patch_states &states, id_t module_id) override;
		virtual void apply(patch_change_results &results, id_t module_id, apply_request_range targets) override;
		virtual void revert(patch_change_results &results, id_t module_id, revert_request_range targets) override;

	private:
		struct mapping_record
		{
			id_t module_id;
			id_t mapping_id;
			std::shared_ptr<executable_memory_allocator> allocator;
		};

		struct patch_record
		{
			patch_record();
			patch_record(patch_record &&from);

			id_t id;
			id_t module_id;
			unsigned int rva, size;
			enum {	dormant, active, unrecoverable_error, activation_error,	} state;
			std::unique_ptr<micro_profiler::patch> patch;
		};

	private:
		virtual void mapped(id_t module_id, id_t mapping_id, const module::mapping &mapping) override;
		virtual void unmapped(id_t mapping_id) override;

	private:
		const patch_factory _patch_factory;
		mapping_access &_mapping_access;
		virtual_memory_manager &_memory_manager;
		mt::mutex _mtx;
		sdb::table< patch_record, auto_increment_constructor<patch_record> > _patches;
		sdb::table<mapping_record> _mappings;

		std::shared_ptr<void> _mapping_subscription;
	};

	struct image_patch_manager::mapping : module::mapping
	{
		mapping(const module::mapping &from, std::shared_ptr<executable_memory_allocator> allocator_)
			: module::mapping(from), allocator(allocator_)
		{	}

		std::shared_ptr<executable_memory_allocator> allocator;
	};



	inline image_patch_manager::patch_record::patch_record()
		: state(dormant)
	{	}

	inline image_patch_manager::patch_record::patch_record(patch_record &&from)
		: id(from.id), module_id(from.module_id), rva(from.rva), state(from.state), patch(std::move(from.patch))
	{	}
}
