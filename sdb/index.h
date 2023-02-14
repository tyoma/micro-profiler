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

#include "hash.h"
#include "table_component.h"
#include "transform_iterator.h"

#include <common/compiler.h>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace sdb
{
	template <typename T>
	struct index_iterator_transform
	{
		template <typename I>
		const T &operator ()(I i) const
		{	return *i->second;	}
	};

	template <typename U, typename K, typename IndexT>
	class index_base : public table_component<typename U::const_iterator>, protected IndexT
	{
	protected:
		typedef typename result<K, typename U::value_type>::type key_type;
		typedef typename U::const_iterator underlying_iterator;
		typedef typename IndexT::const_iterator index_iterator_t;
		typedef std::unordered_map< underlying_iterator, index_iterator_t, iterator_hash<underlying_iterator> > removal_index_t;

	protected:
		index_base(index_base &&other)
			: IndexT(std::move(static_cast<IndexT &&>(other))), _keyer(std::move(other._keyer))
		{	}

		index_base(const U &underlying, const K &keyer)
			: _keyer(keyer)
		{
			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				created(i);
		}

		virtual void created(underlying_iterator record) override
		{	_removal_index.insert(std::make_pair(record, this->insert(std::make_pair(_keyer(*record), record))));	}

		virtual void removed(underlying_iterator record) override
		{
			const auto i = _removal_index.find(record);

			if (i != _removal_index.end())
				this->erase(i->second), _removal_index.erase(i);
		}

		virtual void cleared() override
		{	this->clear(), _removal_index.clear();	}

	protected:
		const K _keyer;

	private:
		index_base(const index_base &other);
		void operator =(index_base &&rhs);

	private:
		removal_index_t _removal_index;
	};


	template <typename U, typename K>
	class immutable_unique_index : public index_base< U, K, std::unordered_multimap<
		typename result<K, typename U::value_type>::type,
		typename U::const_iterator,
		hash<typename result<K, typename U::value_type>::type> > >
	{
	public:
		typedef typename result<K, typename U::value_type>::type key_type;

	public:
		immutable_unique_index(U &underlying, const K &keyer = K())
			: base_t(underlying, keyer), _underlying(underlying)
		{	}

		const typename U::value_type *find(const key_type &key) const
		{
			const auto r = base_t::equal_range(key);
			return r.first != r.second ? &*r.first->second : nullptr;
		}

		const typename U::value_type &operator [](const key_type &key) const
		{
			if (auto r = find(key))
				return *r;
			throw std::invalid_argument("key not found");
		}

		typename U::transacted_record operator [](const key_type &key)
		{
			const auto r = base_t::equal_range(key);

			return r.first != r.second ? _underlying.modify(r.first->second) : create_record(key);
		}

	private:
		typedef typename U::const_iterator underlying_iterator;
		typedef index_base< U, K, std::unordered_multimap< key_type, underlying_iterator, hash<key_type> > > base_t;

	private:
		FORCE_NOINLINE typename U::transacted_record create_record(const key_type &key)
		{
			auto tr = _underlying.create();

			this->_keyer(*this, *tr, key);
			return tr;
		}

	private:
		U &_underlying;
	};


	template <typename U, typename K>
	class immutable_index : public index_base< U, K, std::unordered_multimap<
		typename result<K, typename U::value_type>::type,
		typename U::const_iterator,
		hash<typename result<K, typename U::value_type>::type> > >
	{
	public:
		typedef typename result<K, typename U::value_type>::type key_type;
		typedef typename U::const_iterator underlying_iterator;
		typedef index_base< U, K, std::unordered_multimap< key_type, underlying_iterator, hash<key_type> > > base_t;

		typedef typename U::value_type value_type;
		typedef transform_iterator<typename base_t::index_iterator_t, index_iterator_transform<value_type> >
			const_iterator;

	public:
		explicit immutable_index(const U &underlying, const K &keyer = K())
			: base_t(underlying, keyer)
		{	}

		std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const
		{	return base_t::equal_range(key);	}
	};


	template <typename U, typename K>
	class ordered_index : public index_base< U, K, std::multimap<
		typename result<K, typename U::value_type>::type,
		typename U::const_iterator> >
	{
	public:
		typedef typename result<K, typename U::value_type>::type key_type;
		typedef typename U::const_iterator underlying_iterator;
		typedef index_base< U, K, std::multimap<key_type, underlying_iterator> > base_t;

		typedef typename U::value_type value_type;
		typedef transform_iterator<typename base_t::index_iterator_t, index_iterator_transform<value_type> >
			const_iterator;

	public:
		ordered_index(ordered_index &&other)
			: base_t(std::move(other))
		{	}

		explicit ordered_index(const U &underlying, const K &keyer = K())
			: base_t(underlying, keyer)
		{	}

		const_iterator upper_bound(const key_type &key) const
		{	return base_t::upper_bound(key);	}

		const_iterator begin() const
		{	return base_t::begin();	}

		const_iterator end() const
		{	return base_t::end();	}
	};
}

