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
		struct iterator_hash
		{
			std::size_t operator ()(T value) const
			{	return reinterpret_cast<std::size_t>(&*value);	}
		};

		template <typename U, typename K>
		class immutable_unique_index : public table_component
		{
		public:
			typedef typename K::key_type key_type;

		public:
			explicit immutable_unique_index(U &underlying, const K &keyer = K());

			const typename U::value_type *find(const key_type &key) const;

			const typename U::value_type &operator [](const key_type &key) const;
			typename U::transacted_record operator [](const key_type &key);

		private:
			typedef typename U::const_iterator underlying_iterator;
			typedef std::unordered_map< key_type, underlying_iterator, hash<key_type> > index_t;
			typedef std::unordered_map< underlying_iterator, typename index_t::const_iterator, iterator_hash<underlying_iterator> > removal_index_t;

		private:
			immutable_unique_index(const immutable_unique_index &other);
			void operator =(const immutable_unique_index &rhs);

			typename U::transacted_record create_record(const key_type &key);

		private:
			index_t _index;
			removal_index_t _removal_index;
			U &_underlying;
			const K _keyer;
			wpl::slot_connection _connections[3];
		};


		template <typename U, typename K>
		class immutable_index : public table_component
		{
		public:
			class const_iterator;
			typedef typename K::key_type key_type;
			typedef std::pair<const_iterator, const_iterator> range_type;
			typedef typename U::value_type value_type;
			typedef typename U::const_reference const_reference;

		public:
			immutable_index(const U &underlying, const K &keyer = K());

			range_type equal_range(const key_type &key) const;

		private:
			typedef typename U::const_iterator underlying_iterator;
			typedef std::unordered_multimap< key_type, underlying_iterator, hash<key_type> > index_t;
			typedef std::unordered_map< underlying_iterator, typename index_t::const_iterator, iterator_hash<underlying_iterator> > removal_index_t;

		private:
			immutable_index(const immutable_index &other);
			void operator =(const immutable_index &rhs);

		private:
			index_t _index;
			removal_index_t _removal_index;
			const U &_underlying;
			const K _keyer;
			wpl::slot_connection _connections[3];
		};

		template <typename U, typename K>
		class immutable_index<U, K>::const_iterator : public index_t::const_iterator
		{
			typedef typename index_t::const_iterator base;

		public:
			typedef const typename U::value_type &const_reference;
			typedef const typename U::value_type *pointer;
			typedef const_reference reference;

		public:
			const_iterator()
			{	}

			const_iterator(base from)
				: base(from)
			{	}

			const_reference operator *() const
			{	return *static_cast<const base &>(*this)->second;	}

			pointer operator ->() const
			{	return &*static_cast<const base &>(*this)->second;	}

			underlying_iterator underlying() const
			{	return static_cast<const base &>(*this)->second;	}
		};



		template <typename U, typename K>
		inline immutable_unique_index<U, K>::immutable_unique_index(U &underlying, const K &keyer)
			: _underlying(underlying), _keyer(keyer)
		{
			auto on_changed = [this] (underlying_iterator record, bool new_) {
				if (new_)
				{
					auto i = _index.insert(std::make_pair(_keyer(*record), record)).first;

					_removal_index.insert(std::make_pair(record, i));
				}
			};

			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				on_changed(i, true);
			_connections[0] = underlying.changed += on_changed;
			_connections[1] = underlying.cleared += [this] {	_index.clear();	};
			_connections[2] = underlying.removed += [this] (underlying_iterator record) {
				const auto i = _removal_index.find(record);
				
				if (i != _removal_index.end())
				{
					_index.erase(i->second);
					_removal_index.erase(i);
				}
			};
		}

		template <typename U, typename K>
		inline const typename U::value_type *immutable_unique_index<U, K>::find(const key_type &key) const
		{
			const auto i = _index.find(key);
			return i != _index.end() ? &*i->second : nullptr;
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
			const auto i = _index.find(key);

			return i != _index.end() ? _underlying.modify(i->second) : create_record(key);
		}

		template <typename U, typename K>
		FORCE_NOINLINE inline typename U::transacted_record immutable_unique_index<U, K>::create_record(const key_type &key)
		{
			auto tr = _underlying.create();

			_keyer(*this, *tr, key);
			return tr;
		}


		template <typename U, typename K>
		inline immutable_index<U, K>::immutable_index(const U &underlying, const K &keyer)
			: _underlying(underlying), _keyer(keyer)
		{
			auto on_changed = [this] (underlying_iterator record, bool new_) {
				if (new_)
				{
					auto i = _index.insert(std::make_pair(_keyer(*record), record));

					_removal_index.insert(std::make_pair(record, i));
				}
			};

			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				on_changed(i, true);
			_connections[0] = underlying.changed += on_changed;
			_connections[1] = underlying.cleared += [this] {	_index.clear();	};
			_connections[2] = underlying.removed += [this] (underlying_iterator record) {
				const auto i = _removal_index.find(record);

				if (i != _removal_index.end())
				{
					_index.erase(i->second);
					_removal_index.erase(i);
				}
			};
		}

		template <typename U, typename K>
		inline typename immutable_index<U, K>::range_type immutable_index<U, K>::equal_range(const key_type &key) const
		{	return _index.equal_range(key);	}
	}
}
