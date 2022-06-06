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

#include "table_component.h"

#include <common/compiler.h>
#include <stdexcept>
#include <unordered_map>
#include <wpl/signal.h>

namespace micro_profiler
{
	namespace views
	{
		template <typename T>
		struct hash : std::hash<T>
		{	};

		template <typename T>
		struct hash<T *>
		{
			std::size_t operator ()(T *value) const
			{	return reinterpret_cast<std::size_t>(value);	}
		};

		template <typename T>
		struct iterator_hash
		{
			std::size_t operator ()(T value) const
			{	return reinterpret_cast<std::size_t>(&*value);	}
		};

		template <typename F, typename Arg1T>
		struct result
		{
			typedef decltype((*static_cast<F *>(nullptr))(*static_cast<Arg1T *>(nullptr))) type_rcv;
			typedef typename std::remove_reference<type_rcv>::type type_cv;
			typedef typename std::remove_cv<type_cv>::type type;
		};


		template <typename U, typename K>
		class immutable_index_base : public table_component<typename U::const_iterator>
		{
		public:
			typedef typename result<K, typename U::value_type>::type key_type;
			typedef typename U::const_iterator underlying_iterator;

		public:
			immutable_index_base(const U &underlying, const K &keyer)
				: _keyer(keyer)
			{
				for (auto i = underlying.begin(); i != underlying.end(); ++i)
					created(i);
			}

			virtual void created(underlying_iterator record) override
			{	_removal_index.insert(std::make_pair(record, _index.insert(std::make_pair(_keyer(*record), record))));	}

			virtual void removed(underlying_iterator record) override
			{
				const auto i = _removal_index.find(record);

				if (i != _removal_index.end())
					_index.erase(i->second), _removal_index.erase(i);
			}

			virtual void cleared() override
			{
				_index.clear();
				_removal_index.clear();
			}

		protected:
			typedef std::unordered_multimap< key_type, underlying_iterator, hash<key_type> > index_t;
			typedef typename index_t::const_iterator index_iterator_t;
			typedef std::unordered_map< underlying_iterator, index_iterator_t, iterator_hash<underlying_iterator> > removal_index_t;

		protected:
			std::pair<index_iterator_t, index_iterator_t> equal_range(const key_type &key) const
			{	return _index.equal_range(key);	}

		protected:
			const K _keyer;

		private:
			immutable_index_base(const immutable_index_base &other);
			void operator =(const immutable_index_base &rhs);

		private:
			index_t _index;
			removal_index_t _removal_index;
		};


		template <typename U, typename K>
		class immutable_unique_index : public immutable_index_base<U, K>
		{
			typedef immutable_index_base<U, K> base_t;

		public:
			using typename base_t::key_type;

		public:
			explicit immutable_unique_index(U &underlying, const K &keyer = K());

			const typename U::value_type *find(const key_type &key) const;

			const typename U::value_type &operator [](const key_type &key) const;
			typename U::transacted_record operator [](const key_type &key);

		private:
			typename U::transacted_record create_record(const key_type &key);

		private:
			U &_underlying;
		};


		template <typename U, typename K>
		class immutable_index : public immutable_index_base<U, K>
		{
			typedef immutable_index_base<U, K> base_t;

		public:
			class const_iterator;
			using typename base_t::key_type;
			typedef std::pair<const_iterator, const_iterator> range_type;
			typedef typename U::value_type value_type;
			typedef typename U::const_reference const_reference;

		public:
			explicit immutable_index(const U &underlying, const K &keyer = K());

			range_type equal_range(const key_type &key) const;
		};

		template <typename U, typename K>
		class immutable_index<U, K>::const_iterator : public base_t::index_t::const_iterator
		{
			typedef typename base_t::index_t index_t;
			typedef typename index_t::const_iterator base_iterator_t;

		public:
			typedef const typename U::value_type &const_reference;
			typedef const typename U::value_type *pointer;
			typedef const_reference reference;

		public:
			const_iterator()
			{	}

			const_iterator(base_iterator_t from)
				: base_iterator_t(from)
			{	}

			const_reference operator *() const
			{	return *static_cast<const base_iterator_t &>(*this)->second;	}

			pointer operator ->() const
			{	return &*static_cast<const base_iterator_t &>(*this)->second;	}

			typename U::const_iterator underlying() const
			{	return static_cast<const base_iterator_t &>(*this)->second;	}

			const_iterator &operator ++()
			{	return ++static_cast<base_iterator_t &>(*this), *this;	}

			const_iterator operator ++(int)
			{	return const_iterator(static_cast<base_iterator_t &>(*this)++);	}
		};



		template <typename U, typename K>
		inline immutable_unique_index<U, K>::immutable_unique_index(U &underlying, const K &keyer)
			: immutable_index_base<U, K>(underlying, keyer), _underlying(underlying)
		{	}

		template <typename U, typename K>
		inline const typename U::value_type *immutable_unique_index<U, K>::find(const key_type &key) const
		{
			const auto r = base_t::equal_range(key);
			return r.first != r.second ? &*r.first->second : nullptr;
		}

		template <typename U, typename K>
		inline const typename U::value_type &immutable_unique_index<U, K>::operator [](const key_type &key) const
		{
			if (auto r = find(key))
				return *r;
			throw std::invalid_argument("key not found");
		}

		template <typename U, typename K>
		inline typename U::transacted_record immutable_unique_index<U, K>::operator [](const key_type &key)
		{
			const auto r = base_t::equal_range(key);

			return r.first != r.second ? _underlying.modify(r.first->second) : create_record(key);
		}

		template <typename U, typename K>
		FORCE_NOINLINE inline typename U::transacted_record immutable_unique_index<U, K>::create_record(const key_type &key)
		{
			auto tr = _underlying.create();

			this->_keyer(*this, *tr, key);
			return tr;
		}


		template <typename U, typename K>
		inline immutable_index<U, K>::immutable_index(const U &underlying, const K &keyer)
			: immutable_index_base<U, K>(underlying, keyer)
		{	}

		template <typename U, typename K>
		inline typename immutable_index<U, K>::range_type immutable_index<U, K>::equal_range(const key_type &key) const
		{	return base_t::equal_range(key);	}
	}
}
