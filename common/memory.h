#pragma once

#include "noncopyable.h"
#include "range.h"

#include <memory>

namespace micro_profiler
{
	void mem_copy(void *dest, const void *src, size_t length);
	void mem_set(void *dest, byte value, size_t length);


	class scoped_unprotect : noncopyable
	{
	public:
		scoped_unprotect(byte_range region);
		~scoped_unprotect();

	private:
		byte_range _region;
		unsigned _previous_access;
	};


	class executable_memory_allocator : noncopyable
	{
	public:
		enum { block_size = 0x10000 };

	public:
		executable_memory_allocator(const_byte_range reachable_region, std::ptrdiff_t max_distance);

		virtual std::shared_ptr<void> allocate(size_t size);

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
		const byte_range _region;
		size_t _occupied;
	};



	template <typename T>
	inline std::shared_ptr<T> make_shared_copy(const T &from)
	{	return std::make_shared<T>(from);	}
}
