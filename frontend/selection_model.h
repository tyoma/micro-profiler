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

#include "key.h"

#include <common/noncopyable.h>
#include <unordered_set>
#include <wpl/models.h>
#include <views/integrated_index.h>

namespace micro_profiler
{
	template <typename KeyT>
	class selection : public wpl::dynamic_set_model, private views::table<KeyT>, noncopyable
	{
		typedef views::table<KeyT> base_t;

	public:
		typedef typename base_t::const_iterator const_iterator;
		typedef KeyT key_type;

	public:
		selection();

		// wpl::dynamic_set_model methods
		virtual void clear() throw() override;
		virtual void add(index_type item) override;
		virtual void remove(index_type item) override;
		virtual bool contains(index_type item) const throw() override;

		void add_key(const KeyT &key);
		using base_t::begin;
		using base_t::end;
		using wpl::dynamic_set_model::invalidate;

	private:
		virtual key_type get_key(std::size_t item) const = 0;

	private:
		views::immutable_unique_index<base_t, keyer::self> &_index;
	};


	template <typename UnderlyingT>
	class selection_model : public selection<typename key_traits<typename UnderlyingT::value_type>::key_type>
	{
	public:
		typedef wpl::dynamic_set_model::index_type index_type;
		typedef key_traits<typename UnderlyingT::value_type> selection_key_type;
		typedef typename selection_key_type::key_type key_type;

	public:
		selection_model(const UnderlyingT &underlying);

	private:
		virtual key_type get_key(std::size_t item) const override;

	private:
		const UnderlyingT &_underlying;
	};



	template <typename KeyT>
	inline selection<KeyT>::selection()
		: _index(views::unique_index(*this, keyer::self()))
	{	}

	template <typename KeyT>
	inline void selection<KeyT>::clear() throw()
	{
		base_t::clear();
		this->invalidate(npos());
	}

	template <typename KeyT>
	inline void selection<KeyT>::add(index_type item)
	{
		auto r = _index[get_key(item)];

		r.commit();
		this->invalidate(item);
	}

	template <typename KeyT>
	inline void selection<KeyT>::remove(index_type item)
	{
		auto r = _index[get_key(item)];

		r.remove();
		this->invalidate(item);
	}

	template <typename KeyT>
	inline bool selection<KeyT>::contains(index_type item) const throw()
	{	return !!_index.find(get_key(item));	}

	template <typename KeyT>
	inline void selection<KeyT>::add_key(const key_type &key)
	{
		auto r = _index[key];

		r.commit();
		this->invalidate(npos());
	}


	template <typename UnderlyingT>
	inline selection_model<UnderlyingT>::selection_model(const UnderlyingT &underlying)
		: _underlying(underlying)
	{	}

	template <typename UnderlyingT>
	inline typename selection_model<UnderlyingT>::key_type selection_model<UnderlyingT>::get_key(std::size_t item) const
	{	return selection_key_type::get_key(_underlying[item]);	}
}
