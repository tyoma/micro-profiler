//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "columns_model.h"

#include "../common/configuration.h"

using namespace std;

namespace micro_profiler
{
	void columns_model::store(hive &configuration) const
	{
		configuration.store("OrderBy", _sort_column != npos ? _sort_column : -1);
		configuration.store("OrderDirection", _sort_ascending ? 1 : 0);
		for (vector<column>::const_iterator i = _columns.begin(); i != _columns.end(); ++i)
		{
			shared_ptr<hive> cc = configuration.create(i->id.c_str());

			cc->store("Caption", i->caption.c_str());
		}
	}

	columns_model::index_type columns_model::get_count() const throw()
	{	return static_cast<index_type>(_columns.size());	}

	void columns_model::get_column(index_type index, wpl::ui::listview::columns_model::column &column) const
	{	column = _columns[index].caption;	}

	pair<columns_model::index_type, bool> columns_model::get_sort_order() const throw()
	{	return make_pair(_sort_column, _sort_ascending);	}

	void columns_model::activate_column(index_type column_)
	{
		const column &activated = _columns[column_];

		if (activated.default_sort_direction != dir_none)
		{
			_sort_ascending = _sort_column == column_ ?
				!_sort_ascending : activated.default_sort_direction == dir_ascending;
			_sort_column = column_;
			sort_order_changed(_sort_column, _sort_ascending);
		}
	}
}
