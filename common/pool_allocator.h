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

#include "allocator.h"
#include "hash.h"

#include <unordered_map>
#include <vector>

namespace micro_profiler
{
	class pool_allocator : public allocator
	{
	private:
		struct entry
		{
			std::size_t size;
		};

	public:
		enum {	overhead = sizeof(entry)	};

	public:
		pool_allocator(allocator &underlying);
		~pool_allocator();

		virtual void *allocate(std::size_t length) override;
		virtual void deallocate(void *memory) throw() override;

	private:
		typedef std::vector<entry *> free_bin_t;
		typedef std::unordered_map< size_t, free_bin_t, knuth_hash_fixed<sizeof(std::size_t)> > free_bins_t;

	private:
		void *allocate_slow(free_bins_t::iterator bin, std::size_t length);

	private:
		free_bins_t _free;
		allocator &_underlying;
	};
}
