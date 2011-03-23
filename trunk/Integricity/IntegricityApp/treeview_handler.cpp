#include "treeview_handler.h"

#include "windowing.h"
#include <commctrl.h>

using namespace std;
using namespace placeholders;

namespace
{
	class treeview_handler : noncopyable, public destructible
	{
		HWND _htree;
		prepaint_handler_t _prepaint_handler;
		shared_ptr<window_wrapper> _parent_wrapper;
		std::shared_ptr<destructible> _interception;
		vector<wstring> _path_buffer;

		LRESULT handle_parent_messages(UINT message, WPARAM wparam, LPARAM lparam, const window_wrapper::previous_handler_t &previous);

		void extract_path(HTREEITEM hitem, vector<wstring> &path);

	public:
		treeview_handler(void *htree, const prepaint_handler_t &prepaint_handler);
		~treeview_handler();
	};

	treeview_handler::treeview_handler(void *htree, const prepaint_handler_t &prepaint_handler)
		: _htree(reinterpret_cast<HWND>(htree)), _prepaint_handler(prepaint_handler), _parent_wrapper(window_wrapper::attach(::GetParent(_htree)))
	{	_interception = _parent_wrapper->advise(bind(&treeview_handler::handle_parent_messages, this, _1, _2, _3, _4));	}

	treeview_handler::~treeview_handler()
	{	_parent_wrapper->detach();	}

	LRESULT treeview_handler::handle_parent_messages(UINT message, WPARAM wparam, LPARAM lparam, const window_wrapper::previous_handler_t &previous)
	{
		if (NMTVCUSTOMDRAW *n = message == WM_NOTIFY && reinterpret_cast<NMHDR *>(lparam)->code == NM_CUSTOMDRAW ? reinterpret_cast<NMTVCUSTOMDRAW *>(lparam) : 0)
			switch (n->nmcd.dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT:
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
				bool c;
				_path_buffer.clear();
				extract_path(reinterpret_cast<HTREEITEM>(n->nmcd.dwItemSpec), _path_buffer);
				_prepaint_handler(_path_buffer, reinterpret_cast<unsigned int &>(n->clrText), reinterpret_cast<unsigned int &>(n->clrTextBk), c);
				break;				
			}
		return previous(message, wparam, lparam);
	}

	void treeview_handler::extract_path(HTREEITEM hitem, vector<wstring> &path)
	{
		int level = 0;
		wchar_t buffer[512] = { 0 };
		TVITEMW ti = { 0 };

		ti.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
		ti.pszText = buffer;
		ti.cchTextMax = 999;
		ti.hItem = hitem;
		TreeView_GetItem(_htree, &ti);

		if (HTREEITEM parent = TreeView_GetParent(_htree, hitem))
			extract_path(parent, path);
		path.push_back(ti.pszText);
	}
}

shared_ptr<destructible> handle_tv_notifications(void *htree, const prepaint_handler_t &prepaint_handler)
{	return shared_ptr<destructible>(new treeview_handler(htree, prepaint_handler));	}
