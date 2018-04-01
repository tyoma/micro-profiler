//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/types.h>

#include <memory>
#include <string>
#include <wpl/base/signals.h>

namespace micro_profiler
{
	class functions_list;

	struct frontend_ui
	{
		typedef std::shared_ptr<frontend_ui> ptr;

		virtual ~frontend_ui() { }

		virtual void activate() = 0;
		wpl::signal<void()> closed;
	};

	struct frontend_manager
	{
	public:
		struct instance
		{
			std::wstring executable;
			std::shared_ptr<functions_list> model;
			std::shared_ptr<frontend_ui> ui;
		};

		typedef std::function<frontend_ui::ptr(const std::shared_ptr<functions_list> &model,
			const std::wstring &executable)> frontend_ui_factory;
		typedef std::shared_ptr<frontend_manager> ptr;

	public:
		static std::shared_ptr<frontend_manager> create(const guid_t &id, const frontend_ui_factory &ui_factory);

		virtual void close_all() throw() = 0;

		virtual size_t instances_count() const throw() = 0;
		virtual const instance *get_instance(unsigned index) const throw() = 0;
		virtual void load_instance(const instance &data) = 0;

	protected:
		frontend_manager() { }
		~frontend_manager() { }
	};

}
