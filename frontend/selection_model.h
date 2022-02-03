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

namespace micro_profiler
{
	template <typename KeyT>
	struct selection : wpl::dynamic_set_model
	{
		virtual void add_key(const KeyT &key) = 0;
		virtual void enumerate(const std::function<void (const KeyT &key)> &callback) const = 0;
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

		// wpl::dynamic_set_model methods
		virtual void clear() throw() override;
		virtual void add(index_type item) override;
		virtual void remove(index_type item) override;
		virtual bool contains(index_type item) const throw() override;

		// selection<...> methods
		virtual void add_key(const key_type &key) override;
		virtual void enumerate(const std::function<void (const key_type &key)> &callback) const override;

		using wpl::dynamic_set_model::npos;

	private:
		key_type get_key(size_t item) const;

	private:
		const UnderlyingT &_underlying;
		std::unordered_set<key_type> _selection;
	};



	template <typename UnderlyingT>
	inline selection_model<UnderlyingT>::selection_model(const UnderlyingT &underlying)
		: _underlying(underlying)
	{	}

	template <typename UnderlyingT>
	inline void selection_model<UnderlyingT>::clear() throw()
	{
		_selection.clear();
		this->invalidate(npos());
	}

	template <typename UnderlyingT>
	inline void selection_model<UnderlyingT>::add(index_type item)
	{
		_selection.insert(get_key(item));
		this->invalidate(item);
	}

	template <typename UnderlyingT>
	inline void selection_model<UnderlyingT>::remove(index_type item)
	{
		_selection.erase(get_key(item));
		this->invalidate(item);
	}

	template <typename UnderlyingT>
	inline bool selection_model<UnderlyingT>::contains(index_type item) const throw()
	{	return !!_selection.count(get_key(item));	}

	template <typename UnderlyingT>
	inline void selection_model<UnderlyingT>::add_key(const key_type &key)
	{
		_selection.insert(key);
		this->invalidate(npos());
	}

	template <typename UnderlyingT>
	inline void selection_model<UnderlyingT>::enumerate(const std::function<void (const key_type &key)> &callback) const
	{
		for (auto i = _selection.begin(); i != _selection.end(); ++i)
			callback(*i);
	}

	template <typename UnderlyingT>
	inline typename selection_model<UnderlyingT>::key_type selection_model<UnderlyingT>::get_key(size_t item) const
	{	return selection_key_type::get_key(_underlying[item]);	}
}
