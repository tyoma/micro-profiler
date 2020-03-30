//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "ordered_view.h"
#include "primitives.h"

#include <common/noncopyable.h>

namespace micro_profiler
{
	class symbol_resolver;
	class threads_model;

	template <typename BaseT, typename MapT>
	class statistics_model_impl : public BaseT, noncopyable
	{
	public:
		typedef typename BaseT::index_type index_type;

	public:
		statistics_model_impl(const MapT &statistics, double tick_interval,
			const std::shared_ptr<symbol_resolver> &resolver, const std::shared_ptr<threads_model> &threads);

		std::shared_ptr<symbol_resolver> get_resolver() const throw();
		std::shared_ptr<threads_model> get_threads() const throw();
		std::shared_ptr< series<double> > get_column_series() const throw();

		virtual void detach() throw();

		// wpl::ui::table_model methods
		virtual index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual std::shared_ptr<const wpl::ui::trackable> track(index_type row) const;

		// linked_statistics methods
		virtual function_key get_key(index_type item) const;

		index_type get_index(function_key address) const;

	protected:
		typedef ordered_view<MapT> view_type;

	protected:
		const typename MapT::value_type &get_entry(index_type row) const;
		virtual void on_updated();

	private:
		std::shared_ptr< ordered_view<MapT> > _view;
		const double _tick_interval;
		const std::shared_ptr<symbol_resolver> _resolver;
		const std::shared_ptr<threads_model> _threads;
	};



	template <typename BaseT, typename MapT>
	inline statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, double tick_interval,
			const std::shared_ptr<symbol_resolver> &resolver, const std::shared_ptr<threads_model> &threads)
		: _view(new ordered_view<MapT>(statistics)), _tick_interval(tick_interval), _resolver(resolver), _threads(threads)
	{ }

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<symbol_resolver> statistics_model_impl<BaseT, MapT>::get_resolver() const throw()
	{	return _resolver;	}

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<threads_model> statistics_model_impl<BaseT, MapT>::get_threads() const throw()
	{	return _threads;	}

	template <typename BaseT, typename MapT>
	std::shared_ptr< series<double> > statistics_model_impl<BaseT, MapT>::get_column_series() const throw()
	{	return _view;	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::detach() throw()
	{
		_view.reset();
		this->invalidated(0);
	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_count() const throw()
	{ return _view ? _view->size() : 0u; }

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<const wpl::ui::trackable> statistics_model_impl<BaseT, MapT>::track(index_type row) const
	{	return _view->track(row);	}

	template <typename BaseT, typename MapT>
	inline function_key statistics_model_impl<BaseT, MapT>::get_key(index_type item) const
	{	return get_entry(item).first;	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_index(function_key key) const
	{	return _view->find_by_key(key);	}

	template <typename BaseT, typename MapT>
	inline const typename MapT::value_type &statistics_model_impl<BaseT, MapT>::get_entry(index_type row) const
	{	return (*_view)[row];	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::on_updated()
	{
		if (!_view)
			return;
		_view->fetch();
		this->invalidated(_view->size());
	}
}
