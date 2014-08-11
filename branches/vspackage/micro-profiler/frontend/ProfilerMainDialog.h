//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "../resources/resource.h"
#include "object_lock.h"

#include <atlbase.h>
#include <atltypes.h>
#include <atlwin.h>
#include <string>
#include <functional>
#include <wpl/ui/listview.h>

namespace std
{
	using tr1::function;
}

namespace micro_profiler
{
	class columns_model;
	struct functions_list;
	struct linked_statistics;

	class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>, public self_unlockable
	{
		const std::shared_ptr<functions_list> _statistics;
		const std::wstring _executable;
		std::shared_ptr<linked_statistics> _parents_statistics, _children_statistics;
		const std::shared_ptr<columns_model> _columns_parents, _columns_main, _columns_children;
		CWindow _statistics_view, _children_statistics_view, _parents_statistics_view, _clear_button, _copy_all_button;
		std::shared_ptr<wpl::ui::listview> _statistics_lv, _parents_statistics_lv, _children_statistics_lv;
		std::vector<wpl::slot_connection> _connections;
		CRect _placement;

		void RelocateControls(const CSize &size);

		void OnFocusChange(wpl::ui::listview::index_type index, bool selected);
		void OnDrilldown(std::shared_ptr<linked_statistics> view, wpl::ui::listview::index_type index);

		virtual void OnFinalMessage(HWND hwnd);

	public:
		ProfilerMainDialog(std::shared_ptr<functions_list> s, const std::wstring &executable);
		~ProfilerMainDialog();

		enum {	IDD = IDD_PROFILER_MAIN	};

		BEGIN_MSG_MAP(ProfilerMainDialog)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
			MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged);
			COMMAND_HANDLER(IDC_BTN_CLEAR, BN_CLICKED, OnClearStatistics);
			COMMAND_HANDLER(IDC_BTN_COPY_ALL, BN_CLICKED, OnCopyAll);
			MESSAGE_HANDLER(WM_CLOSE, OnClose);
			REFLECT_NOTIFICATIONS();
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnWindowPosChanged(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnClearStatistics(WORD code, WORD control_id, HWND control, BOOL &handled);
		LRESULT OnCopyAll(WORD code, WORD control_id, HWND control, BOOL &handled);
		LRESULT OnClose(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
	};
}
