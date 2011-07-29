//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "./../primitives.h"
#include "ordered_view.h"

#include <functional>

namespace std
{
	using tr1::function;
}

typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

namespace micro_profiler
{
	typedef stdext::hash_map<const void *, unsigned __int64, address_compare> parent_statistics_map;

	struct function_statistics_detailed2 : function_statistics_detailed
	{
		parent_statistics_map parent_statistics;
	};

	typedef stdext::hash_map<const void * /*address*/, function_statistics_detailed2, address_compare> detailed_statistics2_map;

	class statistics
	{
		typedef std::function<bool (const void *lhs_addr, const function_statistics &lhs, const void *rhs_addr, const function_statistics &rhs)> _predicate_func_t;
		typedef std::function<bool (const void *lhs_addr, unsigned __int64 lhs, const void *rhs_addr, unsigned __int64 rhs)> _parent_predicate_func_t;

		detailed_statistics2_map _statistics;
		ordered_view<detailed_statistics2_map> _main_view;
		std::auto_ptr< ordered_view<statistics_map> > _children_view;
		std::auto_ptr< ordered_view<parent_statistics_map> > _parents_view;
		_predicate_func_t _children_predicate;
		bool _children_ascending;
		_parent_predicate_func_t _parents_predicate;
		bool _parents_ascending;

		statistics(const statistics &);
		void operator =(const statistics &);

	public:
		statistics();
		~statistics();

		void set_order(const _predicate_func_t &predicate, bool ascending);
		const detailed_statistics2_map::value_type &at(size_t index) const;
		size_t size() const;

		void set_focus(size_t index);
		void remove_focus();
		size_t find_index(const void *address) const;

		void set_children_order(const _predicate_func_t &predicate, bool ascending);
		const statistics_map::value_type &at_children(size_t index) const;
		size_t size_children() const;

		void set_parents_order(const _parent_predicate_func_t &predicate, bool ascending);
		const parent_statistics_map::value_type &at_parents(size_t index) const;
		size_t size_parents() const;

		void clear();
		void update(const FunctionStatisticsDetailed *data, unsigned int count);
	};


	inline statistics::statistics()
		: _main_view(_statistics)
	{	}

	inline statistics::~statistics()
	{	}

	inline void statistics::set_order(const _predicate_func_t &predicate, bool ascending)
	{	_main_view.set_order(predicate, ascending);	}

	inline const detailed_statistics2_map::value_type &statistics::at(size_t index) const
	{	return _main_view.at(index);	}

	inline size_t statistics::size() const
	{	return _main_view.size();	}
	
	inline void statistics::set_focus(size_t index)
	{
		const detailed_statistics2_map::value_type &item = at(index);

		_children_view.reset(new ordered_view<statistics_map>(item.second.children_statistics));
		_parents_view.reset(new ordered_view<parent_statistics_map>(item.second.parent_statistics));
		if (_children_predicate)
			_children_view->set_order(_children_predicate, _children_ascending);
		if (_parents_predicate)
			_parents_view->set_order(_parents_predicate, _parents_ascending);
	}

	inline void statistics::remove_focus()
	{
		_children_view.reset();
	}

	inline size_t statistics::find_index(const void *address) const
	{	return _main_view.find_by_key(address);	}


	inline void statistics::set_children_order(const _predicate_func_t &predicate, bool ascending)
	{
		_children_predicate = predicate;
		_children_ascending = ascending;
		if (_children_view.get())
			_children_view->set_order(predicate, ascending);
	}

	inline const statistics_map::value_type &statistics::at_children(size_t index) const
	{	return _children_view->at(index);	}

	inline size_t statistics::size_children() const
	{	return _children_view.get() ? _children_view->size() : 0;	}


	inline void statistics::set_parents_order(const _parent_predicate_func_t &predicate, bool ascending)
	{
		_parents_predicate = predicate;
		_parents_ascending = ascending;
		if (_parents_view.get())
			_parents_view->set_order(predicate, ascending);
	}

	inline const parent_statistics_map::value_type &statistics::at_parents(size_t index) const
	{	return _parents_view->at(index);	}

	inline size_t statistics::size_parents() const
	{	return _parents_view.get() ? _parents_view->size() : 0;	}


	inline void statistics::clear()
	{
		_children_view.reset();
		_parents_view.reset();
		_statistics.clear();
		_main_view.resort();
	}
}
