//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
#include "signal.h"

#include <tuple>

namespace sdb
{
	struct agnostic_key_tag {};
	struct underlying_key_tag : agnostic_key_tag {};
	struct aggregated_key_tag : agnostic_key_tag {};

	template <typename Table1T, typename Table2T>
	class joined_record;

	template <typename TableT>
	struct joined_prime {	typedef typename TableT::const_reference type;	};

	template <typename Table1T, typename Table2T, typename C>
	struct joined_prime< table<joined_record<Table1T, Table2T>, C> > {	typedef typename joined_prime<Table1T>::type type;	};

	template <typename Table1T, typename Table2T>
	class joined_record
	{
	public:
		joined_record() {	}
		joined_record(typename Table1T::const_iterator left_, typename Table2T::const_iterator right_) : _left(left_), _right(right_) {	}

		typename Table1T::const_reference left() const {	return *_left;	}
		typename Table2T::const_reference right() const {	return *_right;	}

		operator typename joined_prime<Table1T>::type() const {	return *_left;	}

	private:
		typename Table1T::const_iterator _left;
		typename Table2T::const_iterator _right;
	};



	template <typename T, typename ConstructorT, typename U, typename KeyerFactoryT, typename AggregatorT>
	inline std::shared_ptr< const table<T, ConstructorT> > group_by(const U &underlying,
		const KeyerFactoryT &keyer_factory, const ConstructorT &/*constructor*/, const AggregatorT &aggregator)
	{
		typedef table<T, ConstructorT> aggregated_table_t;

		const auto composite = std::make_shared< std::tuple< aggregated_table_t, std::vector<slot_connection> > >();
		auto &aggregate = std::get<0>(*composite);
		auto &connections = std::get<1>(*composite);
		const auto ukeyer = keyer_factory(underlying, underlying_key_tag());
		const auto &uindex = multi_index(underlying, ukeyer);
		auto &aindex = unique_index(aggregate, keyer_factory(aggregate, aggregated_key_tag()));
		const auto update_record = [aggregator, ukeyer, &uindex, &aindex] (typename U::const_iterator record) {
			const auto &key = ukeyer(*record);
			const auto uitems = uindex.equal_range(key);
			auto aggregated_record = aindex[key];

			aggregator(*aggregated_record, uitems.first, uitems.second);
			aggregated_record.commit();
		};

		connections.push_back(underlying.cleared += [&aggregate] {	aggregate.clear();	});
		connections.push_back(underlying.created += update_record);
		connections.push_back(underlying.modified += update_record);
		connections.push_back(underlying.removed += [aggregator, ukeyer, &uindex, &aindex] (typename U::const_iterator r) {
			const auto &key = ukeyer(*r);
			const auto uitems = uindex.equal_range(key);
			auto aggregated_record = aindex[key];

			if (uitems.first == uitems.second)
				aggregated_record.remove();
			else
				aggregator(*aggregated_record, uitems.first, uitems.second), aggregated_record.commit();
		});
		connections.push_back(underlying.invalidate += [&aggregate] {	aggregate.invalidate();	});
		for (auto i = underlying.begin(); i != underlying.end(); ++i)
			update_record(i);
		return std::shared_ptr<aggregated_table_t>(composite, &aggregate);
	}

	template <typename T, typename C, typename KeyerFactoryT, typename AggregatorT>
	inline std::shared_ptr< const table<T, C> > group_by(const table<T, C> &underlying,
		const KeyerFactoryT &keyer_factory, const AggregatorT &aggregator)
	{	return group_by<T, C>(underlying, keyer_factory, C(), aggregator);	}


	template <typename JoinedTableT, typename SideTableT, typename KeyerT, typename IndexT, typename ComposerT,
		typename JoinedHandleKeyerT, typename HandleKeyerT>
	inline std::function<void (typename SideTableT::const_iterator record)> maintain_joined(JoinedTableT &joined,
		std::vector<slot_connection> &connections, SideTableT &side, const KeyerT &keyer,
		const IndexT &opposite_side_index, const ComposerT &composer, const JoinedHandleKeyerT &jhkeyer,
		const HandleKeyerT &hkeyer)
	{
		auto &hindex = multi_index(joined, jhkeyer);
		auto on_create = [&joined, keyer, &opposite_side_index, composer] (typename SideTableT::const_iterator i) {
			for (auto j = opposite_side_index.equal_range(keyer(*i)); j.first != j.second; ++j.first)
			{
				auto r = joined.create();

				*r = composer(i, j.first.underlying()->second);
				r.commit();
			}
		};

		connections.push_back(side.created += on_create);
		connections.push_back(side.modified += [&joined, hkeyer, &hindex] (typename SideTableT::const_iterator i) {
			for (auto j = hindex.equal_range(hkeyer(i)); j.first != j.second; ++j.first)
				joined.modify(j.first.underlying()->second).commit();
		});
		connections.push_back(side.removed += [&joined, hkeyer, &hindex] (typename SideTableT::const_iterator i) {
			for (auto j = hindex.equal_range(hkeyer(i)); j.first != j.second; )
				joined.modify(j.first++.underlying()->second).remove();
		});
		connections.push_back(side.cleared += [&joined] {	joined.clear();	});
		connections.push_back(side.invalidate += [&joined] {	joined.invalidate();	});
		return on_create;
	}

	template <typename LeftKeyerT, typename RightKeyerT, typename LeftT, typename RightT>
	inline std::shared_ptr< const table< joined_record<LeftT, RightT> > > join(const LeftT &left, const RightT &right,
		const LeftKeyerT &left_by = LeftKeyerT(), const RightKeyerT &right_by = RightKeyerT())
	{
		typedef joined_record<LeftT, RightT> value_type;
		typedef table<value_type> joined_table_t;

		const auto composite = std::make_shared< std::tuple< joined_table_t, std::vector<slot_connection> > >();
		auto &joined = std::get<0>(*composite);
		auto &connections = std::get<1>(*composite);
		auto add_from_left = maintain_joined(joined, connections, left, left_by, multi_index(right, right_by),
			[] (typename LeftT::const_iterator i, typename RightT::const_iterator j) {
			return value_type(i, j);
		}, [] (const value_type &jrecord) {
			return &jrecord.left();
		}, [] (typename LeftT::const_iterator record) {
			return &*record;
		});

		maintain_joined(joined, connections, right, right_by, multi_index(left, left_by),
			[] (typename RightT::const_iterator i, typename LeftT::const_iterator j) {
			return value_type(j, i);
		}, [] (const value_type &jrecord) {
			return &jrecord.right();
		}, [] (typename RightT::const_iterator record) {
			return &*record;
		});

		for (auto i = left.begin(); i != left.end(); ++i)
			add_from_left(i);

		return std::shared_ptr<joined_table_t>(composite, &joined);
	}
}
