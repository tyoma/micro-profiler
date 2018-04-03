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
#include "symbol_resolver.h"

#include <wpl/ui/listview.h>

namespace micro_profiler
{
	template <typename BaseT, typename MapT>
	class statistics_model_impl : public BaseT, public std::enable_shared_from_this< statistics_model_impl<BaseT, MapT> >
	{
	public:
		typedef typename BaseT::index_type index_type;

	public:
		statistics_model_impl(const MapT &statistics, double tick_interval, std::shared_ptr<symbol_resolver> resolver);

		virtual index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual std::shared_ptr<const wpl::ui::listview::trackable> track(index_type row) const;

		virtual index_type get_index(address_t address) const;

		virtual address_t get_address(index_type item) const;

	protected:
		typedef ordered_view<MapT> view_type;

	protected:
		const view_type &view() const;
		void updated();

	private:
		double _tick_interval;
		std::shared_ptr<symbol_resolver> _resolver;
		ordered_view<MapT> _view;
	};



	template <typename BaseT, typename MapT>
	inline statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, double tick_interval, std::shared_ptr<symbol_resolver> resolver)
		: _view(statistics), _tick_interval(tick_interval), _resolver(resolver)
	{ }

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_count() const throw()
	{ return _view.size(); }

	template <typename BaseT, typename MapT>
	inline std::shared_ptr<const wpl::ui::listview::trackable> statistics_model_impl<BaseT, MapT>::track(index_type row) const
	{
		using namespace wpl::ui;

		class trackable : public listview::trackable
		{
			std::weak_ptr<const statistics_model_impl> _model;
			address_t _address;

		public:
			trackable(std::weak_ptr<const statistics_model_impl> model, address_t address)
				: _model(model), _address(address)
			{	}

			virtual listview::index_type index() const
			{
				if (std::shared_ptr<const statistics_model_impl> model = _model.lock())
					return std::shared_ptr<const statistics_model_impl>(_model)->get_index(_address);
				return static_cast<listview::index_type>(-1);
			}
		};

		return std::shared_ptr<const listview::trackable>(new trackable(shared_from_this(), _view.at(row).first));
	}

	template <typename BaseT, typename MapT>
	inline typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_index(address_t address) const
	{	return _view.find_by_key(address);	}

	template <typename BaseT, typename MapT>
	inline address_t statistics_model_impl<BaseT, MapT>::get_address(index_type item) const
	{	return _view.at(item).first;	}

	template <typename BaseT, typename MapT>
	inline const typename statistics_model_impl<BaseT, MapT>::view_type &statistics_model_impl<BaseT, MapT>::view() const
	{	return _view;	}

	template <typename BaseT, typename MapT>
	inline void statistics_model_impl<BaseT, MapT>::updated()
	{
		_view.resort();
		invalidated(_view.size());
	}
}
