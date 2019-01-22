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

#include <injector/process.h>
#include <wpl/ui/models.h>

namespace micro_profiler
{
	class process_list : public wpl::ui::table_model
	{
	public:
		typedef std::function<void (const process::enumerate_callback_t &callback)> process_enumerator_t;

	public:
		void update(const process_enumerator_t &enumerator);

		std::shared_ptr<process> get_process(index_type row) const;

		virtual index_type get_count() const throw();
		virtual void get_text(index_type row, index_type column, std::wstring &text) const;
		virtual void set_order(index_type column, bool ascending);

	private:
		typedef std::vector< std::shared_ptr<process> > process_container_t;

	private:
		template <typename PredicateT>
		void init_sorter(const PredicateT &p);

	private:
		std::vector< std::shared_ptr<process> > _processes;
		std::function<void (process_container_t &processes)> _sorter;
	};
}
