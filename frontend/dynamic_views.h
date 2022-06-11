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

#include <common/noncopyable.h>
#include <memory>
#include <views/filter.h>
#include <wpl/signal.h>

namespace micro_profiler
{
	template <typename U>
	class filter_view : private views::filter<U>, noncopyable
	{
	public:
		using typename views::filter<U>::const_iterator;
		using typename views::filter<U>::value_type;
		using typename views::filter<U>::const_reference;
		using typename views::filter<U>::reference;

	public:
		filter_view(std::shared_ptr<U> underlying)
			: views::filter<U>(*underlying), _underlying(underlying)
		{	_connection = underlying->invalidate += [this] (...) {	invalidate();	};	}

		template <typename PredicateT>
		void set_filter(const PredicateT &predicate)
		{	views::filter<U>::set_filter(predicate),	invalidate();	}

		void set_filter()
		{	views::filter<U>::set_filter(),	invalidate();	}

		using views::filter<U>::begin;
		using views::filter<U>::end;

	public:
		wpl::signal<void ()> invalidate;

	private:
		const std::shared_ptr<U> _underlying;
		wpl::slot_connection _connection;
	};



	template <typename U>
	inline std::shared_ptr< filter_view<U> > make_filter_view(std::shared_ptr<U> underlying)
	{	return std::make_shared< filter_view<U> >(underlying);	}
}
