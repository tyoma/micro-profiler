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

#include "constructors.h"
#include "keyer.h"
#include "model_context.h"
#include "symbol_resolver.h"
#include "table_model_impl.h"

namespace micro_profiler
{
	namespace views
	{
		template <typename Table1T, typename Table2T>
		class joined_record;
	}

	class symbol_resolver;

	template <>
	struct key_traits<call_statistics>
	{
		typedef id_t key_type;

		static key_type get_key(const call_statistics &value)
		{	return value.id;	}
	};

	template <typename Table1T, typename Table2T>
	struct key_traits< sdb::joined_record<Table1T, Table2T> >
	{
		typedef typename key_traits<typename Table1T::value_type>::key_type key_type;
	
		static key_type get_key(const sdb::joined_record<Table1T, Table2T> &value)
		{	return key_traits<typename Table1T::value_type>::get_key(value);	}
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
		auto &by_id = sdb::unique_index<keyer::id>(*underlying);

		return initialize<statistics_model_context>(tick_interval, [underlying, &by_id] (id_t id) -> const call_statistics * {
			auto r = by_id.find(id);
			return r ? &(const call_statistics &)*r : nullptr;
		}, [threads] (id_t id) -> const thread_info * {
			return sdb::unique_index<keyer::external_id>(*threads).find(id);
		}, resolver, canonical);
	}

	template <typename BaseT, typename U, typename ColumnsT>
	inline std::shared_ptr< table_model_impl<BaseT, U, statistics_model_context, call_statistics> > make_table(
		std::shared_ptr<U> underlying, const statistics_model_context &context, const ColumnsT &columns)
	{
		const auto m = std::make_shared<table_model_impl<BaseT, U, statistics_model_context, call_statistics>>(underlying,
			context);

		m->add_columns(columns);
		return make_shared_aspect(make_shared_copy(make_tuple(m, underlying->invalidate += [m] {
			m->fetch();
		}, context.resolver->invalidate += [m] {
				m->invalidate(m->npos());
		})), m.get());
	}
}
