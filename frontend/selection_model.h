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
#include <functional>
#include <unordered_set>
#include <wpl/models.h>
#include <sdb/integrated_index.h>

namespace micro_profiler
{
	template <typename KeyT>
	class selection : public wpl::dynamic_set_model, noncopyable
	{
		typedef sdb::table<KeyT> base_t;

	public:
		typedef typename base_t::const_iterator const_iterator;
		typedef KeyT key_type;
		typedef std::function<key_type (index_type item)> get_key_cb;

	public:
		selection(std::shared_ptr<base_t> scope, const get_key_cb &get_key);

		// wpl::dynamic_set_model methods
		virtual void clear() throw() override;
		virtual void add(index_type item) override;
		virtual void remove(index_type item) override;
		virtual bool contains(index_type item) const throw() override;

		const_iterator begin() const;
		const_iterator end() const;

	private:
		void notify_invalidate(index_type item);

	private:
		const std::shared_ptr<base_t> _scope;
		sdb::immutable_unique_index<base_t, keyer::self> &_index;
		const get_key_cb _get_key;
		unsigned int _omit_notify;
		const wpl::slot_connection _conn;
	};



	template <typename KeyT>
	inline selection<KeyT>::selection(std::shared_ptr<base_t> scope, const get_key_cb &get_key)
		: _scope(scope), _index(sdb::unique_index(*scope, keyer::self())), _get_key(get_key), _omit_notify(0),
			_conn(scope->invalidate += [this] {	if (!_omit_notify) invalidate(npos());	})
	{	}

	template <typename KeyT>
	inline void selection<KeyT>::clear() throw()
	{	_scope->clear();	}

	template <typename KeyT>
	inline void selection<KeyT>::add(index_type item)
	{
		auto r = _index[_get_key(item)];

		r.commit();
		notify_invalidate(item);
	}

	template <typename KeyT>
	inline void selection<KeyT>::remove(index_type item)
	{
		auto r = _index[_get_key(item)];

		r.remove();
		notify_invalidate(item);
	}

	template <typename KeyT>
	inline bool selection<KeyT>::contains(index_type item) const throw()
	{	return !!_index.find(_get_key(item));	}

	template <typename KeyT>
	inline typename selection<KeyT>::const_iterator selection<KeyT>::begin() const
	{	return _scope->begin();	}

	template <typename KeyT>
	inline typename selection<KeyT>::const_iterator selection<KeyT>::end() const
	{	return _scope->end();	}

	template <typename KeyT>
	inline void selection<KeyT>::notify_invalidate(index_type item)
	{
		if (!_omit_notify++)
		{
			_scope->invalidate();
			invalidate(item);
		}
		_omit_notify--;
	}
}
