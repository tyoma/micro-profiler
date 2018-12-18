//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/symbol_resolver.h>

#include <wpl/ui/models.h>

namespace micro_profiler
{
	template <typename BaseT, typename MapT>
	class statistics_model_impl : public BaseT
	{
	public:
		typedef typename BaseT::index_type index_type;

	public:
		statistics_model_impl(const MapT &statistics, double tick_interval);

		void set_resolver(const std::shared_ptr<symbol_resolver> &resolver);

		std::shared_ptr< series<double> > get_column_series() const;

		virtual void detach() throw();

		virtual index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual std::shared_ptr<const wpl::ui::trackable> track(index_type row) const;

		virtual index_type get_index(address_t address) const;

		virtual address_t get_address(index_type item) const;

	protected:
		typedef ordered_view<MapT> view_type;

	protected:
		const typename MapT::value_type &get_entry(index_type row) const;
		virtual void on_updated();

	private:
		double _tick_interval;
		std::shared_ptr<symbol_resolver> _resolver;
		std::shared_ptr< ordered_view<MapT> > _view;
	};



	template <typename BaseT, typename MapT>
	inline statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, double tick_interval)
		: _tick_interval(tick_interval), _view(new ordered_view<MapT>(statistics))
	{ }

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::set_resolver(const std::shared_ptr<symbol_resolver> &resolver)
	{	_resolver = resolver;	}

	template <typename BaseT, typename MapT>
	std::shared_ptr< series<double> > statistics_model_impl<BaseT, MapT>::get_column_series() const
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
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_index(address_t address) const
	{	return _view->find_by_key(address);	}

	template <typename BaseT, typename MapT>
	inline address_t statistics_model_impl<BaseT, MapT>::get_address(index_type item) const
	{	return get_entry(item).first;	}

	template <typename BaseT, typename MapT>
	inline const typename MapT::value_type &statistics_model_impl<BaseT, MapT>::get_entry(index_type row) const
	{	return _view->at(row);	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::on_updated()
	{
		if (!_view)
			return;
		_view->resort();
		this->invalidated(_view->size());
	}
}
