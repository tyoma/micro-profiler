//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/pool_allocator.h>

using namespace std;

namespace micro_profiler
{
	pool_allocator::pool_allocator(allocator &underlying)
		: _underlying(underlying)
	{	}

	pool_allocator::~pool_allocator()
	{
		for (auto i = _free.begin(); i != _free.end(); ++i)
		{
			while (!i->second.empty())
			{
				_underlying.deallocate(i->second.back());
				i->second.pop_back();
			}
		}
	}

	void *pool_allocator::allocate(size_t length)
	{
		auto bin = _free.find(length);

		if (bin != _free.end() && !bin->second.empty())
		{
			const auto e = bin->second.back();

			bin->second.pop_back();
			return e + 1;
		}
		return allocate_slow(bin, length);
	}

	void pool_allocator::deallocate(void *memory) throw()
	{
		const auto e = static_cast<entry *>(memory) - 1;
		auto &bin = _free.find(e->size)->second; // Returning a block not allocated with this allocator is UB!
		
		bin.push_back(e);
	}

	void *pool_allocator::allocate_slow(free_bins_t::iterator bin, size_t length)
	{
		if (bin == _free.end())
			bin = _free.insert(make_pair(length, free_bin_t())).first;
		bin->second.reserve(bin->second.capacity() + 1); // To guarantee, push_back() in deallocate() is noexcept.

		const auto e = static_cast<entry *>(_underlying.allocate(overhead + length));

		e->size = length;
		return e + 1;
	}
}
