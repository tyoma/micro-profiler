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

#include <frontend/process_list.h>

#include "text_conversion.h"

#include <algorithm>
#include <common/string.h>

using namespace std;

namespace micro_profiler
{
	void process_list::update(const process_enumerator_t &enumerator)
	{
		_processes.clear();
		enumerator([this] (const shared_ptr<process> &p) {
			_processes.push_back(p);
		});
		if (_sorter)
			_sorter(_processes);
		invalidated(_processes.size());
	}

	shared_ptr<process> process_list::get_process(index_type row) const
	{	return _processes[row];	}

	process_list::index_type process_list::get_count() const throw()
	{	return _processes.size();	}

	void process_list::get_text(index_type row, index_type column, wstring &text) const
	{
		process &p = *_processes[row];

		switch (column)
		{
		case 0:
			text = unicode(p.name());
			break;

		case 1:
			text = unicode(to_string2(p.get_pid()));
			break;
		}
	}

	void process_list::set_order(index_type column, bool ascending)
	{
		if ((0 == column) & ascending)
			init_sorter([] (const shared_ptr<process> &lhs, const shared_ptr<process> &rhs) {
				return lhs->name() < rhs->name();
			});
		else if ((0 == column) & !ascending)
			init_sorter([] (const shared_ptr<process> &lhs, const shared_ptr<process> &rhs) {
				return lhs->name() > rhs->name();
			});
		else if ((1 == column) & ascending)
			init_sorter([] (const shared_ptr<process> &lhs, const shared_ptr<process> &rhs) {
				return lhs->get_pid() < rhs->get_pid();
			});
		else if ((1 == column) & !ascending)
			init_sorter([] (const shared_ptr<process> &lhs, const shared_ptr<process> &rhs) {
				return lhs->get_pid() > rhs->get_pid();
			});
		_sorter(_processes);
	}

	template <typename PredicateT>	
	void process_list::init_sorter(const PredicateT &p)
	{
		_sorter = [p] (process_container_t &processes) {
			sort(processes.begin(), processes.end(), p);
		};
	}
}
