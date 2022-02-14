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

#include "index.h"
#include "table.h"

namespace micro_profiler
{
	namespace views
	{
		template < typename U, typename ConstructorT = default_constructor<typename U::value_type> >
		class aggregated_table
		{
		private:
			typedef table<typename U::value_type, ConstructorT> aggregated_items_type;

		public:
			typedef ConstructorT constructor_type;
			typedef typename aggregated_items_type::const_iterator const_iterator;
			typedef const_iterator iterator;
			typedef typename U::value_type value_type;
			typedef const value_type &const_reference;
			typedef const_reference reference;
			class transacted_record;

		public:
			aggregated_table(const U &underlying);

			const_iterator begin() const;
			const_iterator end() const;

			template <typename UnderlyingKeyerT, typename AggregatedKeyerT, typename AggregatorT>
			void group_by(const UnderlyingKeyerT &ukeyer, const AggregatedKeyerT &akeyer, const AggregatorT &aggregator);

		public:
			mutable wpl::signal<void (iterator irecord, bool new_)> changed;
			wpl::signal<void ()> &cleared;

		private:
			const U &_underlying;
			table<value_type, ConstructorT> _aggregated;
			wpl::slot_connection _on_clear, _on_changed, _on_changed2;
		};



		template <typename U, typename C>
		inline aggregated_table<U, C>::aggregated_table(const U &underlying)
			: cleared(_aggregated.cleared), _underlying(underlying)
		{
			_on_clear = _underlying.cleared += [this] {	_aggregated.clear();	};
			_on_changed2 = _aggregated.changed += [this] (const_iterator i, bool new_) {	changed(i, new_);	};
		}

		template <typename U, typename C>
		typename aggregated_table<U, C>::const_iterator aggregated_table<U, C>::begin() const
		{	return _aggregated.begin();	}

		template <typename U, typename C>
		typename aggregated_table<U, C>::const_iterator aggregated_table<U, C>::end() const
		{	return _aggregated.end();	}

		template <typename U, typename C>
		template <typename UnderlyingKeyerT, typename AggregatedKeyerT, typename AggregatorT>
		inline void aggregated_table<U, C>::group_by(const UnderlyingKeyerT &ukeyer, const AggregatedKeyerT &akeyer,
			const AggregatorT& aggregator)
		{
			auto uindex = std::make_shared< immutable_index<U, UnderlyingKeyerT> >(_underlying, ukeyer);
			auto aindex = std::make_shared< immutable_unique_index<aggregated_items_type, AggregatedKeyerT> >(_aggregated,
				akeyer);
			auto update_record = [aggregator, uindex, aindex] (const typename UnderlyingKeyerT::key_type &key) {
				auto uitems = uindex->equal_range(key);
				auto aggregated_record = (*aindex)[key];

				aggregator(*aggregated_record, uitems.first, uitems.second);
				aggregated_record.commit();
			};

			_on_changed = _underlying.changed += [ukeyer, update_record] (typename U::const_iterator r, bool /*new_*/) {
				update_record(ukeyer(*r));
			};
			_aggregated.clear();
			for (auto i = _underlying.begin(); i != _underlying.end(); ++i)
				update_record(ukeyer(*i));
		}
	}
}
