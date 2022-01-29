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

#pragma once

#include <cstddef>
#include <memory>

#pragma warning(disable: 4127)

namespace micro_profiler
{
	namespace views
	{
		class any_key
		{
		public:
			enum {	storage_size = 48	};

		public:
			template <typename T, typename H>
			any_key(const T &from, const H &hash);
			any_key(const any_key &rhs);
			~any_key();

			std::size_t hash() const;
			bool operator ==(const any_key &rhs) const;

		private:
			struct box
			{
				virtual ~box() {	}
				virtual void copy(void *dest_ptr) const = 0;
				virtual std::size_t hash() const = 0;
				virtual bool equals(const void *rhs) const = 0;
			};

		private:
			void operator =(const any_key &rhs);

			const box &get_box() const;

		private:
			char _storage[storage_size + sizeof(void *)];
		};



		template <typename T, typename H>
		inline any_key::any_key(const T &from, const H &hash)
		{
#pragma pack(push, 1)
			struct box_impl : box
			{
				box_impl(const T &from, const H &hash)
					: _value(from), _hash(hash)
				{	}

				virtual void copy(void *dest_ptr) const override
				{	new (dest_ptr) box_impl(_value, _hash);	}

				virtual std::size_t hash() const override
				{	return _hash(_value);	}

				virtual bool equals(const void *rhs) const override
				{	return _value == static_cast<const box_impl *>(rhs)->_value;	}

				T _value;
				H _hash;
			};
#pragma pack(pop)

			if (sizeof(box_impl) > sizeof(_storage))
				throw std::bad_alloc();

			new (_storage) box_impl(from, hash);
		}

		inline any_key::any_key(const any_key &rhs)
		{	rhs.get_box().copy(_storage);	}

		inline any_key::~any_key()
		{	get_box().~box();	}

		inline std::size_t any_key::hash() const
		{	return get_box().hash();	}

		inline bool any_key::operator ==(const any_key &rhs) const
		{	return get_box().equals(rhs._storage);	}

		inline const any_key::box &any_key::get_box() const
		{	return *static_cast<const any_key::box *>(static_cast<const void *>(_storage));	}
	}
}

#pragma warning(default: 4127)
