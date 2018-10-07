#include "tables_ui.h"

#include <olectl.h>
#include <windows.h>
#include <commctrl.h>
#include <wpl/ui/win32/controls.h>
#include <wpl/ui/win32/window.h>

using namespace std;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		class listview_complex
		{
		public:
			listview_complex()
			{
				enum {
					lvstyle = LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER
						| WS_TABSTOP | WS_POPUP | WS_VISIBLE,
					lvstyleex = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
				};

				_parent.reset(::CreateWindow(WC_LISTVIEW, NULL, (DWORD)lvstyle, 0, 0, 1, 1, NULL, NULL, NULL, NULL),
					&::DestroyWindow);
				_controller = window::attach(static_cast<HWND>(_parent.get()), &passthrough);

				HWND hlv = ::CreateWindow(WC_LISTVIEW, NULL, (DWORD)lvstyle, 0, 0, 1, 1, static_cast<HWND>(_parent.get()),
					NULL, NULL, NULL);

				ListView_SetExtendedListViewStyle(hlv, lvstyleex | ListView_GetExtendedListViewStyle(hlv));
				_listview = wrap_listview(hlv);
			}

			listview *get_listview()
			{	return _listview.get();	}

		private:
			static LRESULT passthrough(UINT message, WPARAM wparam, LPARAM lparam,
				const window::original_handler_t &handler)
			{
				switch (message)
				{
				case WM_NOTIFY:
					return SendMessage(reinterpret_cast<const NMHDR*>(lparam)->hwndFrom, OCM_NOTIFY, wparam, lparam);

				default:
					return handler(message, wparam, lparam);
				}
			}

		private:
			shared_ptr<void> _parent;
			shared_ptr<window> _controller;
			shared_ptr<listview> _listview;
		};
	}

	shared_ptr<listview> tables_ui::create_listview()
	{
		shared_ptr<listview_complex> complex(new listview_complex);

		return shared_ptr<listview>(complex, complex->get_listview());
	}
}
