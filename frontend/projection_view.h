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

#include "trackables_provider.h"

#include <common/noncopyable.h>
#include <wpl/models.h>

namespace micro_profiler
{
	template <class UnderlyingT, typename T>
	class projection_view : public wpl::list_model<T>, noncopyable
	{
	public:
		typedef typename wpl::list_model<T>::index_type index_type;
		typedef typename UnderlyingT::value_type value_type;
		typedef std::function<T (const value_type &entry)> extractor_t;

	public:
		projection_view(const UnderlyingT &underlying);

		void fetch();
		void project(const extractor_t &extractor);
		void project();

		// wpl::series<...> methods
		virtual index_type get_count() const throw() override;
		virtual void get_value(index_type index, T &value) const throw() override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type index) const override;

	private:
		void operator =(const projection_view &rhs);

	private:
		const UnderlyingT &_underlying;
		const std::shared_ptr< trackables_provider<UnderlyingT> > _trackables;
		extractor_t _extractor;
	};



	template <class UnderlyingT, typename T>
	inline projection_view<UnderlyingT, T>::projection_view(const UnderlyingT &underlying)
		: _underlying(underlying), _trackables(std::make_shared< trackables_provider<UnderlyingT> >(underlying))
	{	}

	template <class UnderlyingT, typename T>
	inline void projection_view<UnderlyingT, T>::fetch()
	{
		_trackables->fetch();
		this->invalidate(this->npos());
	}

	template <class UnderlyingT, typename T>
	inline void projection_view<UnderlyingT, T>::project(const extractor_t &extractor)
	{
		_extractor = extractor;
		this->invalidate(this->npos());
	}

	template <class UnderlyingT, typename T>
	inline void projection_view<UnderlyingT, T>::project()
	{
		_extractor = extractor_t();
		this->invalidate(this->npos());
	}

	template <class UnderlyingT, typename T>
	inline typename projection_view<UnderlyingT, T>::index_type projection_view<UnderlyingT, T>::get_count() const throw()
	{	return _extractor ? static_cast<index_type>(_underlying.size()) : index_type();	}

	template <class UnderlyingT, typename T>
	inline void projection_view<UnderlyingT, T>::get_value(index_type index, T &value) const throw()
	{	value = _extractor ? _extractor(_underlying[index]) : T();	}

	template <class UnderlyingT, typename T>
	inline std::shared_ptr<const wpl::trackable> projection_view<UnderlyingT, T>::track(index_type index) const
	{	return _trackables->track(index);	}
}
