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

#include <frontend/frontend_ui.h>

#include <agge/types.h>
#include <functional>
#include <string>
#include <vector>
#include <wpl/concepts.h>

namespace wpl
{
	class factory;
	struct form;
}

namespace micro_profiler
{
	struct hive;
	class tables_ui;

	class standalone_ui : public frontend_ui, wpl::noncopyable
	{
	public:
		standalone_ui(std::shared_ptr<hive> configuration, const wpl::factory &factory, const frontend_ui_context &ctx);

		wpl::signal<void (const std::string &text)> copy_to_buffer;
		wpl::signal<void (agge::point<int> center, const std::shared_ptr<wpl::form> &new_form)> show_about;
		wpl::signal<void (agge::point<int> center, const std::shared_ptr<wpl::form> &new_form)> show_patcher;

	private:
		virtual void activate();

	private:
		const std::shared_ptr<hive> _configuration;
		std::shared_ptr<tables_ui> _statistics_display;
		std::vector<wpl::slot_connection> _connections;
		std::shared_ptr<wpl::form> _form;
	};
}
