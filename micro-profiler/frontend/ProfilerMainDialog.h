//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "resource.h"

#include <atlbase.h>
#include <atltypes.h>
#include <atlwin.h>
#include <string>
#include <functional>

namespace std
{
	using tr1::function;
}

namespace micro_profiler
{
	struct function_statistics;
	class statistics;
	class symbol_resolver;
	typedef std::basic_string<TCHAR> tstring;

	class ProfilerMainDialog : public ATL::CDialogImpl<ProfilerMainDialog>
	{
		typedef std::function<tstring(const void *, const function_statistics &)> print_func_t;
		typedef std::function<tstring(const void *, unsigned __int64)> print_parent_func_t;
		typedef std::function<bool (const void *lhs_addr, const function_statistics &lhs, const void *rhs_addr, const function_statistics &rhs)> predicate_func_t;

		print_func_t _printers[7];
		print_parent_func_t _parent_printers[2];
		std::pair<predicate_func_t, bool /*default_ascending*/> _sorters[7];
		statistics &_statistics;
		const symbol_resolver &_resolver;
		CWindow _statistics_view, _children_statistics_view, _parents_statistics_view, _clear_button, _copy_all_button;
		int _last_sort_column, _last_children_sort_column;
		bool _sort_ascending, _sort_children_ascending;
		const void *_last_selected;

		void SelectByAddress(const void *address);
		void RelocateControls(const CSize &size);

	public:
		ProfilerMainDialog(statistics &s, const symbol_resolver &resolver, __int64 ticks_resolution);
		~ProfilerMainDialog();

		void RefreshList(unsigned int new_count);

		enum {	IDD = IDD_PROFILER_MAIN	};

		BEGIN_MSG_MAP(ProfilerMainDialog)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
			MESSAGE_HANDLER(WM_SIZE, OnSize);
			NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_PARENTS_STATISTICS, LVN_GETDISPINFO, OnGetDispInfo);
			NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_CHILDREN_STATISTICS, LVN_COLUMNCLICK, OnColumnSort);
			NOTIFY_RANGE_CODE_HANDLER(IDC_FUNCTIONS_STATISTICS, IDC_FUNCTIONS_STATISTICS, LVN_ITEMCHANGED, OnFocusedFunctionChange);
			NOTIFY_RANGE_CODE_HANDLER(IDC_CHILDREN_STATISTICS, IDC_PARENTS_STATISTICS, LVN_ITEMACTIVATE, OnDrillDown);
			COMMAND_HANDLER(IDC_BTN_CLEAR, BN_CLICKED, OnClearStatistics);
			COMMAND_HANDLER(IDC_BTN_COPY_ALL, BN_CLICKED, OnCopyAll);
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnSize(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnGetDispInfo(int control_id, LPNMHDR pnmh, BOOL &handled);
		LRESULT OnColumnSort(int control_id, LPNMHDR pnmh, BOOL &handled);
		LRESULT OnFocusedFunctionChange(int control_id, LPNMHDR pnmh, BOOL &handled);
		LRESULT OnDrillDown(int control_id, LPNMHDR pnmh, BOOL &handled);
		LRESULT OnClearStatistics(WORD code, WORD control_id, HWND control, BOOL &handled);
		LRESULT OnCopyAll(WORD code, WORD control_id, HWND control, BOOL &handled);
	};
}
