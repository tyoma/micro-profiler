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

#include "columns_layout.h"
#include "constructors.h"
#include "container_view_model.h"
#include "model_context.h"
#include "models.h"
#include "nested_statistics_model.h"
#include "nested_transform.h"
#include "tables.h"

#include <views/filter.h>

namespace micro_profiler
{
	class symbol_resolver;

	template <>
	struct key_traits<call_statistics>
	{
		typedef id_t key_type;

		static key_type get_key(const call_statistics &value)
		{	return value.id;	}
	};

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

	private:
		wpl::slot_connection _invalidation_connections[2];
	};



	template <typename U>
	inline statistics_model_context create_context(std::shared_ptr<U> underlying, double tick_interval,
		std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads, bool canonical)
	{
		return initialize<statistics_model_context>(tick_interval,
			[underlying] (id_t id) {	return underlying->by_id.find(id);	}, threads, resolver, canonical);
	}


	template <typename U>
	inline std::shared_ptr<linked_statistics> create_callees_model(std::shared_ptr<U> underlying, double tick_interval,
		std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<id_t> > scope)
	{
		return construct_nested<callees_transform>(create_context(underlying, tick_interval, resolver, threads, false),
			underlying, scope, c_callee_statistics_columns);
	}


	template <typename U>
	inline std::shared_ptr<linked_statistics> create_callers_model(std::shared_ptr<U> underlying, double tick_interval,
		std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads,
		std::shared_ptr< std::vector<id_t> > scope)
	{
		return construct_nested<callers_transform>(create_context(underlying, tick_interval, resolver, threads, false),
			underlying, scope, c_caller_statistics_columns);
	}


	inline functions_list::functions_list(std::shared_ptr<tables::statistics> statistics, double tick_interval,
			std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads)
		: container_view_model<wpl::richtext_table_model, views::filter<tables::statistics>, statistics_model_context>(
			make_bound< views::filter<tables::statistics> >(statistics),
			create_context(statistics, tick_interval, resolver, threads, false))
	{
		add_columns(c_statistics_columns);

		_invalidation_connections[0] = statistics->invalidate += [this] {	fetch();	};
		_invalidation_connections[1] = resolver->invalidate += [this] {	this->invalidate(this->npos());	};
	}

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
