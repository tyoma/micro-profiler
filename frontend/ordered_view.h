//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "piechart.h"

#include <functional>
#include <memory>
#include <vector>
#include <algorithm>

namespace micro_profiler
{
	template <class Map>
	class ordered_view : public piechart_model
	{
	public:
		typedef typename Map::value_type value_type;
		typedef typename Map::key_type key_type;

		static const size_t npos = static_cast<size_t>(-1);

	public:
		ordered_view(const Map &map);

		// Paermanently detaches from the underlying map. Postcondition: size() becomes zero.
		void detach() throw();

		// Set order for internal orderd storage, save it until new order is set.
		template <typename Predicate>
		void set_order(Predicate predicate, bool ascending);

		template <typename ExtractorT>
		void project_value(const ExtractorT &extractor);

		// Repopulate internal ordered storage from source map with respect to predicate set.
		void resort();

		virtual size_t size() const throw();
		const value_type &at(size_t index) const;
		virtual float get_value(size_t index) const;
		size_t find_by_key(const key_type &key) const;

	private:
		typedef typename Map::const_iterator stored_type;
		typedef std::vector<typename stored_type> ordered_storage;

		struct extractor;
		struct sorter;

		template <typename Predicate>
		class sorter_impl;

	private:
		// Repopulate internal storage with data from source map, ignores any predicate set.
		void fetch_data();

		const ordered_view &operator =(const ordered_view &);

	private:
		const Map *_map;
		ordered_storage _ordered_data;
		std::auto_ptr<sorter> _sorter;
		std::function<float(const typename Map::mapped_type &entry)> _extractor;
		bool _ascending;
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
	public:
		sorter_impl(Predicate predicate, bool ascending)
			: _predicate(predicate), _ascending(ascending)
		{	}

		virtual void sort(typename sorter::target_iterator begin, typename sorter::target_iterator end) const
		{	std::sort(begin, end, *this);	}

		bool operator ()(const typename Map::const_iterator &lhs, const typename Map::const_iterator &rhs) const
		{	return _ascending ? _predicate(lhs->first, lhs->second, rhs->first, rhs->second) : _predicate(rhs->first, rhs->second, lhs->first, lhs->second);	}

	private:
		Predicate _predicate;
		bool _ascending;
	};



	/// ordered_view implementation
	template <class Map>
	inline ordered_view<Map>::ordered_view(const Map &map)
		: _map(&map)
	{	fetch_data();	}

	template <class Map>
	inline void ordered_view<Map>::detach() throw()
	{
		_map = 0;
		_ordered_data.clear();
	}

	template <class Map>
	inline size_t ordered_view<Map>::size() const throw()
	{	return _ordered_data.size();	}

	template <class Map>
	template <typename Predicate>
	inline void ordered_view<Map>::set_order(Predicate predicate, bool ascending)
	{
		_sorter.reset(new sorter_impl<Predicate>(predicate, ascending));
		_ascending = ascending;
		_sorter->sort(_ordered_data.begin(), _ordered_data.end());
	}

	template <class Map>
	template <typename ExtractorT>
	inline void ordered_view<Map>::project_value(const ExtractorT &extractor)
	{	_extractor = extractor;	}

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
		if (!_map)
			return;
		_ordered_data.clear();
		_ordered_data.reserve(_map->size());
		for(stored_type i = _map->begin(), end = _map->end(); i != end; ++i)
			_ordered_data.push_back(i);
	}

	template <class Map>
	inline const typename ordered_view<Map>::value_type &ordered_view<Map>::at(size_t index) const
	{	return *_ordered_data.at(index);	}

	template <class Map>
	inline float ordered_view<Map>::get_value(size_t index) const
	{	return _extractor(at(_ascending ? _ordered_data.size() - index - 1 : index).second);	}

	template <class Map>
	inline size_t ordered_view<Map>::find_by_key(const key_type &key) const
	{
		for (size_t i = 0; i < _ordered_data.size(); ++i)
		{
			if (_ordered_data[i]->first == key)
				return i;
		}
		return npos;
	}
}
