//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <vector>
#include <wpl/models.h>

namespace micro_profiler
{
	struct call_statistics;
	struct hive;

	class headers_model : public wpl::headers_model
	{
	public:
		template <typename T, size_t N>
		headers_model(T (&columns)[N], index_type sort_column, bool sort_ascending);

		void store(hive &configuration) const;
		void update(const hive &configuration);

		virtual index_type get_count() const throw() override;
		virtual void get_value(index_type index, short int &width) const throw() override;
		virtual std::pair<index_type, bool> get_sort_order() const throw() override;
		virtual void set_width(index_type index, short int width) override;
		virtual agge::full_alignment get_alignment(index_type index) const override;
		virtual agge::full_alignment get_header_alignment(index_type index) const override;
		virtual void get_caption(index_type index, agge::richtext_t &caption) const override;
		virtual void activate_column(index_type column_) override;

	private:
		struct column
		{
			template <typename T, typename CtxT>
			column(const column_definition<T, CtxT> &from);

			std::string id;
			agge::richtext_modifier_t caption;
			short int width;
			agge::text_alignment alignment;
			bool compare, ascending;
		};

	private:
		std::vector<column> _columns;
		index_type _sort_column;
		bool _sort_ascending;
	};



	template <typename T, typename CtxT>
	inline headers_model::column::column(const column_definition<T, CtxT> &from)
		: caption(agge::style_modifier::empty)
	{
		id = from.id;
		caption = from.caption;
		width = from.width;
		alignment = from.alignment;
		compare = !!from.compare;
		ascending = from.ascending;
	}


	template <typename T, size_t N>
	inline headers_model::headers_model(T (&columns)[N], index_type sort_column, bool sort_ascending)
		: _columns(columns, columns + N), _sort_column(sort_column), _sort_ascending(sort_ascending)
	{	}
}
