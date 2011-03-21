#include "treeview_handler.h"

#include "windowing.h"
#include <commctrl.h>

using namespace std;

namespace
{
	LRESULT handle_tv_parent_messages(UINT message, WPARAM wparam, LPARAM lparam, const window_wrapper::previous_handler_t &previous)
	{
		if (NMTVCUSTOMDRAW *n = message == WM_NOTIFY && reinterpret_cast<NMHDR *>(lparam)->code == NM_CUSTOMDRAW
			? reinterpret_cast<NMTVCUSTOMDRAW *>(lparam) : 0)
		{
			return CDRF_NOTIFYITEMDRAW;
		}
		return previous(message, wparam, lparam);
	}
}

treeview_handler::treeview_handler(void *htree)
{
	shared_ptr<window_wrapper> w(window_wrapper::attach(::GetParent(reinterpret_cast<HWND>(htree))));

	_interception = w->advise(&handle_tv_parent_messages);
}
