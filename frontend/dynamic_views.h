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
#include <views/flatten.h>
#include <wpl/signal.h>

namespace micro_profiler
{
	template <typename U>
	class filter_view : private views::filter<U>, noncopyable
	{
	public:
		using views::filter<U>::const_iterator;
		using views::filter<U>::value_type;
		using views::filter<U>::const_reference;
		using views::filter<U>::reference;

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


	template <typename U, typename ScopeT, typename X>
	class scoped_view : public views::flatten<ScopeT, X>, noncopyable
	{
	public:
		scoped_view(std::shared_ptr<U> underlying, std::shared_ptr<ScopeT> scope)
			: views::flatten<ScopeT, X>(*scope, X(*underlying)), _underlying(underlying), _scope(scope)
		{
			_connections[0] = underlying->invalidate += [this] (...) {	invalidate();	};
			_connections[1] = scope->invalidate += [this] (...) {	invalidate();	};
		}

	public:
		mutable wpl::signal<void ()> invalidate;

	private:
		const std::shared_ptr<U> _underlying;
		const std::shared_ptr<ScopeT> _scope;
		wpl::slot_connection _connections[2];
	};



	template <typename U>
	inline std::shared_ptr< filter_view<U> > make_filter_view(std::shared_ptr<U> underlying)
	{	return std::make_shared< filter_view<U> >(underlying);	}


	template <typename X, typename U, typename ScopeT>
	inline std::shared_ptr< scoped_view<U, ScopeT, X> > make_scoped_view(std::shared_ptr<U> underlying,
		std::shared_ptr<ScopeT> scope)
	{	return std::make_shared< scoped_view<U, ScopeT, X> >(underlying, scope);	}
}
