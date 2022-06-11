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
#include "dynamic_views.h"
#include "model_context.h"
#include "models.h"
#include "symbol_resolver.h"
#include "table_model_impl.h"
#include "tables.h"

#include <views/filter.h>
#include <views/integrated_index.h>

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

	struct stack_hierarchy_access
	{
		stack_hierarchy_access(const statistics_model_context &ctx)
			: _by_id(ctx.by_id)
		{	}

		static bool same_parent(const call_statistics &lhs, const call_statistics &rhs)
		{	return lhs.parent_id == rhs.parent_id;	}

		const std::vector<id_t> &path(const call_statistics &item) const
		{	return item.path(_by_id);	}

		const call_statistics &lookup(id_t id) const
		{	return *_by_id(id);	}

		static bool total_less(const call_statistics &lhs, const call_statistics &rhs)
		{	return lhs.id < rhs.id;	}

	private:
		std::function<const call_statistics *(id_t id)> _by_id;
	};



	inline stack_hierarchy_access access_hierarchy(const statistics_model_context &ctx, const call_statistics * /*tag*/)
	{	return stack_hierarchy_access(ctx);	}

	template <typename U>
	inline statistics_model_context create_context(std::shared_ptr<U> underlying, double tick_interval,
		std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<const tables::threads> threads, bool canonical)
	{
		auto &by_id = views::unique_index<keyer::id>(*underlying);

		return initialize<statistics_model_context>(tick_interval, [underlying, &by_id] (id_t id) {
			return by_id.find(id);
		}, [threads] (id_t id) -> const thread_info * {
			auto i = threads->find(id);
			return i != threads->end() ? &i->second : nullptr;
		}, resolver, canonical);
	}

	template <typename BaseT, typename U, typename ColumnsT>
	inline std::shared_ptr< table_model_impl<BaseT, U, statistics_model_context> > make_table(std::shared_ptr<U> underlying,
		const statistics_model_context &context, const ColumnsT &columns)
	{
		typedef table_model_impl<BaseT, U, statistics_model_context> model_type;
		typedef std::tuple<std::shared_ptr<model_type>, wpl::slot_connection, wpl::slot_connection> composite_t;

		const auto m = std::make_shared<model_type>(underlying, context);
		const auto c = std::make_shared<composite_t>(m, underlying->invalidate += [m] {
			m->fetch();
		}, context.resolver->invalidate += [m] {
			m->invalidate(m->npos());
		});

		m->add_columns(columns);
		return std::shared_ptr<model_type>(c, std::get<0>(*c).get());
	}

	template <typename U, typename CtxT>
	inline std::shared_ptr< table_model_impl<table_model<id_t>, U, CtxT> >
		create_statistics_model(std::shared_ptr<U> underlying, const CtxT &context)
	{	return make_table< table_model<id_t> >(underlying, context, c_statistics_columns);	}
}
