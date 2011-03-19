#include "application.h"

#include "repository.h"
#include "dispatch.h"

#include <tchar.h>
#include <commctrl.h>
#include <iostream>

using namespace std;
using namespace placeholders;
using namespace EnvDTE;

namespace
{
	LRESULT f(HWND htree, UINT message, WPARAM wparam, LPARAM lparam, const function<LRESULT(UINT, WPARAM, LPARAM)> &previous)
	{
		if (message == WM_NOTIFY && ((NMHDR *)lparam)->code == NM_CUSTOMDRAW)
		{
			NMTVCUSTOMDRAW *s = (NMTVCUSTOMDRAW *)lparam;
			int a = 0;
		}
		return previous(message, wparam, lparam);
	}

	void print_tree(HWND htree, HTREEITEM firstItem, int level = 0)
	{
		do
		{
			TCHAR buffer[1000] = { 0 };
			TVITEM ti = { 0 };

			ti.mask = TVIF_HANDLE | TVIF_TEXT;
			ti.pszText = buffer;
			ti.cchTextMax = 999;
			ti.hItem = firstItem;
			TreeView_GetItem(htree, &ti);

			for (int i = 0; i < level; ++i)
				cout << "  ";
			wcout << ti.pszText << endl;

			if (HTREEITEM firstChild = TreeView_GetChild(htree, firstItem))
				print_tree(htree, firstChild, level + 1);
		}	while (firstItem = TreeView_GetNextSibling(htree, firstItem));
	}
}

application::application(_DTEPtr dte)
	: _dte(dte)
{
	if (::AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
		::SetConsoleTitle(_T("Debug Console"));
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}

	//_openedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(_dispSolutionEvents), 1, &f);
	//_closedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(_dispSolutionEvents), 1, &f);


	HWND hwnd = ::FindWindow(NULL, _T("testmfc - Microsoft Visual Studio"));
	hwnd = ::FindWindowEx(hwnd, NULL, _T("GenericPane"), _T("Solution Explorer"));
	HWND parent = hwnd = ::FindWindowEx(hwnd, NULL, _T("VsUIHierarchyBaseWin"), NULL);
	hwnd = ::FindWindowEx(hwnd, NULL, _T("SysTreeView32"), NULL);

	_interception = window_wrapper::attach(parent)->advise(bind(&f, hwnd, _1, _2, _3, _4));

	print_tree(hwnd, TreeView_GetRoot(hwnd));
}


application::~application()
{
	::FreeConsole();
}
