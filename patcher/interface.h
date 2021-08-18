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

#pragma once

#include <common/range.h>
#include <memory>
#include <vector>

namespace micro_profiler
{
	struct patch_result
	{
		enum errors {	ok, error, unchanged,	};
	};

	struct patch_state
	{
		enum states {	active, dormant, orphaned,	} state;
		unsigned int id;
	};

	struct patch_apply
	{
		patch_result::errors result;
		unsigned int id; // Identifier assigned or existed before for a dormant patch.
	};

	struct patch_manager
	{
		typedef std::vector< std::pair<unsigned int /*rva*/, patch_apply> > apply_results;
		typedef std::vector< std::pair<unsigned int /*rva*/, patch_result::errors> > revert_results;
		typedef std::vector< std::pair<unsigned int /*rva*/, patch_state> > patch_states;
		typedef range<const unsigned int /*rva*/, size_t> request_range;

		patch_manager() {	}

		virtual void query(patch_state &states, unsigned int persistent_id) = 0;
		virtual void apply(apply_results &results, unsigned int persistent_id, void *base, std::shared_ptr<void> lock,
			request_range targets) = 0;
		virtual void revert(revert_results &results, unsigned int persistent_id, request_range targets) = 0;
	};
}
