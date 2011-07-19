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



template<typename Functor>
struct DescFunctor
{
	DescFunctor(Functor& f) : _f(f) {}

	template<typename Arg>
	bool operator()(const Arg &left, const Arg &right)
	{
		return _f(right, left);
	}

private:
	Functor _f;
};

template<typename Functor>
DescFunctor<Functor> make_desc(Functor& f)
{
	return DescFunctor<Functor>(f);
}

template <typename Iter>
class Sorter
{
public:
	typedef Iter sorted_type;

	virtual void sort(sorted_type begin, sorted_type end, bool asc) = 0;

	virtual ~Sorter(){};
};

template<typename Functor, typename Iter>
class SorterImpl : public Sorter<Iter>
{
public:
	SorterImpl(Functor& functor) : _functor(functor) {}

	virtual void sort(sorted_type b, sorted_type e, bool asc)
	{
		if (asc)
			std::sort(b, e, _functor);
		else
			std::sort(b, e, make_desc(_functor));
	}

private:
	Functor _functor;
};

//////////////////////////////////////////////////////////////////////////

template <class Map>
class ordered_view
{
public:
	typedef typename Map::value_type value_type;
	typedef typename Map::key_type key_type;
	
	static const size_t npos = (size_t)(-1);

	ordered_view(const Map &map);

	size_t size() const;

	template <typename Predicate>
	void sort(Predicate predicate, bool ascending);

	const value_type &at(size_t index) const;

	size_t find_by_key(const key_type &key) const;

private:
	typedef Map map_type;
	typedef typename Map::const_iterator stored_type;
	
	typedef std::vector<typename stored_type> ordered_storage;
	typedef typename ordered_storage::iterator sorter_arg;

	typedef Sorter<sorter_arg> sorter_type;

	ordered_view(const ordered_view &);
	ordered_view &operator=(const ordered_view &);

	template <typename Functor>
	void reset_sorter(Functor f)
	{
		_sorter.reset(new SorterImpl<Functor, sorter_arg>(f));
	}

	const map_type &_map;
	ordered_storage _ordered_data;
	
	std::auto_ptr<sorter_type> _sorter;
};

/// Implementation

template <class Map>
ordered_view<Map>::ordered_view(const Map &map)
:_map(map)
{ 
	stored_type it = _map.begin();
	const stored_type end = _map.end();

	_ordered_data.reserve(std::distance(it, end));

	for(; it != end; ++it)
		_ordered_data.push_back(it);
}

template <class Map>
size_t ordered_view<Map>::size() const
{
	return _ordered_data.size();
}

template <class Map>
template <typename Predicate>
void ordered_view<Map>::sort( Predicate predicate, bool ascending )
{
	reset_sorter(predicate);
	_sorter->sort(_ordered_data.begin(), _ordered_data.end(), ascending);
}

template <class Map>
const typename ordered_view<Map>::value_type &ordered_view<Map>::at( size_t index ) const
{
	return *_ordered_data.at(index);
}

template <class Map>
size_t ordered_view<Map>::find_by_key(const key_type &key) const
{
	for(size_t i = 0; i < _ordered_data.size(); ++i)
	{
		if((*_ordered_data[i]).first == key)
			return i;
	}
	return npos;
}