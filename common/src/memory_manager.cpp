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

#include <common/memory_manager.h>

#include <common/gap_searcher.h>

using namespace std;

namespace micro_profiler
{
	class reachable_executable_allocator : public executable_memory_allocator
	{
	public:
		reachable_executable_allocator(const_byte_range reference, ptrdiff_t distance_order, size_t block_size)
			: _reference(reference), _distance_order(distance_order), _block_size(block_size)
		{	}

		virtual shared_ptr<void> allocate(size_t size) override
		{
			if (!_block)
				_block = make_shared<block>(_reference, _distance_order, _block_size);

			void *ptr = _block->allocate(size);

			if (!ptr)
				_block = make_shared<block>(_reference, _distance_order, _block_size), ptr = _block->allocate(size);
			return shared_ptr<void>(_block, ptr);
		}

	private:
		class block
		{
		public:
			block(const_byte_range r, ptrdiff_t d, size_t block_size)
				: _size(block_size), _occupied(0)
			{
				auto e = virtual_memory::enumerate_allocations();
				vector< pair<byte *, size_t> > allocations;
				auto l = r.end();
				auto above = ((static_cast<ptrdiff_t>(1) << (d - 1)) - 1) - static_cast<ptrdiff_t>(block_size);
				auto below = -((static_cast<ptrdiff_t>(1) << (d - 1)) - 1) - 1;

				for (pair<void *, size_t> a; e(a); )
					virtual_memory::normalize(a), allocations.push_back(make_pair(static_cast<byte *>(a.first), a.second));
				if (!gap_search_up(l, allocations, block_size, location_length_pair_less()) || l - r.begin() - 1 > above)
				{
					l = r.begin();
					if (!gap_search_down(l, allocations, block_size, location_length_pair_less()) || l - r.end() < below)
						throw bad_alloc();
				}
				_region = static_cast<byte *>(virtual_memory::allocate(l, block_size,
					protection::read | protection::write | protection::execute));
			}

			~block()
			{	virtual_memory::free(_region, _size);	}

			void *allocate(size_t size)
			{
				if (size > _size - _occupied)
					return nullptr;

				void *ptr = _region + _occupied;

				_occupied += size;
				return ptr;
			}

		private:
			byte *_region;
			size_t _size, _occupied;
		};

	private:
		const const_byte_range _reference;
		const ptrdiff_t _distance_order;
		const size_t _block_size;
		shared_ptr<block> _block;
	};


	memory_manager::memory_manager(size_t allocation_block_size)
		: _allocation_block_size(allocation_block_size)
	{	}

	shared_ptr<executable_memory_allocator> memory_manager::create_executable_allocator(const_byte_range reference,
		ptrdiff_t distance_order)
	{	return make_shared<reachable_executable_allocator>(reference, distance_order, _allocation_block_size);	}

	shared_ptr<void> memory_manager::scoped_protect(byte_range region, int /*protection::flags*/ /*scoped_protection*/,
		int /*protection::flags*/ /*released_protection*/)
	{
		// TODO: rewrite through tests.
		try
		{
			return make_shared<scoped_unprotect>(region); // For a while forgive protection change exceptions.
		} 
		catch (exception &/*e*/)
		{
			return nullptr;
		}
	}
}
