//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "function_list.h"

#include <views/flatten.h>

namespace micro_profiler
{
	template <typename U, typename X>
	struct nested_statistics_model_complex
	{
		typedef typename key_traits<typename U::value_type>::key_type key_type;
		typedef std::vector<key_type> scope_type;
		typedef views::flatten<scope_type, X> view_type;

		nested_statistics_model_complex(std::shared_ptr<U> underlying_, std::shared_ptr<scope_type> scope_);

		const std::shared_ptr<U> underlying;
		const std::shared_ptr<scope_type> scope;
		X transform;
		view_type flattened;
		wpl::slot_connection fetch_connection;
	};



	template <typename U, typename X>
	inline nested_statistics_model_complex<U, X>::nested_statistics_model_complex(std::shared_ptr<U> underlying_,
			std::shared_ptr<scope_type> scope_)
		: underlying(underlying_), scope(scope_), transform(*underlying), flattened(*scope, transform)
	{	}


	template <typename X, typename U>
	inline std::shared_ptr<linked_statistics> construct_nested(std::shared_ptr<U> underlying, double tick_interval,
		std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<threads_model> threads,
		std::shared_ptr< std::vector<typename key_traits<typename U::value_type>::key_type> > scope)
	{
		typedef nested_statistics_model_complex<U, X> complex_type;
		typedef typename complex_type::view_type view_type;

		const auto complex = std::make_shared<complex_type>(underlying, scope);
		const std::shared_ptr<view_type> v(complex, &complex->flattened);
		const auto nested = std::make_shared< statistics_model_impl<linked_statistics, view_type> >(v, tick_interval, resolver,
			threads);
		const auto pnested = nested.get();

		complex->fetch_connection = underlying->invalidate += [pnested] {
			pnested->fetch();
		};
		return nested;
	}
}
