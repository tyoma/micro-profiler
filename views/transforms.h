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

		template <typename Table1T, typename Table2T>
		class joined_record
		{
		public:
			joined_record() {	}
			joined_record(typename Table1T::const_iterator left_, typename Table2T::const_iterator right_) : _left(left_), _right(right_) {	}

			typename Table1T::const_reference left() const {	return *_left;	}
			typename Table2T::const_reference right() const {	return *_right;	}

		private:
			typename Table1T::const_iterator _left;
			typename Table2T::const_iterator _right;
		};



		template <typename T, typename ConstructorT, typename U, typename KeyerFactoryT, typename AggregatorT>
		inline std::shared_ptr< const table<T, ConstructorT> > group_by(const U &underlying,
			const KeyerFactoryT &keyer_factory, const ConstructorT &/*constructor*/, const AggregatorT &aggregator)
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
		inline std::shared_ptr< const table<T, C> > group_by(const table<T, C> &underlying,
			const KeyerFactoryT &keyer_factory, const AggregatorT &aggregator)
		{	return group_by<T, C>(underlying, keyer_factory, C(), aggregator);	}


		template <typename Keyer1T, typename Keyer2T, typename Table1T, typename Table2T>
		std::shared_ptr< const table< joined_record<Table1T, Table2T> > > join(const Table1T &left, const Table2T &right)
		{
			typedef joined_record<Table1T, Table2T> value_type;
			typedef table<value_type> joined_table_t;
			typedef wpl::slot_connection slot_t;
			typedef std::tuple<joined_table_t, slot_t, slot_t> composite_t;

			const auto composite = std::make_shared<composite_t>();
			auto &joined = std::get<0>(*composite);
			const auto lkeyer = Keyer1T();
			auto &lindex = multi_index(left, lkeyer);
			const auto rkeyer = Keyer2T();
			auto &rindex = multi_index(right, rkeyer);

			auto add_from_left = [&joined, lkeyer, &rindex] (typename Table1T::const_iterator i, bool new_) {
				if (!new_)
					return;
				for (auto j = rindex.equal_range(lkeyer(*i)); j.first != j.second; ++j.first)
				{
					auto r = joined.create();

					*r = value_type(i, j.first.underlying());
					r.commit();
				}
			};
			auto add_from_right = [&joined, rkeyer, &lindex] (typename Table2T::const_iterator i, bool new_) {
				if (!new_)
					return;
				for (auto j = lindex.equal_range(rkeyer(*i)); j.first != j.second; ++j.first)
				{
					auto r = joined.create();

					*r = value_type(j.first.underlying(), i);
					r.commit();
				}
			};

			for (auto i = left.begin(); i != left.end(); ++i)
				add_from_left(i, true);

			std::get<1>(*composite) = left.changed += add_from_left;
			std::get<2>(*composite) = right.changed += add_from_right;

			return std::shared_ptr<joined_table_t>(composite, &joined);
		}
	}
}
