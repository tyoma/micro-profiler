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
	void print_tree(HWND htree, HTREEITEM firstItem, int level = 0)
	{
		do
		{
			TCHAR buffer[1000] = { 0 };
			TVITEM ti = { 0 };

			ti.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
			ti.pszText = buffer;
			ti.cchTextMax = 999;
			ti.hItem = firstItem;
			TreeView_GetItem(htree, &ti);

			for (int i = 0; i < level; ++i)
				cout << "  ";
			wcout << ti.pszText << L" (" << ti.lParam << L")" << endl;

			if (HTREEITEM firstChild = TreeView_GetChild(htree, firstItem))
				print_tree(htree, firstChild, level + 1);
		}	while (firstItem = TreeView_GetNextSibling(htree, firstItem));
	}
}

application::application(_DTEPtr dte)
	: _dte(dte)
{
	_font = CreateFont(13, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, _T("Impact"));

	if (::AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
		::SetConsoleTitle(_T("Debug Console"));
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}

	//_openedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(_dispSolutionEvents), 1, &f);
	//_closedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(_dispSolutionEvents), 1, &f);

	_SolutionPtr s = _dte->Solution;

	HWND hwnd = ::FindWindow(NULL, _T("testmfc - Microsoft Visual Studio"));
	hwnd = ::FindWindowEx(hwnd, NULL, _T("GenericPane"), _T("Solution Explorer"));
	HWND parent = hwnd = ::FindWindowEx(hwnd, NULL, _T("VsUIHierarchyBaseWin"), NULL);
	hwnd = ::FindWindowEx(hwnd, NULL, _T("SysTreeView32"), NULL);

	_interception = window_wrapper::attach(parent)->advise(bind(&application::handle_solution_tree_event, this, hwnd, _1, _2, _3, _4));

	print_tree(hwnd, TreeView_GetRoot(hwnd));
}


application::~application()
{
	::FreeConsole();
	::DeleteObject(_font);
}

LRESULT application::handle_solution_tree_event(HWND htree, UINT message, WPARAM wparam, LPARAM lparam, const window_wrapper::previous_handler_t &previous)
{
	NMTVCUSTOMDRAW *n = message == WM_NOTIFY && ((NMHDR *)(lparam))->code == NM_CUSTOMDRAW ? ((NMTVCUSTOMDRAW *)(lparam)) : 0;

	LRESULT result = previous(message, wparam, lparam);

	if (n)
	{
		switch (n->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT:
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
				::SelectObject(n->nmcd.hdc, _font);
				n->clrText = 0x00123456;
				return CDRF_NEWFONT;
			default:
				break;
		}
	}
	return result;
}
