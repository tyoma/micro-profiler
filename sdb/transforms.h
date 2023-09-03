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

#include <common/nullable.h>
#include <tuple>

namespace sdb
{
	using micro_profiler::nullable;

	struct agnostic_key_tag {};
	struct underlying_key_tag : agnostic_key_tag {};
	struct aggregated_key_tag : agnostic_key_tag {};

	template <typename Table1T, typename Table2T>
	class joined_record;

	template <typename I>
	struct joined_prime;

	template <typename T>
	struct joined_leftmost {	typedef T type;	};

	template <typename I1, typename I2>
	struct joined_leftmost< joined_record<I1, I2> > {	typedef typename joined_prime<I1>::type type;	};

	template <typename I>
	struct joined_prime {	typedef typename joined_leftmost<typename std::iterator_traits<I>::value_type>::type type;	};

	template <typename I>
	struct iterator_reference {	typedef typename std::iterator_traits<I>::reference type;	};

	template <typename I>
	struct iterator_reference< nullable<I> > {	typedef nullable<typename iterator_reference<I>::type> type;	};

	template <typename I>
	inline typename iterator_reference<I>::type dereference(const I &iterator)
	{	return *iterator;	}

	template <typename I>
	inline nullable<typename iterator_reference<I>::type> dereference(const nullable<I> &iterator)
	{	return iterator.and_then([] (const I &i) -> typename iterator_reference<I>::type {	return *i;	});	}

	template <typename I1, typename I2>
	class joined_record
	{
	public:
		typedef typename iterator_reference<I1>::type left_reference;
		typedef typename std::remove_all_extents<left_reference>::type left_type;
		typedef typename iterator_reference<I2>::type right_reference;

	public:
		joined_record() {	}
		joined_record(I1 left_, I2 right_) : _left(left_), _right(right_) {	}

		left_reference left() const {	return dereference(_left);	}
		right_reference right() const {	return dereference(_right);	}

		operator const typename joined_prime<I1>::type &() const {	return *_left;	}

	private:
		I1 _left;
		I2 _right;
	};

	template <typename LeftTableT, typename RightTableT>
	struct joined
	{
		typedef joined_record<typename LeftTableT::const_iterator, typename RightTableT::const_iterator> value_type;
		typedef table<value_type> table_type;
		typedef std::shared_ptr<const table_type> table_ptr;
	};

	template <typename LeftTableT, typename RightTableT>
	struct left_joined
	{
		typedef joined_record< typename LeftTableT::const_iterator, nullable<typename RightTableT::const_iterator> > value_type;
		typedef table<value_type> table_type;
		typedef std::shared_ptr<const table_type> table_ptr;
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


	template <typename JoinedTableT, typename SideTableT, typename KeyerT, typename IndexT, typename MatchedF,
		typename UnmatchedF, typename JoinedHandleKeyerT, typename HandleKeyerT>
	inline std::function<void (typename SideTableT::const_iterator record)> maintain_joined(JoinedTableT &joined,
		std::vector<slot_connection> &connections, SideTableT &side, KeyerT keyer, const IndexT &opposite_side_index,
		MatchedF matched, UnmatchedF unmatched, JoinedHandleKeyerT jhkeyer, const HandleKeyerT &hkeyer)
	{
		auto &hindex = multi_index(joined, jhkeyer);
		auto create = [&joined, keyer, &opposite_side_index, matched, unmatched] (typename SideTableT::const_iterator i) {
			auto j = opposite_side_index.equal_range(keyer(*i));

			if (j.first == j.second)
				unmatched(i);
			else for (; j.first != j.second; ++j.first)
			{
				auto r = joined.create();

				*r = matched(i, j.first.underlying()->second);
				r.commit();
			}
		};

		connections.push_back(side.created += create);
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
		return create;
	}

