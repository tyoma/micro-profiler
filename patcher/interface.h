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

#include <common/module.h>

namespace micro_profiler
{
	struct patch_state
	{
		id_t id;
		unsigned int rva;
		enum states {
			pending,	// Will be applied when module is mapped;
			active,	// Patch is applied;
			dormant,	// Patch was once requested, but is now reverted and not required on load;
			unrecoverable_error,	// Patch was once requested, but failed to apply. Sticks forever;
			activation_error, // Patch activation failed, can be retried later.
		} state;
		unsigned int size;
	};

	struct patch_change_result
	{
		enum errors {	ok, unchanged, unrecoverable_error, activation_error,	};

		id_t id; // Identifier assigned or existed before for a pending patch.
		unsigned int rva;
		errors result;
	};

	struct mapping_access
	{
		struct events;

		virtual std::shared_ptr<module::mapping> lock_mapping(id_t mapping_id) = 0;
		virtual std::shared_ptr<void> notify(events &events_) = 0;
	};

	struct mapping_access::events
	{
		// Events are guaranteed to come in causal order, i.e. an unmapped() is called for a module always before
		// it gets mapped() again. No more than one mapping_id is allowed to exist for any particular module_id at any
		// given moment in time.
		virtual void mapped(id_t module_id, id_t mapping_id, const module::mapping &mapping) = 0;
		virtual void unmapped(id_t mapping_id) = 0;
	};

	struct patch_manager
	{
		typedef std::vector<patch_change_result> patch_change_results;
		typedef std::vector<patch_state> patch_states;
		typedef std::pair<unsigned int /*rva*/, unsigned int /*size*/> apply_request;
		typedef range<const apply_request, size_t> apply_request_range;
		typedef unsigned int /*rva*/ revert_request;
		typedef range<const revert_request, size_t> revert_request_range;

		patch_manager() {	}

		virtual void query(patch_states &states, id_t module_id) = 0;
		virtual void apply(patch_change_results &results, id_t module_id, apply_request_range targets) = 0;
		virtual void revert(patch_change_results &results, id_t module_id, revert_request_range targets) = 0;
	};

	struct patch
	{
		virtual ~patch() {	}
		virtual bool activate() = 0;
		virtual bool revert() = 0;
	};
}
