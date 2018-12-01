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

#include <common/allocator.h>

#include <common/noncopyable.h>
#include <common/types.h>
#ifdef _WIN32
	#include <windows.h>
#elif __unix__
	#include <sys/mman.h>
#endif

using namespace std;

namespace micro_profiler
{
	class executable_memory_allocator::block : noncopyable
	{
	public:
		block(size_t block_size);
		~block();

		void *allocate(size_t size);

	private:
		byte * const _region;
		const size_t _block_size;
		size_t _occupied;
	};



	executable_memory_allocator::block::block(size_t block_size)
#ifdef _WIN32
		: _region(static_cast<byte *>(::VirtualAlloc(0, block_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE))),
#elif __unix__
		: _region(static_cast<byte *>(::mmap(0, block_size, PROT_EXEC | PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))),
#endif
			_block_size(block_size), _occupied(0)
	{	}

	executable_memory_allocator::block::~block()
	{
#ifdef _WIN32
		::VirtualFree(_region, 0, MEM_RELEASE);
#elif __unix__
		::munmap(_region, _block_size);
#endif
	}

	void *executable_memory_allocator::block::allocate(size_t size)
	{
		if (size <= _block_size - _occupied)
		{
			void *ptr = _region + _occupied;

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
