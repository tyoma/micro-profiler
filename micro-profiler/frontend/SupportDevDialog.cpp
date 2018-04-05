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

#include "SupportDevDialog.h"

namespace micro_profiler
{
	extern HINSTANCE g_instance;

	LRESULT SupportDevDialog::OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL& handled)
	{
		HICON hicon = ::LoadIcon(g_instance, MAKEINTRESOURCE(IDI_APPMAIN));
		
		SetIcon(hicon, TRUE);
		CenterWindow();
		return handled = TRUE, 1;
	}

	LRESULT SupportDevDialog::OnLinkClicked(int /*id*/, NMHDR *nmhdr, BOOL &handled)
	{
		const NMLINK *n = static_cast<const NMLINK *>(static_cast<const void *>(nmhdr));

		::ShellExecute(NULL, _T("open"), n->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
		return handled = TRUE, 1;
	}

	LRESULT SupportDevDialog::OnCloseCmd(WORD, WORD /*id*/, HWND, BOOL &handled)
	{
		EndDialog(0);
		return handled = TRUE, 0;
	}
}
