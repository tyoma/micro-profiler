#pragma once

#include "noncopyable.h"
#include "primitives.h"

#include <memory>

namespace micro_profiler
{
	void mem_copy(void *dest, const void *src, size_t length);
	void mem_set(void *dest, byte value, size_t length);


	class scoped_unprotect : noncopyable
	{
	public:
		scoped_unprotect(range<byte> region);
		~scoped_unprotect();

	private:
		range<byte> _region;
		unsigned _previous_access;
	};


	class executable_memory_allocator : noncopyable
	{
	public:
		enum { block_size = 0x10000 };

	public:
		executable_memory_allocator();

		std::shared_ptr<void> allocate(size_t size);

	private:
		class block;

	private:
		std::shared_ptr<block> _block;
	};

	class executable_memory_allocator::block : noncopyable
	{
	public:
		block(size_t size);
		~block();

		void *allocate(size_t size);

	private:
		const range<byte> _region;
		size_t _occupied;
	};
}
