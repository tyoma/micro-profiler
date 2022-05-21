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

#pragma once

#include "integrated_index.h"

#include <tuple>

namespace micro_profiler
{
	namespace views
	{
		struct agnostic_key_tag {};
		struct underlying_key_tag : agnostic_key_tag {};
		struct aggregated_key_tag : agnostic_key_tag {};

		template <typename T, typename ConstructorT, typename U, typename KeyerFactoryT, typename AggregatorT>
		inline std::shared_ptr< table<T, ConstructorT> > group_by(const U &underlying, const KeyerFactoryT &keyer_factory,
			const ConstructorT &/*constructor*/, const AggregatorT &aggregator)
		{
			typedef table<T, ConstructorT> aggregated_table_t;
			typedef wpl::slot_connection slot_t;
			typedef std::tuple<aggregated_table_t, slot_t, slot_t, slot_t> composite_t;

			const auto composite = std::make_shared<composite_t>();
			auto &aggregate = std::get<0>(*composite);
			const auto ukeyer = keyer_factory(underlying, underlying_key_tag());
			const auto &uindex = multi_index(underlying, ukeyer);
			auto &aindex = unique_index(aggregate, keyer_factory(aggregate, aggregated_key_tag()));
			const auto update_record = [aggregator, ukeyer, &uindex, &aindex] (typename U::const_reference value) {
				const auto &key = ukeyer(value);
				const auto uitems = uindex.equal_range(key);
				auto aggregated_record = aindex[key];

				aggregator(*aggregated_record, uitems.first, uitems.second);
				aggregated_record.commit();
			};

			std::get<1>(*composite) = underlying.cleared += [&aggregate] {	aggregate.clear();	};
			std::get<2>(*composite) = underlying.changed += [update_record] (typename U::const_iterator r, bool) {
				update_record(*r);
			};
			std::get<3>(*composite) = underlying.invalidate += [&aggregate] {	aggregate.invalidate();	};
			for (auto i = underlying.begin(); i != underlying.end(); ++i)
				update_record(*i);
			return std::shared_ptr<aggregated_table_t>(composite, &aggregate);
		}

		template <typename T, typename C, typename KeyerFactoryT, typename AggregatorT>
		inline std::shared_ptr< table<T, C> > group_by(const table<T, C> &underlying, const KeyerFactoryT &keyer_factory,
			const AggregatorT &aggregator)
		{	return group_by<T, C>(underlying, keyer_factory, C(), aggregator);	}
	}
}
