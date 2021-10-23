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

#include "primitives.h"
#include "projection_view.h"
#include "selection_model.h"
#include "trackables_provider.h"

#include <common/noncopyable.h>
#include <views/ordered.h>

namespace micro_profiler
{
	class symbol_resolver;

	namespace tables
	{
		struct threads;
	}

	template <typename BaseT, typename U>
	class statistics_model_impl : public BaseT, noncopyable
	{
	public:
		typedef typename BaseT::index_type index_type;
		typedef typename key_traits<typename U::value_type>::key_type key_type;

	public:
		statistics_model_impl(std::shared_ptr<U> underlying, double tick_interval_,
			std::shared_ptr<symbol_resolver> resolver_, std::shared_ptr<const tables::threads> threads_);

		std::shared_ptr<U> get_underlying() const;

		// wpl::richtext_table_model methods
		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type item, index_type subitem, agge::richtext_t &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;

		// linked_statistics methods
		virtual void fetch() /*override*/;
		virtual void set_order(index_type column, bool ascending) /*override*/;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() const /*override*/;
		virtual std::shared_ptr< selection<key_type> > create_selection() const /*override*/;

		index_type get_index(key_type address) const;

	public:
		const double tick_interval;
		const std::shared_ptr<symbol_resolver> resolver;
		const std::shared_ptr<const tables::threads> threads;

	protected:
		const typename U::value_type &get_entry(index_type row) const;

	private:
		struct view_complex;

	private:
		const std::shared_ptr<view_complex> _view;
		wpl::slot_connection _symbols_invalidation;
	};

	template <typename BaseT, typename U>
	struct statistics_model_impl<BaseT, U>::view_complex
	{
		view_complex(std::shared_ptr<U> underlying_);

		std::shared_ptr<U> underlying;
		views::ordered<U> ordered;
		trackables_provider< views::ordered<U> > trackables;
		projection_view<views::ordered<U>, double> projection;
	};



	template <typename BaseT, typename U>
	inline statistics_model_impl<BaseT, U>::view_complex::view_complex(std::shared_ptr<U> underlying_)
		: underlying(underlying_), ordered(*underlying), trackables(ordered), projection(ordered)
	{	}


	template <typename BaseT, typename U>
	inline statistics_model_impl<BaseT, U>::statistics_model_impl(std::shared_ptr<U> statistics_, double tick_interval_,
			std::shared_ptr<symbol_resolver> resolver_, std::shared_ptr<const tables::threads> threads_)
		: tick_interval(tick_interval_), resolver(resolver_), threads(threads_),
			_view(std::make_shared<view_complex>(statistics_)),
			_symbols_invalidation(resolver_->invalidate += [this] {	this->invalidate(this->npos());	})
	{	}

	template <typename BaseT, typename U>
	inline std::shared_ptr<U> statistics_model_impl<BaseT, U>::get_underlying() const
	{	return _view->underlying;	}

	template <typename BaseT, typename U>
	inline typename statistics_model_impl<BaseT, U>::index_type statistics_model_impl<BaseT, U>::get_count() const throw()
	{ return _view ? _view->ordered.size() : 0u; }

	template <typename BaseT, typename U>
	inline std::shared_ptr<const wpl::trackable> statistics_model_impl<BaseT, U>::track(index_type row) const
	{	return _view->trackables.track(row);	}

	template <typename BaseT, typename U>
	inline void statistics_model_impl<BaseT, U>::fetch()
	{
		_view->ordered.fetch();
		_view->trackables.fetch();
		_view->projection.fetch();
		this->invalidate(this->npos());
	}

	template <typename BaseT, typename U>
	inline std::shared_ptr< wpl::list_model<double> > statistics_model_impl<BaseT, U>::get_column_series() const
	{	return std::shared_ptr< wpl::list_model<double> >(_view, &_view->projection);	}

	template <typename BaseT, typename U>
	inline std::shared_ptr< selection<typename statistics_model_impl<BaseT, U>::key_type> > statistics_model_impl<BaseT, U>::create_selection() const
	{
		typedef std::pair< std::shared_ptr<view_complex>, std::shared_ptr< selection<key_type> > > selection_complex_t;

		auto complex = std::make_shared<selection_complex_t>(_view,
			std::make_shared< selection_model< views::ordered<U> > >(_view->ordered));

		return std::shared_ptr< selection<key_type> >(complex, complex->second.get());
	}

	template <typename BaseT, typename U>
	inline typename statistics_model_impl<BaseT, U>::index_type statistics_model_impl<BaseT, U>::get_index(key_type key) const
	{
		for (size_t i = 0, count = _view->ordered.size(); i != count; ++i)
		{
			if (_view->ordered[i].first == key)
				return i;
		}
		return this->npos();
	}

	template <typename BaseT, typename U>
	inline const typename U::value_type &statistics_model_impl<BaseT, U>::get_entry(index_type row) const
	{	return _view->ordered[row];	}
}
