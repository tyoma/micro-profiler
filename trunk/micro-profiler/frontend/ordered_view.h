//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <memory>
#include <vector>
#include <algorithm>

template <class Map>
class ordered_view
{
	typedef typename Map::const_iterator stored_type;
	typedef std::vector<typename stored_type> ordered_storage;

	struct sorter;

	template <typename Predicate>
	class sorter_impl;

	const Map &_map;
	ordered_storage _ordered_data;
	std::auto_ptr<sorter> _sorter;

	// Repopulate internal storage with data from source map, ignores any predicate set.
	void fetch_data();

	const ordered_view &operator =(const ordered_view &);

public:
	typedef typename Map::value_type value_type;
	typedef typename Map::key_type key_type;

	static const size_t npos = static_cast<size_t>(-1);

public:
	ordered_view(const Map &map);

	// Set order for internal orderd storage, save it until new order is set.
	template <typename Predicate>
	void set_order(Predicate predicate, bool ascending);

	// Repopulate internal ordered storage from source map with respect to predicate set.
	void resort();

	size_t size() const;
	const value_type &at(size_t index) const;
	size_t find_by_key(const key_type &key) const;
};

// Sorter interface.
// We need this to be able to switch sorter. Since concrete sorter type 
// depends on functor type, we can't switch it without interface.
template <class Map>
struct ordered_view<Map>::sorter
{
	typedef typename std::vector<typename Map::const_iterator>::iterator target_iterator;

	virtual ~sorter()	{	}

	virtual void sort(target_iterator begin, target_iterator end) const = 0;
};

// Sorter implementation.
// This stuff(i.e. desc_functor, sorter, sorter_impl) was introduced to let compiler inline functor.
// We sacrifice one virtual call, but get bunch of functor inlines.
template <class Map>
template <typename Predicate>
class ordered_view<Map>::sorter_impl : public ordered_view<Map>::sorter
{
	Predicate _predicate;
	bool _ascending;

public:
	sorter_impl(Predicate predicate, bool ascending)
		: _predicate(predicate), _ascending(ascending)
	{	}

	virtual void sort(typename sorter::target_iterator begin, typename sorter::target_iterator end) const
	{	std::sort(begin, end, *this);	}

	bool operator ()(const typename Map::const_iterator &lhs, const typename Map::const_iterator &rhs) const
	{	return _ascending ? _predicate(lhs, rhs) : _predicate(rhs, lhs);	}
};

/// ordered_view implementation
template <class Map>
inline ordered_view<Map>::ordered_view(const Map &map)
	: _map(map)
{	fetch_data();	}

template <class Map>
inline size_t ordered_view<Map>::size() const
{	return _ordered_data.size();	}

template <class Map>
template <typename Predicate>
inline void ordered_view<Map>::set_order(Predicate predicate, bool ascending)
{
	_sorter.reset(new sorter_impl<Predicate>(predicate, ascending));
	_sorter->sort(_ordered_data.begin(), _ordered_data.end());
}

template <class Map>
inline void ordered_view<Map>::resort()
{
	fetch_data();

	if (_sorter.get())
		_sorter->sort(_ordered_data.begin(), _ordered_data.end());
}

template <class Map>
inline void ordered_view<Map>::fetch_data()
{
	stored_type it = _map.begin();
	const stored_type end = _map.end();

	_ordered_data.clear();
	_ordered_data.reserve(_map.size());
	for(; it != end; ++it)
		_ordered_data.push_back(it);
}

template <class Map>
inline const typename ordered_view<Map>::value_type &ordered_view<Map>::at( size_t index ) const
{	return *_ordered_data.at(index);	}

template <class Map>
inline size_t ordered_view<Map>::find_by_key(const key_type &key) const
{
	for(size_t i = 0; i < _ordered_data.size(); ++i)
	{
		if((*_ordered_data[i]).first == key)
			return i;
	}
	return npos;
}
