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

#include "series.h"

#include <functional>
#include <memory>
#include <vector>
#include <algorithm>

namespace micro_profiler
{
	template <class Map>
	class ordered_view : public series<double>
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
		template <typename PredicateT>
		void set_order(PredicateT predicate, bool ascending);

		template <typename ExtractorT>
		void project_value(const ExtractorT &extractor);

		void disable_projection();

		// Repopulate internal ordered storage from source map with respect to predicate set.
		void resort();

		virtual size_t size() const throw();
		const value_type &at(size_t index) const;
		virtual double get_value(size_t index) const;
		size_t find_by_key(const key_type &key) const;

	private:
		typedef std::vector<const value_type *> ordered_container;

		template <typename PredicateT>
		class predicate_wrap_a;
		template <typename PredicateT>
		class predicate_wrap_d;

	private:
		// Repopulate internal storage with data from source map, ignores any predicate set.
		void fetch_data();

		template <typename PredicateT>
		void sort(const PredicateT &predicate);

		const ordered_view &operator =(const ordered_view &);

	private:
		const Map *_map;
		ordered_container _ordered_data;
		std::function<void()> _sorter;
		std::function<double(const typename Map::mapped_type &entry)> _extractor;
		bool _ascending;
	};

	template <typename Map>
	template <typename PredicateT>
	class ordered_view<Map>::predicate_wrap_a
	{
	public:
		predicate_wrap_a(const PredicateT &predicate)
			: _predicate(predicate)
		{ }

		template <typename ArgT>
		bool operator ()(ArgT lhs, ArgT rhs) const
		{	return _predicate(lhs->first, lhs->second, rhs->first, rhs->second);	}

	private:
		PredicateT _predicate;
	};

	template <typename Map>
	template <typename PredicateT>
	class ordered_view<Map>::predicate_wrap_d
	{
	public:
		predicate_wrap_d(const PredicateT &predicate)
			: _predicate(predicate)
		{ }

		template <typename ArgT>
		bool operator ()(ArgT lhs, ArgT rhs) const
		{	return _predicate(rhs->first, rhs->second, lhs->first, lhs->second);	}

	private:
		PredicateT _predicate;
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
	template <typename PredicateT>
	inline void ordered_view<Map>::set_order(PredicateT predicate, bool ascending)
	{
		if (ascending)
			_sorter = std::bind(&ordered_view::sort< predicate_wrap_a<PredicateT> >, this, predicate_wrap_a<PredicateT>(predicate));
		else
			_sorter = std::bind(&ordered_view::sort< predicate_wrap_d<PredicateT> >, this, predicate_wrap_d<PredicateT>(predicate));
		_ascending = ascending;
		_sorter();
	}

	template <class Map>
	template <typename ExtractorT>
	inline void ordered_view<Map>::project_value(const ExtractorT &extractor)
	{	_extractor = extractor;	}

	template <class Map>
	inline void ordered_view<Map>::disable_projection()
	{	_extractor = std::function<double(const typename Map::mapped_type &entry)>();	}

	template <class Map>
	inline void ordered_view<Map>::resort()
	{
		fetch_data();

		if (_sorter)
			_sorter();
		invalidated();
	}

	template <class Map>
	inline void ordered_view<Map>::fetch_data()
	{
		if (!_map)
			return;
		_ordered_data.clear();
		_ordered_data.reserve(_map->size());
		for (typename Map::const_iterator i = _map->begin(), end = _map->end(); i != end; ++i)
			_ordered_data.push_back(&*i);
	}

	template <typename Map>
	template <typename PredicateT>
	inline void ordered_view<Map>::sort(const PredicateT &predicate)
	{	std::sort(_ordered_data.begin(), _ordered_data.end(), predicate);	}

	template <class Map>
	inline const typename ordered_view<Map>::value_type &ordered_view<Map>::at(size_t index) const
	{	return *_ordered_data[index];	}

	template <class Map>
	inline double ordered_view<Map>::get_value(size_t index) const
	{	return _extractor ? _extractor(at(_ascending ? _ordered_data.size() - index - 1 : index).second) : double();	}

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
