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

#include "listview_impl.h"

#include <wpl/ui/win32/window.h>
#include <commctrl.h>
#include <atlstr.h>
#include <atlwin.h>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace wpl;
using namespace wpl::ui;

namespace
{
	class listview_impl : public listview
	{
		shared_ptr<window> _window;
		shared_ptr<destructible> _advisory, _invalidate_connection;
		shared_ptr<datasource> _datasource;
		vector<sort_direction> _default_sorts;
		int _sort_column;
		bool _sort_ascending;

		virtual void set_datasource(shared_ptr<datasource> ds)
		{
			_datasource = ds;
			_invalidate_connection = _datasource->invalidated += bind(&listview_impl::on_datasource_invalidated, this, _1);
			ListView_SetItemCountEx(_window->hwnd(), ds->get_count(), LVSICF_NOSCROLL);
		}

		virtual void add_column(const wstring &caption, sort_direction default_sort_direction)
		{
			unsigned index(Header_GetItemCount(ListView_GetHeader(_window->hwnd())));
			CString captionT(caption.c_str());
			LVCOLUMN column = {	LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER | LVCF_WIDTH, 0, -1, (LPTSTR)(LPCTSTR)captionT, 0, index, 0, index,	};

			ListView_InsertColumn(_window->hwnd(), index, &column);
			_default_sorts.push_back(default_sort_direction);
		}

		LRESULT window_proc(UINT message, WPARAM wparam, LPARAM lparam, const window::original_handler_t &original_handler)
		{
			if (OCM_NOTIFY == message)
			{
				UINT code = reinterpret_cast<const NMHDR *>(lparam)->code;
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW *>(lparam);

				if (LVN_ITEMACTIVATE == code)
				{
					const NMITEMACTIVATE *item = reinterpret_cast<const NMITEMACTIVATE *>(lparam);

					item_activate(item->iItem);
				}
				else if (LVN_ITEMCHANGED == code && (pnmlv->uOldState & LVIS_SELECTED) != (pnmlv->uNewState & LVIS_SELECTED))
					selection_changed(pnmlv->iItem, 0 != (pnmlv->uNewState & LVIS_SELECTED));
				else if (_datasource)
					if (LVN_GETDISPINFO == code)
					{
						const NMLVDISPINFO *pdi = reinterpret_cast<const NMLVDISPINFO *>(lparam);

						if (LVIF_TEXT & pdi->item.mask)
						{
							wstring text;

							_datasource->get_text(pdi->item.iItem, pdi->item.iSubItem, text);
							CString textT(text.c_str());
							_tcsncpy_s(pdi->item.pszText, pdi->item.cchTextMax, textT, _TRUNCATE);
						}
					}
					else if (LVN_COLUMNCLICK == code)
					{
						int sort_column = pnmlv->iSubItem;
						sort_direction default_sort = _default_sorts[sort_column];

						if (default_sort != dir_none)
						{
							HWND hheader = ListView_GetHeader(_window->hwnd());
							bool sort_ascending = sort_column == _sort_column ? !_sort_ascending : default_sort == dir_ascending;

							_datasource->set_order(sort_column, sort_ascending);
							set_column_direction(hheader, _sort_column, dir_none);
							set_column_direction(hheader, sort_column, sort_ascending ? dir_ascending : dir_descending);
							_sort_ascending = sort_ascending;
							_sort_column = sort_column;
						}
					}
					else if (LVN_ODCACHEHINT == code)
					{
						const NMLVCACHEHINT *lvch = reinterpret_cast<const NMLVCACHEHINT *>(lparam);

						_datasource->precache(lvch->iFrom, lvch->iTo - lvch->iFrom + 1);
					}
				return 0;
			}
			else
				return original_handler(message, wparam, lparam);
		}

		void on_datasource_invalidated(index_type new_count)
		{
			ListView_SetItemCountEx(_window->hwnd(), new_count, LVSICF_NOSCROLL);
		}

		static void set_column_direction(HWND hheader, index_type column, sort_direction direction) throw()
		{
			HDITEM item = { 0 };

			item.mask = HDI_FORMAT;
			item.fmt = HDF_STRING | (direction == dir_ascending ? HDF_SORTUP : direction == dir_descending ? HDF_SORTDOWN : 0);
			Header_SetItem(hheader, column, &item);
		}

	public:
		listview_impl(HWND hwnd)
			: _window(window::attach(hwnd)), _sort_column(-1)
		{
			_advisory = _window->advise(bind(&listview_impl::window_proc, this, _1, _2, _3, _4));
			ListView_SetExtendedListViewStyle(_window->hwnd(), LVS_EX_FULLROWSELECT | ListView_GetExtendedListViewStyle(_window->hwnd()));
		}

		~listview_impl() throw()
		{	_advisory.reset();	}
	};
}

shared_ptr<listview> wrap_listview(void *hwnd)
{	return shared_ptr<listview>(new listview_impl(reinterpret_cast<HWND>(hwnd)));	}
