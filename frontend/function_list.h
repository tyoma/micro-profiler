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

#include "statistics_model.h"

#include <common/pool_allocator.h>
#include <views/aggregate.h>
#include <views/filter.h>

namespace micro_profiler
{
	namespace tables
	{
		struct statistics;
		struct threads;
	}

	struct linked_statistics : wpl::richtext_table_model
	{
		virtual ~linked_statistics() {	}
		virtual void fetch() = 0;
		virtual void set_order(index_type column, bool ascending) = 0;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() const = 0;
		virtual std::shared_ptr< selection<statistic_types::key> > create_selection() const = 0;
	};

	std::shared_ptr<linked_statistics> create_callees_model(std::shared_ptr<const tables::statistics> underlying,
		double tick_interval, std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<statistic_types::key> > scope);

	std::shared_ptr<linked_statistics> create_callers_model(std::shared_ptr<const tables::statistics> underlying,
		double tick_interval, std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<statistic_types::key> > scope);

	struct functions_list_provider
	{
		struct sum
		{
			typedef std::pair<statistic_types::key, statistic_types::function_detailed> aggregated_type;

			aggregated_type operator ()(const statistic_types::map_detailed::value_type &value) const;
			void operator ()(aggregated_type &group, const statistic_types::map_detailed::value_type &value) const;
		};

		typedef views::filter<statistic_types::map_detailed> filter_type;
		typedef views::aggregate<filter_type, sum> aggregate_type;
		typedef aggregate_type::value_type value_type;
		typedef aggregate_type::const_iterator const_iterator;
		typedef aggregate_type::const_reference const_reference;

		functions_list_provider(std::shared_ptr<tables::statistics> statistics_);

		const_iterator begin() const;
		const_iterator end() const;

		std::shared_ptr<const tables::statistics> statistics;
		filter_type filter;
		default_allocator allocator_base;
		pool_allocator allocator;
		aggregate_type aggregate;
		wpl::slot_connection connection;
	};

	class functions_list : public statistics_model_impl<wpl::richtext_table_model, functions_list_provider>
	{
	public:
		typedef statistic_types::map_detailed::value_type value_type;

	public:
		functions_list(std::shared_ptr<tables::statistics> statistics, double tick_interval_,
			std::shared_ptr<symbol_resolver> resolver_, std::shared_ptr<const tables::threads> threads_);
		virtual ~functions_list();

		template <typename PredicateT>
		void set_filter(const PredicateT &predicate);
		void set_filter();

		void clear();
		void print(std::string &content) const;

	private:
		typedef statistics_model_impl<wpl::richtext_table_model, functions_list_provider> base;

	private:
		std::shared_ptr<tables::statistics> _statistics;
		wpl::slot_connection _connection;
	};



	template <typename PredicateT>
	inline void functions_list::set_filter(const PredicateT &predicate)
	{
		get_underlying()->filter.set_filter(predicate);
		get_underlying()->aggregate.fetch();
		fetch();
	}

	inline void functions_list::set_filter()
	{
		get_underlying()->filter.set_filter();
		get_underlying()->aggregate.fetch();
		fetch();
	}
}
