//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "column_definition.h"
#include "projection_view.h"
#include "selection_model.h"
#include "symbol_resolver.h" // TODO: remove dependency from symbol_resolver
#include "trackables_provider.h"

#include <common/noncopyable.h>
#include <tuple>
#include <views/ordered.h>

namespace micro_profiler
{
	template <typename BaseT, typename U, typename CtxT>
	class table_model_impl : public BaseT, public std::enable_shared_from_this< table_model_impl<BaseT, U, CtxT> >,
		noncopyable
	{
	public:
		typedef typename U::value_type value_type;
		typedef column_definition<value_type, CtxT> column_type;
		typedef typename BaseT::index_type index_type;
		typedef typename key_traits<value_type>::key_type key_type;

	public:
		table_model_impl(std::shared_ptr<U> underlying, const CtxT &context_);

		const views::ordered<U> &ordered() const throw();
		const CtxT &context() const throw();
		const std::vector<column_type> &columns() const throw();
		U &underlying() throw();

		template <typename ContainerT>
		void add_columns(const ContainerT &columns_);

		void fetch();

		// wpl::richtext_table_model methods
		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type item, index_type subitem, agge::richtext_t &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;

		// table_model<...> methods
		virtual void set_order(index_type column, bool ascending) /*override*/;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() /*override*/;
		virtual std::shared_ptr< selection<key_type> > create_selection() const /*override*/;

	private:
		const std::shared_ptr<U> _underlying;
		const CtxT _context;
		views::ordered<U> _ordered;
		trackables_provider< views::ordered<U> > _trackables;
		projection_view<views::ordered<U>, double> _projection;
		std::vector<column_type> _columns;
	};



	template <typename BaseT, typename U, typename CtxT>
	inline table_model_impl<BaseT, U, CtxT>::table_model_impl(std::shared_ptr<U> underlying, const CtxT &context_)
		: _underlying(underlying), _context(context_), _ordered(*_underlying), _trackables(_ordered), _projection(_ordered)
	{	}

	template <typename BaseT, typename U, typename CtxT>
	inline const views::ordered<U> &table_model_impl<BaseT, U, CtxT>::ordered() const throw()
	{	return _ordered;	}

	template <typename BaseT, typename U, typename CtxT>
	inline const CtxT &table_model_impl<BaseT, U, CtxT>::context() const throw()
	{	return _context;	}

	template <typename BaseT, typename U, typename CtxT>
	inline const std::vector<typename table_model_impl<BaseT, U, CtxT>::column_type> &table_model_impl<BaseT, U, CtxT>::columns() const throw()
	{	return _columns;	}

	template <typename BaseT, typename U, typename CtxT>
	inline U &table_model_impl<BaseT, U, CtxT>::underlying() throw()
	{	return *_underlying;	}

	template <typename BaseT, typename U, typename CtxT>
	template <typename ContainerT>
	inline void table_model_impl<BaseT, U, CtxT>::add_columns(const ContainerT &columns_)
	{	_columns.insert(_columns.end(), std::begin(columns_), std::end(columns_));	}

	template <typename BaseT, typename U, typename CtxT>
	inline void table_model_impl<BaseT, U, CtxT>::fetch()
	{
		_ordered.fetch();
		_trackables.fetch();
		_projection.fetch();
		this->invalidate(this->npos());
	}

	template <typename BaseT, typename U, typename CtxT>
	inline typename table_model_impl<BaseT, U, CtxT>::index_type table_model_impl<BaseT, U, CtxT>::get_count() const throw()
	{	return _ordered.size();	}

	template <typename BaseT, typename U, typename CtxT>
	inline void table_model_impl<BaseT, U, CtxT>::get_text(index_type row, index_type column, agge::richtext_t &text) const
	{
		if (column < _columns.size())
			_columns[column].get_text(text, _context, row, _ordered[row]);
	}

	template <typename BaseT, typename U, typename CtxT>
	inline std::shared_ptr<const wpl::trackable> table_model_impl<BaseT, U, CtxT>::track(index_type row) const
	{	return _trackables.track(row);	}

	template <typename BaseT, typename U, typename CtxT>
	inline void table_model_impl<BaseT, U, CtxT>::set_order(index_type column, bool ascending)
	{
		const auto &context_ = _context;
		const auto &c = _columns.at(column);
		const auto &less = c.less;
		const auto &get_value = c.get_value;

		if (less)
		{
			_ordered.set_order([&context_, less] (const value_type &lhs, const value_type &rhs) {
				return less(context_, lhs, rhs);
			}, ascending);
			this->invalidate(this->npos());
		}
		if (get_value)
			_projection.project([&context_, get_value] (const value_type &item) {	return get_value(context_, item);	});
		else
			_projection.project();
		_trackables.fetch();
		_projection.fetch();
	}

	template <typename BaseT, typename U, typename CtxT>
	inline std::shared_ptr< wpl::list_model<double> > table_model_impl<BaseT, U, CtxT>::get_column_series()
	{	return std::shared_ptr< wpl::list_model<double> >(this->shared_from_this(), &_projection);	}

	template <typename BaseT, typename U, typename CtxT>
	inline std::shared_ptr< selection<typename table_model_impl<BaseT, U, CtxT>::key_type> > table_model_impl<BaseT, U, CtxT>::create_selection() const
	{
		typedef std::pair< std::shared_ptr<const table_model_impl>, selection_model< views::ordered<U> > > selection_complex_t;

		auto complex = std::make_shared<selection_complex_t>(this->shared_from_this(),
			selection_model< views::ordered<U> >(_ordered));

		return std::shared_ptr< selection<key_type> >(complex, &complex->second);
	}


	template <typename BaseT, typename U, typename CtxT, typename ColumnsT>
	inline std::shared_ptr< table_model_impl<BaseT, U, CtxT> > make_table(std::shared_ptr<U> underlying,
		const CtxT &context, const ColumnsT &columns)
	{
		typedef table_model_impl<BaseT, U, CtxT> model_type;
		typedef std::tuple<std::shared_ptr<model_type>, wpl::slot_connection, wpl::slot_connection> complex_type;

		const auto m = std::make_shared<model_type>(underlying, context);
		const auto p = m.get();
		const auto c = std::make_shared<complex_type>(m, underlying->invalidate += [p] {
			p->fetch();
		}, context.resolver->invalidate += [p] {
			p->invalidate(p->npos());
		});

		m->add_columns(columns);
		return std::shared_ptr<model_type>(c, std::get<0>(*c).get());
	}
}