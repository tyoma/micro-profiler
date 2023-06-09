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

#include "noncopyable.h"
#include "range.h"

#include <functional>
#include <memory>

namespace micro_profiler
{
	class executable_memory_allocator;

	void mem_copy(void *dest, const void *src, std::size_t length);
	void mem_set(void *dest, byte value, std::size_t length);

	struct protection
	{
		enum flags {	read = (1 << 0), write = (1 << 1), execute = (1 << 2),	};
	};

	struct virtual_memory_manager
	{
		virtual std::shared_ptr<executable_memory_allocator> create_executable_allocator(const_byte_range reference_region,
			std::ptrdiff_t distance_order) = 0;
		virtual std::shared_ptr<void> scoped_protect(byte_range region, int /*protection::flags*/ scoped_protection,
			int /*protection::flags*/ released_protection) = 0;
	};

	class scoped_unprotect : noncopyable
	{
	public:
		scoped_unprotect(byte_range region);
		~scoped_unprotect();

	private:
		byte_range _region;
		unsigned _previous_access;
	};

	struct mapped_region
	{
		byte *address;
		std::size_t size;
		int /*protection::flags*/ protection;
	};

	struct virtual_memory
	{
		struct bad_fixed_alloc;

		static std::size_t granularity();
		static void *allocate(std::size_t size, int protection);
		static void *allocate(const void *at, std::size_t size, int protection);
		static void free(void *address, std::size_t size);
		static std::function<bool (std::pair<void *, size_t> &allocation)> enumerate_allocations();
		static void normalize(std::pair<void *, size_t> &allocation);
	};

	struct virtual_memory::bad_fixed_alloc : std::bad_alloc
	{
	};

	class executable_memory_allocator : noncopyable
	{
	public:
		enum { block_size = 0x10000 };

	public:
		executable_memory_allocator();

		virtual std::shared_ptr<void> allocate(std::size_t size);

	private:
		class block;

	private:
		std::shared_ptr<block> _block;
	};

	class executable_memory_allocator::block : noncopyable
	{
	public:
		block(std::size_t size);
		~block();

		void *allocate(std::size_t size);

	private:
		const byte_range _region;
		std::size_t _occupied;
	};
}
