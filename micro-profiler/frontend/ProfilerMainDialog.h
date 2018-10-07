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

#include <resources/resource.h>

#include <atlbase.h>
#include <atltypes.h>
#include <atlwin.h>
#include <functional>
#include <string>
#include <wpl/ui/view_host.h>

namespace micro_profiler
{
	class functions_list;
	class tables_ui;

	class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>, public frontend_ui
	{
	public:
		enum {	IDD = IDD_PROFILER_MAIN	};

	public:
		ProfilerMainDialog(std::shared_ptr<functions_list> s, const std::wstring &executable, HWND parent);
		~ProfilerMainDialog();

	private:
		BEGIN_MSG_MAP(ProfilerMainDialog)
			MESSAGE_HANDLER(WM_ACTIVATE, OnActivated)
			MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
			MESSAGE_HANDLER(WM_CLOSE, OnClose)
			REFLECT_NOTIFICATIONS()
		END_MSG_MAP()

		LRESULT OnActivated(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnWindowPosChanged(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnClose(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);

		void OnCopyAll();
		void OnSupport();

	private:
		virtual void OnFinalMessage(HWND hwnd);

		virtual void activate();

	private:
		const std::shared_ptr<functions_list> _statistics;
		const std::wstring _executable;
		CRect _placement;
		std::shared_ptr<wpl::ui::view_host> _host;
		std::shared_ptr<tables_ui> _statistics_display;
	};
}