	template <typename LeftKeyerT, typename RightKeyerT, typename LeftT, typename RightT>
	inline typename joined<LeftT, RightT>::table_ptr join(const LeftT &left, const RightT &right,
		LeftKeyerT left_by = LeftKeyerT(), RightKeyerT right_by = RightKeyerT())
	{
		typedef typename joined<LeftT, RightT>::value_type value_type;
		typedef typename joined<LeftT, RightT>::table_type joined_table_t;
		typedef typename joined<LeftT, RightT>::table_ptr joined_table_ptr_t;

		const auto composite = std::make_shared< std::tuple< joined_table_t, std::vector<slot_connection> > >();
		auto &joined = std::get<0>(*composite);
		auto &connections = std::get<1>(*composite);
		auto add_from_left = maintain_joined(joined, connections, left, left_by, multi_index(right, right_by),
			[] (typename LeftT::const_iterator i, typename RightT::const_iterator j) {
			return value_type(i, j);
		}, [] (typename LeftT::const_iterator /*i*/) {
		}, [] (const value_type &jrecord) {
			return &jrecord.left();
		}, [] (typename LeftT::const_iterator record) {
			return &*record;
		});

		maintain_joined(joined, connections, right, right_by, multi_index(left, left_by),
			[] (typename RightT::const_iterator i, typename LeftT::const_iterator j) {
			return value_type(j, i);
		}, [] (typename RightT::const_iterator /*i*/) {
		}, [] (const value_type &jrecord) {
			return &jrecord.right();
		}, [] (typename RightT::const_iterator record) {
			return &*record;
		});

		for (auto i = left.begin(); i != left.end(); ++i)
			add_from_left(i);

		return joined_table_ptr_t(composite, &joined);
	}

	template <typename LeftKeyerT, typename RightKeyerT, typename LeftT, typename RightT>
	inline typename left_joined<LeftT, RightT>::table_ptr left_join(const LeftT &left, const RightT &right,
		LeftKeyerT left_by = LeftKeyerT(), RightKeyerT right_by = RightKeyerT())
	{
		typedef typename left_joined<LeftT, RightT>::value_type value_type;
		typedef nullable<typename RightT::const_iterator> nullable_type;
		typedef typename left_joined<LeftT, RightT>::table_type joined_table_t;
		typedef typename left_joined<LeftT, RightT>::table_ptr joined_table_ptr_t;

		const auto composite = std::make_shared< std::tuple< joined_table_t, std::vector<slot_connection> > >();
		auto &joined = std::get<0>(*composite);
		auto &connections = std::get<1>(*composite);
		auto &left_index = multi_index(left, left_by);
		auto &joined_left_index = multi_index(joined, [left_by] (const value_type &j) {	return std::make_tuple(left_by(j.left()), j.right().has_value());	});
		auto add_stub = [&joined] (typename LeftT::const_iterator i) {
			auto r = joined.create();

			*r = value_type(i, nullable_type());
			r.commit();
		};
		auto add_from_left = maintain_joined(joined, connections, left, left_by, multi_index(right, right_by),
			[] (typename LeftT::const_iterator i, typename RightT::const_iterator j) {
			return value_type(i, nullable_type(j));
		}, add_stub, [] (const value_type &jrecord) {
			return &jrecord.left();
		}, [] (typename LeftT::const_iterator record) {
			return &*record;
		});

		connections.push_back(right.created += [&joined, &joined_left_index, right_by] (typename RightT::const_iterator i) {
			for (auto j = joined_left_index.equal_range(std::make_tuple(right_by(*i), false)); j.first != j.second; )
				joined.modify(j.first++.underlying()->second).remove();
		});

		maintain_joined(joined, connections, right, right_by, left_index,
			[] (typename RightT::const_iterator i, typename LeftT::const_iterator j) {
			return value_type(j, nullable_type(i));
		}, [] (typename RightT::const_iterator /*i*/) {
		}, [] (const value_type &jrecord) {
			return jrecord.right().has_value() ? &*jrecord.right() : nullptr;
		}, [] (typename RightT::const_iterator record) {
			return &*record;
		});

		connections.push_back(right.removed += [right_by, &joined, &left_index, &joined_left_index, add_stub] (typename RightT::const_iterator i) {
			auto k = right_by(*i);
			auto j = joined_left_index.equal_range(std::make_tuple(k, true));

			if (j.first == j.second)
				for (auto l = left_index.equal_range(k); l.first != l.second; ++l.first)
					add_stub(l.first.underlying()->second);
		});

		for (auto i = left.begin(); i != left.end(); ++i)
			add_from_left(i);

		return joined_table_ptr_t(composite, &joined);
	}
}
