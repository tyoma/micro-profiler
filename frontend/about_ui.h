//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <atlwin.h>

namespace micro_profiler
{
	class SupportDevDialog : public ATL::CDialogImpl<SupportDevDialog>
	{
	public:
		enum {	IDD = IDD_SUPPORT_DEV	};

	private:
		BEGIN_MSG_MAP(SupportDevDialog)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
			COMMAND_RANGE_HANDLER(IDOK, IDNO, OnCloseCmd)
			NOTIFY_HANDLER(IDC_WAYS_TO_SUPPORT, NM_CLICK, OnLinkClicked)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT message, WPARAM wparam, LPARAM lparam, BOOL &handled);
		LRESULT OnLinkClicked(int id, NMHDR *nmhdr, BOOL &handled);
		LRESULT OnCloseCmd(WORD, WORD id, HWND, BOOL &handled);
	};
}
