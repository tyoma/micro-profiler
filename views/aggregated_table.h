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

namespace micro_profiler
{
	namespace views
	{
		template < typename U, typename ConstructorT = default_constructor<typename U::value_type> >
		class aggregated_table : public table<typename U::value_type, ConstructorT>
		{
		public:
			aggregated_table(const U &underlying);

			template <typename KeyerFactoryT, typename AggregatorT>
			void group_by(const KeyerFactoryT &keyer_factory, const AggregatorT &aggregator);

		private:
			const U &_underlying;
			wpl::slot_connection _on_underlying_cleared, _on_changed;
		};



		template <typename U, typename C>
		inline aggregated_table<U, C>::aggregated_table(const U &underlying)
			: _underlying(underlying)
		{
			_on_underlying_cleared = underlying.cleared += [this] {	this->clear();	};
		}

		template <typename U, typename C>
		template <typename KeyerFactoryT, typename AggregatorT>
		inline void aggregated_table<U, C>::group_by(const KeyerFactoryT &keyer_factory, const AggregatorT& aggregator)
		{
			const auto ukeyer = keyer_factory(_underlying);
			auto &uindex = multi_index(_underlying, ukeyer);
			auto &aindex = unique_index(*this, keyer_factory(*this));
			auto update_record = [aggregator, ukeyer, &uindex, &aindex] (typename U::const_reference value) {
				const auto key = ukeyer(value);
				auto uitems = uindex.equal_range(key);
				auto aggregated_record = aindex[key];

				aggregator(*aggregated_record, uitems.first, uitems.second);
				aggregated_record.commit();
			};

			_on_changed = _underlying.changed += [update_record] (typename U::const_iterator r, bool /*new_*/) {
				update_record(*r);
			};
			this->clear();
			for (auto i = _underlying.begin(); i != _underlying.end(); ++i)
				update_record(*i);
		}
	}
}
