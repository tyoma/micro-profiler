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

#include <frontend/process_list.h>

#include <algorithm>
#include <common/formatting.h>

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
		invalidate(npos());
	}

	shared_ptr<process> process_list::get_process(index_type row) const
	{	return _processes[row];	}

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

	process_list::index_type process_list::get_count() const throw()
	{	return _processes.size();	}

	void process_list::get_text(index_type row, index_type column, agge::richtext_t &text) const
	{
		process &p = *_processes[row];

		switch (column)
		{
		case 0:	text << p.name().c_str();	break;
		case 1:	itoa<10>(text, p.get_pid());	break;
		}
	}

	template <typename PredicateT>	
	void process_list::init_sorter(const PredicateT &p)
	{
		_sorter = [p] (process_container_t &processes) {
			sort(processes.begin(), processes.end(), p);
		};
	}
}
