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

#include "./../collector/primitives.h"
#include "ordered_view.h"

#include <wpl/ui/listview.h>
#include <functional>

namespace std
{
	using tr1::function;
}

class symbol_resolver;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

namespace micro_profiler
{
	typedef stdext::hash_map<const void *, unsigned __int64, address_compare> parent_statistics_map;

	struct function_statistics_detailed2 : function_statistics_detailed
	{
		parent_statistics_map parent_statistics;
	};

	typedef stdext::hash_map<const void * /*address*/, function_statistics_detailed2, address_compare> detailed_statistics2_map;

	struct dependant_calls_list : public wpl::ui::listview::model
	{
		virtual const void *get_address_at(index_type index) const throw() = 0;
		virtual void update_view(bool notify) = 0;
	};

	class functions_list : public wpl::ui::listview::model, wpl::noncopyable
	{
		std::shared_ptr<symbol_resolver> _resolver;
		detailed_statistics2_map _statistics;
		ordered_view<detailed_statistics2_map> _view;
		std::shared_ptr<dependant_calls_list> _watched_parents, _watched_children;
		unsigned int _cycle_counter;

	public:
		functions_list(std::shared_ptr<symbol_resolver> resolver);
		~functions_list();

		void clear();
		void update(const FunctionStatisticsDetailed *data, unsigned int count);

		virtual index_type get_count() const throw();
		virtual void get_text(index_type row, index_type column, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual std::shared_ptr<const wpl::ui::listview::trackable> track(index_type row) const;

		index_type get_index(const void *address) const;
		const detailed_statistics2_map::value_type &get_at(index_type row) const;

		std::shared_ptr<dependant_calls_list> watch_parents(index_type index);
		std::shared_ptr<dependant_calls_list> watch_children(index_type index);
	};

	class parent_calls_list : public dependant_calls_list, wpl::noncopyable
	{
		std::shared_ptr<symbol_resolver> _resolver;
		ordered_view<parent_statistics_map> _view;

	public:
		parent_calls_list(std::shared_ptr<symbol_resolver> resolver, const parent_statistics_map &parents);

		virtual index_type get_count() const throw();
		virtual void get_text(index_type row, index_type column, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual const void *get_address_at(index_type index) const throw();
		virtual void update_view(bool notify);
	};

	class child_calls_list : public dependant_calls_list, wpl::noncopyable
	{
		std::shared_ptr<symbol_resolver> _resolver;
		ordered_view<statistics_map> _view;

	public:
		child_calls_list(std::shared_ptr<symbol_resolver> resolver, const statistics_map &children);

		virtual index_type get_count() const throw();
		virtual void get_text(index_type row, index_type column, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual const void *get_address_at(index_type index) const throw();
		virtual void update_view(bool notify);
	};


	inline functions_list::functions_list(std::shared_ptr<symbol_resolver> resolver)
		: _resolver(resolver), _view(_statistics), _cycle_counter(0)
	{	}

	inline wpl::ui::listview::index_type functions_list::get_count() const throw()
	{	return _view.size();	}

	inline const detailed_statistics2_map::value_type &functions_list::get_at(index_type row) const
	{	return _view.at(row);	}

	inline wpl::ui::listview::index_type parent_calls_list::get_count() const throw()
	{	return _view.size();	}

	inline const void *parent_calls_list::get_address_at(index_type index) const throw()
	{	return _view.at(index).first;	}


	inline wpl::ui::listview::index_type child_calls_list::get_count() const throw()
	{	return _view.size();	}

	inline const void *child_calls_list::get_address_at(index_type index) const throw()
	{	return _view.at(index).first;	}
}
