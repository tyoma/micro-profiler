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

#include <type_traits>
#include <wpl/models.h>

namespace micro_profiler
{
	template <typename KeyT>
	struct selection
	{
		virtual void enumerate(const std::function<void (const KeyT &key)> &callback) = 0;
		wpl::signal<void ()> changed;
	};

	template <typename T>
	struct selection_key;

	template <typename T1, typename T2>
	struct selection_key< std::pair<T1, T2> >
	{
		typedef typename std::remove_const<T1>::type key_type;
	};

	template <typename UnderlyingT>
	class selection_model : public selection<typename selection_key<typename UnderlyingT::value_type>::key_type>
	{
	public:
		selection_model(const UnderlyingT &underlying);

		virtual void enumerate(const std::function<void (const KeyT &key)> &callback) override;

		virtual void clear() throw() /*override*/;
		virtual void add(wpl::index_traits::index_type item) /*override*/;
		virtual void remove(wpl::index_traits::index_type item) /*override*/;
		virtual bool contains(wpl::index_traits::index_type item) const throw() /*override*/;

		wpl::signal<void (index_type item)> invalidate;
	};
}
