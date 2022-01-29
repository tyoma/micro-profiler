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

#include <common/memory.h>

using namespace std;

namespace micro_profiler
{
	void mem_copy(void *dest, const void *src, size_t length)
	{
		byte *dest_ = static_cast<byte *>(dest);
		const byte *src_ = static_cast<const byte *>(src);

		while (length--)
			*dest_++ = *src_++;
	}

	void mem_set(void *dest, byte value, size_t length)
	{
		byte *dest_ = static_cast<byte *>(dest);

		while (length--)
			*dest_++ = value;
	}


	void *executable_memory_allocator::block::allocate(size_t size)
	{
		if (size <= _region.length() - _occupied)
		{
			void *ptr = _region.begin() + _occupied;

			_occupied += size;
			return ptr;
		}
		return 0;
	}


	executable_memory_allocator::executable_memory_allocator()
		: _block(new block(block_size))
	{	}

	shared_ptr<void> executable_memory_allocator::allocate(size_t size)
	{
		if (size > block_size)
			throw bad_alloc();

		void *ptr = _block->allocate(size);

		if (!ptr)
			_block.reset(new block(block_size)), ptr = _block->allocate(size);

		shared_ptr<block> b = _block;

		return shared_ptr<void>(ptr, [b] (...) { });
	}
}
