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

#include <frontend/frontend_manager.h>

#include <functional>
#include <string>
#include <wpl/base/concepts.h>
#include <wpl/base/signals.h>
#include <wpl/ui/view_host.h>
#include <wpl/ui/win32/window.h>

namespace micro_profiler
{
	class functions_list;
	struct hive;
	class tables_ui;

	class ProfilerMainDialog : public frontend_ui, wpl::noncopyable
	{
	public:
		ProfilerMainDialog(std::shared_ptr<functions_list> s, const std::string &executable);
		~ProfilerMainDialog();

	private:
		void OnCopyAll();
		void OnSupport();

	private:
		LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam, const wpl::ui::window::original_handler_t &handler);

		virtual void activate();

	private:
		HWND _hwnd;
		const std::shared_ptr<hive> _configuration;
		const std::shared_ptr<functions_list> _statistics;
		const std::string _executable;
		agge::rect_i _placement;
		std::shared_ptr<wpl::ui::view_host> _host;
		std::shared_ptr<tables_ui> _statistics_display;
		std::vector<wpl::slot_connection> _connections;
	};
}
