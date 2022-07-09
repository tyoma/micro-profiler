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

#include "tables.h"

#include <common/noncopyable.h>
#include <wpl/models.h>

namespace micro_profiler
{
	template <typename UnderlyingT>
	class trackables_provider;

	namespace views
	{
		template <typename U>
		class ordered;
	}

	class threads_model : public wpl::list_model<std::string>, noncopyable
	{
	public:
		enum {	all = static_cast<id_t>(-2), cumulative = static_cast<id_t>(-1),	};

	public:
		threads_model(std::shared_ptr<const tables::threads> threads);

		bool get_key(unsigned int &thread_id, index_type index) const throw();

		virtual index_type get_count() const throw() override;
		virtual void get_value(index_type index, std::string &text) const override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type index) const override;

	private:
		typedef views::ordered<tables::threads> view_type;
		typedef trackables_provider<view_type> trackables_type;

	private:
		const std::shared_ptr<const tables::threads> _underlying;
		const std::shared_ptr<view_type> _view;
		const std::shared_ptr<trackables_type> _trackables;
		wpl::slot_connection _invalidation;
	};
}
