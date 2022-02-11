//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "container_view_model.h"
#include "model_context.h"
#include "tables.h"

#include <views/filter.h>

namespace micro_profiler
{
	class symbol_resolver;
	namespace tables
	{
		struct threads;
	}

	template <>
	struct key_traits<call_statistics>
	{
		typedef id_t key_type;

		static key_type get_key(const call_statistics &value)
		{	return value.id;	}
	};

	struct linked_statistics : wpl::richtext_table_model
	{
		virtual ~linked_statistics() {	}
		virtual void fetch() = 0;
		virtual void set_order(index_type column, bool ascending) = 0;
		virtual std::shared_ptr< wpl::list_model<double> > get_column_series() = 0;
		virtual std::shared_ptr< selection<id_t> > create_selection() const = 0;
	};

	std::shared_ptr<linked_statistics> create_callees_model(std::shared_ptr<const tables::statistics> underlying,
		double tick_interval, std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<id_t> > scope);

	std::shared_ptr<linked_statistics> create_callers_model(std::shared_ptr<const tables::statistics> underlying,
		double tick_interval, std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<id_t> > scope);

	class functions_list : public container_view_model<wpl::richtext_table_model, views::filter<tables::statistics>, statistics_model_context>
	{
	public:
		typedef tables::statistics::value_type value_type;

	public:
		functions_list(std::shared_ptr<tables::statistics> statistics, double tick_interval,
			std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads);

		template <typename PredicateT>
		void set_filter(const PredicateT &predicate);
		void set_filter();

		void print(std::string &content) const;

	private:
		const std::shared_ptr<const tables::statistics> _statistics;
		const double _tick_interval;
		const std::shared_ptr<symbol_resolver> _resolver;
		const std::shared_ptr<const tables::threads> _threads;
		wpl::slot_connection _invalidation_connections[2];
	};



	template <typename PredicateT>
	inline void functions_list::set_filter(const PredicateT &predicate)
	{
		underlying().set_filter(predicate);
		fetch();
	}

	inline void functions_list::set_filter()
	{
		underlying().set_filter();
		fetch();
	}
}
