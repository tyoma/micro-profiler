//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "filter_view.h"
#include "ordered_view.h"
#include "primitives.h"
#include "projection_view.h"
#include "selection_model.h"
#include "trackables_provider.h"

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
		typedef typename MapT::key_type key_type;

	public:
		statistics_model_impl(const MapT &statistics, double tick_interval,
			const std::shared_ptr<symbol_resolver> &resolver, const std::shared_ptr<threads_model> &threads);

		std::shared_ptr<symbol_resolver> get_resolver() const throw();
		std::shared_ptr<threads_model> get_threads() const throw();
		std::shared_ptr< wpl::list_model<double> > get_column_series() const throw();
		std::shared_ptr< selection<typename MapT::key_type> > create_selection() const;

		template <typename PredicateT>
		void set_filter(const PredicateT &predicate);
		void set_filter();

		virtual void detach() throw();

		// wpl::richtext_table_model methods
		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type item, index_type subitem, agge::richtext_t &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;

		// linked_statistics methods
		virtual void set_order(index_type column, bool ascending) /*override*/;
		virtual key_type get_key(index_type item) const;

		index_type get_index(key_type address) const;

	protected:
		typedef filter_view<MapT> filter_view_type;
		typedef ordered_view<filter_view_type> view_type;

	protected:
		const typename MapT::value_type &get_entry(index_type row) const;
		virtual void on_updated();

	protected:
		double _tick_interval;

	private:
		filter_view_type _filter;
		std::shared_ptr<view_type> _view;
		std::shared_ptr< trackables_provider<view_type> > _trackables;
		std::shared_ptr< projection_view<view_type, double> > _projection;
		const std::shared_ptr<symbol_resolver> _resolver;
		const std::shared_ptr<threads_model> _threads;
	};



	template <typename BaseT, typename MapT>
	inline statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, double tick_interval,
			const std::shared_ptr<symbol_resolver> &resolver, const std::shared_ptr<threads_model> &threads)
		: _tick_interval(tick_interval), _filter(statistics), _view(new ordered_view< filter_view<MapT> >(_filter)),
			_trackables(std::make_shared< trackables_provider<view_type> >(*_view)),
			_projection(std::make_shared< projection_view<view_type, double> >(*_view)), _resolver(resolver),
			_threads(threads)
	{ }

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<symbol_resolver> statistics_model_impl<BaseT, MapT>::get_resolver() const throw()
	{	return _resolver;	}

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<threads_model> statistics_model_impl<BaseT, MapT>::get_threads() const throw()
	{	return _threads;	}

	template <typename BaseT, typename MapT>
	inline std::shared_ptr< wpl::list_model<double> > statistics_model_impl<BaseT, MapT>::get_column_series() const throw()
	{	return _projection;	}

	template <typename BaseT, typename MapT>
	inline std::shared_ptr< selection<typename MapT::key_type> > statistics_model_impl<BaseT, MapT>::create_selection() const
	{	return std::make_shared< selection_model<view_type> >(*_view);	}

	template <typename BaseT, typename MapT>
	template <typename PredicateT>
	inline void statistics_model_impl<BaseT, MapT>::set_filter(const PredicateT &predicate)
	{
		_filter.set_filter(predicate);
		on_updated();
	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::set_filter()
	{
		_filter.set_filter();
		on_updated();
	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::detach() throw()
	{
		_view.reset();
		this->invalidate(this->npos());
	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_count() const throw()
	{ return _view ? _view->size() : 0u; }

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<const wpl::trackable> statistics_model_impl<BaseT, MapT>::track(index_type row) const
	{	return _trackables->track(row);	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::key_type statistics_model_impl<BaseT, MapT>::get_key(index_type item) const
	{	return get_entry(item).first;	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_index(key_type key) const
	{
		for (size_t i = 0, count = _view->size(); i != count; ++i)
		{
			if ((*_view)[i].first == key)
				return i;
		}
		return this->npos();
	}

	template <typename BaseT, typename MapT>
	inline const typename MapT::value_type &statistics_model_impl<BaseT, MapT>::get_entry(index_type row) const
	{	return (*_view)[row];	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::on_updated()
	{
		if (!_view)
			return;
		_view->fetch();
		_trackables->fetch();
		_projection->fetch();
		this->invalidate(this->npos());
	}
}
