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

#include <list>
#include <wpl/models.h>

namespace micro_profiler
{
	template <typename UnderlyingT>
	class trackables_provider
	{
	public:
		trackables_provider(const UnderlyingT &underlying);
		~trackables_provider();

		void fetch();

		std::shared_ptr<wpl::trackable> track(size_t index) const;

	private:
		class trackable;

		typedef typename UnderlyingT::value_type value_type;
		typedef typename value_type::first_type key_type;
		typedef std::list<trackable> trackables_t;

	private:
		void operator =(const trackables_provider &rhs);

		void invalidate_trackables() throw();

	private:
		const UnderlyingT &_underlying;
		const std::shared_ptr<trackables_t> _trackables;
	};

	template <typename UnderlyingT>
	class trackables_provider<UnderlyingT>::trackable : public wpl::trackable
	{
	public:
		trackable(key_type key_, size_t current_index_) : key(key_), current_index(current_index_) {	}
		virtual size_t index() const override {	return current_index;	}

	public:
		key_type key;
		size_t current_index;

	private:
		const trackable &operator =(const trackable &rhs);
	};



	template <typename UnderlyingT>
	inline trackables_provider<UnderlyingT>::trackables_provider(const UnderlyingT &underlying)
		: _underlying(underlying), _trackables(std::make_shared<trackables_t>())
	{	}

	template <typename UnderlyingT>
	inline trackables_provider<UnderlyingT>::~trackables_provider()
	{	invalidate_trackables();	}

	template <typename UnderlyingT>
	inline void trackables_provider<UnderlyingT>::fetch()
	{
		const auto b = _trackables->begin();
		const auto e = _trackables->end();

		invalidate_trackables();
		for (size_t i = 0, count = _underlying.size(); i != count; ++i)
		{
			const auto &key = _underlying[i].first;

			for (auto j = b; j != e; ++j)
				if (j->key == key)
					j->current_index = i;
		}
	}

	template <typename UnderlyingT>
	inline std::shared_ptr<wpl::trackable> trackables_provider<UnderlyingT>::track(size_t index) const
	{
		auto trackables = _trackables;
		auto i = trackables->insert(trackables->end(), trackable(_underlying[index].first, index));

		return std::shared_ptr<wpl::trackable>(&*i, [trackables, i] (void *) {	trackables->erase(i);	});
	}

	template <typename UnderlyingT>
	inline void trackables_provider<UnderlyingT>::invalidate_trackables() throw()
	{
		for (auto i = _trackables->begin(); i != _trackables->end(); ++i)
			i->current_index = trackable::npos();
	}
}
