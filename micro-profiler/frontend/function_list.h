//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "statistics_model.h"

#include "primitives.h"

#include <wpl/ui/listview.h>
#include <string>
#include <memory>

namespace micro_profiler
{
	struct symbol_resolver;

	struct linked_statistics : wpl::ui::listview::model
	{
		virtual address_t get_address(index_type item) const = 0;
	};

	class functions_list : public statistics_model_impl<wpl::ui::listview::model, statistics_map_detailed>
	{
	public:
		void clear();
		void print(std::wstring &content) const;
		std::shared_ptr<linked_statistics> watch_children(index_type item) const;
		std::shared_ptr<linked_statistics> watch_parents(index_type item) const;

		static std::shared_ptr<functions_list> create(timestamp_t ticks_per_second, std::shared_ptr<symbol_resolver> resolver);

	private:
		functions_list(std::shared_ptr<statistics_map_detailed> statistics, double tick_interval, std::shared_ptr<symbol_resolver> resolver);

	private:
		std::shared_ptr<statistics_map_detailed> _statistics;
		double _tick_interval;
		std::shared_ptr<symbol_resolver> _resolver;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, functions_list &data);
	};

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, functions_list &data)
	{
		archive(*data._statistics);
		data.updated();
	}
}
