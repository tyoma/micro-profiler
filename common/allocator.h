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

#include <cstddef>

namespace micro_profiler
{
	struct allocator
	{
		virtual void *allocate(std::size_t length) = 0;
		virtual void deallocate(void *memory) throw() = 0;
	};

	struct default_allocator : allocator
	{
		virtual void *allocate(std::size_t length) override;
		virtual void deallocate(void *memory) throw() override;
	};

	namespace allocators
	{
		template <typename T>
		class adaptor
		{
		public:
			typedef T value_type;
			typedef value_type *pointer;
			typedef const value_type *const_pointer;
			typedef value_type &reference;
			typedef const value_type &const_reference;
			typedef typename std::size_t size_type;
			typedef typename std::ptrdiff_t difference_type;

			template <typename OtherT>
			struct rebind;

		public:
			adaptor(allocator &underlying)
				: _underlying(underlying)
			{	}

			template <typename U>
			adaptor(const adaptor<U> &other)
				: _underlying(other.underlying())
			{	}

			static size_type max_size()
			{	return 1u + (0u - sizeof(T)) / sizeof(T);	}

			static void construct(pointer ptr, const value_type &from)
			{	new (ptr) T(from);	}

			static void destroy(pointer ptr)
			{	ptr->~T(), ptr;	}

			pointer allocate(size_type n)
			{	return static_cast<pointer>(_underlying.allocate(sizeof(T) * n));	}

			void deallocate(pointer ptr, size_type /*n*/)
			{	return _underlying.deallocate(ptr);	}

			bool operator ==(const adaptor &rhs) const
			{	return &_underlying == &rhs._underlying;	}

			bool operator !=(const adaptor &rhs) const
			{	return &_underlying != &rhs._underlying;	}

			allocator &underlying() const
			{	return _underlying;	}

		private:
			allocator &_underlying;
		};

		template <typename T>
		template <typename OtherT>
		struct adaptor<T>::rebind
		{
			typedef adaptor<OtherT> other;
		};
	}
}
