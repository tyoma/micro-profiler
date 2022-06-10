//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma warning(disable: 4180)

#include <frontend/derived_statistics.h>

#include <frontend/db.h>
#include <frontend/constructors.h>
#include <frontend/key.h>
#include <frontend/selection_model.h>
#include <views/transforms.h>

using namespace std;

namespace micro_profiler
{
	using namespace keyer;
	using namespace views;

	namespace
	{
		struct sum_functions_plain
		{
			template <typename I>
			void operator ()(call_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated.parent_id = 0;
				static_cast<function_statistics&>(aggregated) = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					static_cast<function_statistics &>(aggregated) += *i;
			}
		};

		struct void_aggregator
		{
			template <typename R, typename I>
			void operator ()(R &, I, I) const
			{	}
		};

		struct hierarchy_address
		{
			hierarchy_address(calls_statistics_table_cptr hierarchy)
				: _hierarchy(hierarchy), _by_id(views::unique_index<id>(*_hierarchy))
			{	}

			long_address_t operator ()(id_t record) const
			{	return _by_id[record].address;	}

		private:
			const calls_statistics_table_cptr _hierarchy;
			const views::immutable_unique_index<calls_statistics_table, id> &_by_id;
		};

		struct selection_address_keyer_factory
		{
			template <typename IgnoreT>
			hierarchy_address operator ()(IgnoreT &, underlying_key_tag) const
			{	return hierarchy_address(hierarchy);	}

			template <typename IgnoreT>
			self operator ()(IgnoreT &, aggregated_key_tag) const
			{	return self();	}

			const calls_statistics_table_cptr hierarchy;
		};

		struct address_keyer_factory
		{
			template <typename IgnoreT>
			combine2<parent_address, thread_id> operator ()(IgnoreT &, underlying_key_tag) const
			{	return initialize< combine2<parent_address, thread_id> >(parent_address(*hierarchy));	}

			template <typename IgnoreT>
			combine2<address, thread_id> operator ()(IgnoreT &, aggregated_key_tag) const
			{	return combine2<address, thread_id>();	}

			const calls_statistics_table_cptr hierarchy;
		};
	}

	address_table_cptr derived_statistics::addresses(shared_ptr<const selection<id_t> > selection_,
			calls_statistics_table_cptr hierarchy)
	{
		typedef tuple< shared_ptr<const address_table>, vector< shared_ptr<const void> > > composite_t;

		const auto composite = make_shared<composite_t>();
		auto &result = get<0>(*composite);
		auto &others = get<1>(*composite);

		others.push_back(selection_);
		others.push_back(hierarchy);
		result = group_by<long_address_t>(selection_->get_table(), initialize<selection_address_keyer_factory>(hierarchy),
			views::default_constructor<long_address_t>(), void_aggregator());
		return address_table_cptr(composite, &*result);
	}

	calls_statistics_table_cptr derived_statistics::callers(address_table_cptr callees,
		calls_statistics_table_cptr hierarchy)
	{
		typedef tuple< calls_statistics_table_cptr, vector< shared_ptr<const void> > > composite_t;

		const auto composite = make_shared<composite_t>();
		auto &result = get<0>(*composite);
		auto &others = get<1>(*composite);
		const auto all_instances = join<address, self>(*hierarchy, *callees);

		others.push_back(callees);
		others.push_back(hierarchy);
		others.push_back(all_instances);
		result = group_by<call_statistics>(*all_instances, initialize<address_keyer_factory>(hierarchy),
			call_statistics_constructor(), sum_functions_plain());
		return calls_statistics_table_cptr(composite, &*result);
	}
}
