//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <wpl/ui/models.h>

namespace strmd
{
	template <typename, bool>
	struct container_reader;
}

namespace micro_profiler
{
	struct linked_statistics : wpl::ui::table_model
	{
		virtual address_t get_address(index_type item) const = 0;
		virtual std::shared_ptr< series<double> > get_column_series() const = 0;
	};

	struct linked_statistics_ex : linked_statistics
	{
		virtual void on_updated() = 0;
		virtual void detach() = 0;
		virtual void set_resolver(const std::shared_ptr<symbol_resolver> &resolver) = 0;
	};

	class functions_list : public statistics_model_impl<wpl::ui::table_model, statistic_types::map_detailed>
	{
	public:
		virtual ~functions_list();

		void clear();
		void print(std::string &content) const;
		std::shared_ptr<linked_statistics> watch_children(index_type item) const;
		std::shared_ptr<linked_statistics> watch_parents(index_type item) const;
		std::shared_ptr<symbol_resolver> get_resolver() const;

		static std::shared_ptr<functions_list> create(timestamp_t ticks_per_second,
			std::shared_ptr<symbol_resolver> resolver);

		template <typename ArchiveT>
		void save(ArchiveT &archive) const;

		template <typename ArchiveT>
		static std::shared_ptr<functions_list> load(ArchiveT &archive);

	public:
		bool updates_enabled;

	private:
		typedef statistics_model_impl<wpl::ui::table_model, statistic_types::map_detailed> base;
		typedef std::list<linked_statistics_ex *> linked_statistics_list_t;

	private:
		functions_list(std::shared_ptr<statistic_types::map_detailed> statistics, double tick_interval,
			std::shared_ptr<symbol_resolver> resolver);

		void on_updated();

	private:
		std::shared_ptr<statistic_types::map_detailed> _statistics;
		std::shared_ptr<linked_statistics_list_t> _linked;
		double _tick_interval;
		std::shared_ptr<symbol_resolver> _resolver;

	private:
		friend struct functions_list_reader;
	};



	template <typename ArchiveT>
	inline void functions_list::save(ArchiveT &archive) const
	{
		archive(static_cast<timestamp_t>(1 / _tick_interval));
		archive(*_resolver);
		archive(*_statistics);
	}

	template <typename ArchiveT>
	inline std::shared_ptr<functions_list> functions_list::load(ArchiveT &archive)
	{
		timestamp_t ticks_per_second;
		std::shared_ptr<symbol_resolver> resolver(new symbol_resolver([] (unsigned int /*persistent_id*/) {}));

		archive(ticks_per_second);
		archive(*resolver);

		std::shared_ptr<functions_list> fl(create(ticks_per_second, resolver));

		archive(*fl->_statistics);
		fl->on_updated();
		return fl;
	}
}
