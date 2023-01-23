#pragma once

#include "noncopyable.h"
#include "range.h"

#include <memory>

namespace micro_profiler
{
	void mem_copy(void *dest, const void *src, std::size_t length);
	void mem_set(void *dest, byte value, std::size_t length);


	class scoped_unprotect : noncopyable
	{
	public:
		scoped_unprotect(byte_range region);
		~scoped_unprotect();

	private:
		byte_range _region;
		unsigned _previous_access;
	};

	struct virtual_memory
	{
		enum protection_flags {	read = (1 << 0), write = (1 << 1), execute = (1 << 2),	};

		struct bad_fixed_alloc;

		static std::size_t granularity();
		static void *allocate(std::size_t size, int protection);
		static void *allocate(const void *at, std::size_t size, int protection);
		static void free(void *address, std::size_t size);
	};

	struct virtual_memory::bad_fixed_alloc : std::bad_alloc
	{
	};

	class executable_memory_allocator : noncopyable
	{
	public:
		enum { block_size = 0x10000 };

	public:
		executable_memory_allocator(const_byte_range reachable_region, std::ptrdiff_t max_distance);

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



	template <typename T>
	inline std::shared_ptr<T> make_shared_copy(const T &from)
	{	return std::make_shared<T>(from);	}
}
