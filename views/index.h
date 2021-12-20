//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

		template <typename U, typename K>
		class immutable_unique_index
		{
		public:
			typedef typename K::key_type key_type;

		public:
			explicit immutable_unique_index(U &underlying, const K &keyer = K());

			const typename U::value_type &operator [](const key_type &key) const;
			typename U::transacted_record operator [](const key_type &key);

		private:
			std::unordered_map< key_type, typename U::iterator, hash<key_type> > _index;
			U &_underlying;
			const K _keyer;
			wpl::slot_connection _on_changed;
		};


		template <typename U, typename K>
		class immutable_index
		{
		public:
			class const_iterator;
			typedef typename K::key_type key_type;
			typedef std::pair<const_iterator, const_iterator> range_type;

		public:
			immutable_index(U &underlying, const K &keyer = K());

			range_type operator [](const key_type &key) const;

		private:
			typedef std::unordered_multimap< key_type, typename U::const_iterator, hash<key_type> > index_t;

		private:
			index_t _index;
			const K _keyer;
			wpl::slot_connection _on_changed;
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
			const_iterator(base from)
				: base(from)
			{	}

			const_reference operator *() const
			{	return *static_cast<const base &>(*this)->second;	}

			pointer operator ->() const
			{	return &*static_cast<const base &>(*this)->second;	}
		};



		template <typename U, typename K>
		inline immutable_unique_index<U, K>::immutable_unique_index(U &underlying, const K &keyer)
			: _underlying(underlying), _keyer(keyer)
		{
			typedef typename U::const_iterator iterator_t;

			auto on_changed = [this] (typename U::iterator record, bool new_) {
				if (new_)
					_index.insert(std::make_pair(_keyer(*iterator_t(record)), record));
			};

			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				on_changed(i, true);
			_on_changed = underlying.changed += on_changed;
		}

		template <typename U, typename K>
		inline const typename U::value_type &immutable_unique_index<U, K>::operator [](const key_type &key) const
		{
			const auto i = _index.find(key);

			if (i != _index.end())
				return *typename U::const_iterator(i->second);
			throw std::invalid_argument("key not found");
		}

		template <typename U, typename K>
		inline typename U::transacted_record immutable_unique_index<U, K>::operator [](const key_type &key)
		{
			const auto i = _index.find(key);

			if (i != _index.end())
				return *i->second;

			auto tr = _underlying.create();

			_keyer(*tr, key);
			return tr;
		}


		template <typename U, typename K>
		inline immutable_index<U, K>::immutable_index(U &underlying, const K &keyer)
			: _keyer(keyer)
		{
			auto on_changed = [this] (typename U::const_iterator record, bool new_) {
				if (new_)
					_index.insert(std::make_pair(_keyer(*record), record));
			};

			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				on_changed(i, true);
			_on_changed = underlying.changed += on_changed;
		}

		template <typename U, typename K>
		inline typename immutable_index<U, K>::range_type immutable_index<U, K>::operator [](const key_type &key) const
		{	return _index.equal_range(key);	}
	}
}
