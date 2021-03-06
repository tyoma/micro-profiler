//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <vector>
#include <wpl/models.h>

namespace micro_profiler
{
	struct hive;

	class headers_model : public wpl::headers_model
	{
	public:
		struct column;
		enum sort_direction	{	dir_none, dir_ascending, dir_descending	};

	public:
		template <size_t N>
		headers_model(const column (&columns)[N], index_type sort_column, bool sort_ascending);

		void store(hive &configuration) const;
		void update(const hive &configuration);

		virtual index_type get_count() const throw() override;
		virtual void get_value(index_type index, short int &width) const throw() override;
		virtual std::pair<index_type, bool> get_sort_order() const throw() override;
		virtual void set_width(index_type index, short int width) override;
		virtual void get_caption(index_type index, agge::richtext_t &caption) const override;
		virtual void activate_column(index_type column) override;

	private:
		std::vector<column> _columns;
		index_type _sort_column;
		bool _sort_ascending;
	};


	struct headers_model::column
	{
		std::string id;
		agge::richtext_modifier_t caption;
		short int width;
		headers_model::sort_direction default_sort_direction;
	};



	template <size_t N>
	inline headers_model::headers_model(const column (&columns)[N], index_type sort_column,
			bool sort_ascending)
		: _columns(columns, columns + N), _sort_column(sort_column), _sort_ascending(sort_ascending)
	{	}
}
