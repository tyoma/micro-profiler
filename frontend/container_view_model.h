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

#include "projection_view.h"
#include "selection_model.h"
#include "trackables_provider.h"

#include <common/noncopyable.h>
#include <views/ordered.h>

namespace micro_profiler
{
	template <typename BaseT, typename U>
	class container_view_model : public BaseT, public std::enable_shared_from_this< container_view_model<BaseT, U> >,
		noncopyable
	{
	public:
		typedef typename BaseT::index_type index_type;
		typedef typename key_traits<typename U::value_type>::key_type key_type;
		struct column
		{
			template <typename CompareT> column &on_set_order(const CompareT &compare);
			template <typename ProjectT> column &on_project(const ProjectT &project);

			container_view_model &owner;
			std::function<void (agge::richtext_t &text, index_type row)> get_text;
			std::function<void (bool ascending)> set_order;
			std::function<void ()> set_projection;
		};

	public:
		container_view_model(std::shared_ptr<U> underlying);

		U &underlying();

		template <typename GetTextT>
		column &add_column(const GetTextT &get_text_);

		// wpl::richtext_table_model methods
		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type item, index_type subitem, agge::richtext_t &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;

		// linked_statistics methods
		virtual void fetch() /*override*/;
		virtual void set_order(index_type column, bool ascending) /*override*/;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() /*override*/;
		virtual std::shared_ptr< selection<key_type> > create_selection() const /*override*/;

	protected:
		const typename U::value_type &get_entry(index_type row) const;

	private:
		const std::shared_ptr<U> _underlying;
		views::ordered<U> _ordered;
		trackables_provider< views::ordered<U> > _trackables;
		projection_view<views::ordered<U>, double> _projection;
		std::vector<column> _columns;
	};



	template <typename BaseT, typename U>
	template <typename CompareT>
	inline typename container_view_model<BaseT, U>::column &container_view_model<BaseT, U>::column::on_set_order(const CompareT &compare)
	{
		auto &owner_ = owner;
		set_order = [&owner_, compare] (bool ascending) {	owner_._ordered.set_order(compare, ascending);	};
		return *this;
	}

	template <typename BaseT, typename U>
	template <typename ProjectT>
	inline typename container_view_model<BaseT, U>::column &container_view_model<BaseT, U>::column::on_project(const ProjectT &project)
	{
		auto &owner_ = owner;
		set_projection = [&owner_, project] {	owner_._projection.project(project);	};
		return *this;
	}


	template <typename BaseT, typename U>
	inline container_view_model<BaseT, U>::container_view_model(std::shared_ptr<U> underlying)
		: _underlying(underlying), _ordered(*_underlying), _trackables(_ordered), _projection(_ordered)
	{	}

	template <typename BaseT, typename U>
	inline U &container_view_model<BaseT, U>::underlying()
	{	return *_underlying;	}

	template <typename BaseT, typename U>
	template <typename GetTextT>
	inline typename container_view_model<BaseT, U>::column &container_view_model<BaseT, U>::add_column(const GetTextT &get_text_)
	{
		column ops = {
			*this,
			[this, get_text_] (agge::richtext_t &text, index_type row) {	get_text_(text, row, _ordered[row]);	},
		};

		_columns.push_back(ops);
		return _columns.back();
	}

	template <typename BaseT, typename U>
	inline typename container_view_model<BaseT, U>::index_type container_view_model<BaseT, U>::get_count() const throw()
	{	return _ordered.size();	}

	template <typename BaseT, typename U>
	inline void container_view_model<BaseT, U>::get_text(index_type row, index_type column, agge::richtext_t &text) const
	{
		if (column < _columns.size())
			_columns[column].get_text(text, row);
	}

	template <typename BaseT, typename U>
	inline std::shared_ptr<const wpl::trackable> container_view_model<BaseT, U>::track(index_type row) const
	{	return _trackables.track(row);	}

	template <typename BaseT, typename U>
	inline void container_view_model<BaseT, U>::fetch()
	{
		_ordered.fetch();
		_trackables.fetch();
		_projection.fetch();
		this->invalidate(this->npos());
	}

	template <typename BaseT, typename U>
	inline void container_view_model<BaseT, U>::set_order(index_type column, bool ascending)
	{
		const auto &c = _columns.at(column);

		if (c.set_order)
			c.set_order(ascending), this->invalidate(this->npos());
		if (c.set_projection)
			c.set_projection();
		else
			_projection.project();
		_trackables.fetch();
		_projection.fetch();
	}

	template <typename BaseT, typename U>
	inline std::shared_ptr< wpl::list_model<double> > container_view_model<BaseT, U>::get_column_series()
	{	return std::shared_ptr< wpl::list_model<double> >(this->shared_from_this(), &_projection);	}

	template <typename BaseT, typename U>
	inline std::shared_ptr< selection<typename container_view_model<BaseT, U>::key_type> > container_view_model<BaseT, U>::create_selection() const
	{
		typedef std::pair< std::shared_ptr<const container_view_model>, selection_model< views::ordered<U> > > selection_complex_t;

		auto complex = std::make_shared<selection_complex_t>(this->shared_from_this(),
			selection_model< views::ordered<U> >(_ordered));

		return std::shared_ptr< selection<key_type> >(complex, &complex->second);
	}

	template <typename BaseT, typename U>
	inline const typename U::value_type &container_view_model<BaseT, U>::get_entry(index_type row) const
	{	return _ordered[row];	}
}
