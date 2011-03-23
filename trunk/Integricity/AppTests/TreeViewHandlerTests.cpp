#include <treeview_handler.h>

#include "TestHelpers.h"

#include <windowing.h>
#include <windows.h>
#include <commctrl.h>

using namespace std;
using namespace placeholders;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	namespace
	{
		void dummy_handler(const vector<wstring> &path, unsigned int &foreground_color, unsigned int &background_color, bool &emphasized)
		{	}

		void copy_arguments(vector< vector<wstring> > &path_dest, const vector<wstring> &path, unsigned int &foreground_color, unsigned int &background_color, bool &emphasized)
		{
			path_dest.push_back(path);
		}

		void *add_item(void *htree, void *parent_item, const wstring &text)
		{
			TVITEMW ti = { TVIF_TEXT, 0, 0, 0, const_cast<wchar_t *>(text.c_str()), };
			TVINSERTSTRUCTW insertion = { (HTREEITEM)parent_item, TVI_LAST, };

			insertion.item = ti;
			return TreeView_InsertItem((HWND)htree, &insertion);
		}
	}

	[TestClass]
	public ref class TreeViewHandlerTests : ut::WindowTestsBase
	{
	public:
		[TestMethod]
		void TreeWrapperRequestsPrepaint()
		{
			// INIT
			void *htree = create_tree();
			NMTVCUSTOMDRAW n = { 0 };

			n.nmcd.hdr.code = NM_CUSTOMDRAW;
			n.nmcd.dwDrawStage = CDDS_PREPAINT;

			// ACT
			shared_ptr<destructible> tvh(handle_tv_notifications(htree, &dummy_handler));
			LRESULT result = ::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);

			// ASSERT
			Assert::IsTrue(CDRF_NOTIFYITEMDRAW == result);
		}


		[TestMethod]
		void PassthroughToPrevious()
		{
			// INIT
			void *htree = create_tree();
			shared_ptr<destructible> tvh(handle_tv_notifications(htree, &dummy_handler));

			::SetWindowText(::GetParent((HWND)htree), _T("abcdefg"));

			// ACT / ASSERT
			Assert::IsTrue(7 == ::GetWindowTextLength(::GetParent((HWND)htree)));
		}


		[TestMethod]
		void WindowWrappersAreDetachedOnHandlerDestruction()
		{
			// INIT
			HWND dummy = (HWND)create_window();
			HWND htree = (HWND)create_tree(), hparent = ::GetParent(htree);
			shared_ptr<destructible> tvh(handle_tv_notifications(htree, &dummy_handler));

			window_wrapper::attach(dummy);

			LONG_PTR wrapper_proc(::GetWindowLongPtr(dummy, GWLP_WNDPROC));

			// ACT / ASSERT
			Assert::IsTrue(wrapper_proc == ::GetWindowLongPtr(hparent, GWLP_WNDPROC));

			// ACT
			tvh.reset();

			// ASSERT
			Assert::IsFalse(wrapper_proc == ::GetWindowLongPtr(hparent, GWLP_WNDPROC));
		}


		[TestMethod]
		void PrepaintItemAndSubitemPassedToNotification()
		{
			// INIT
			void *htree = create_tree();
			vector< vector<wstring> > path_dest;
			NMTVCUSTOMDRAW n = { 0 };
			vector< vector<wstring> > paths;
			shared_ptr<destructible> tvh(handle_tv_notifications(htree, bind(&copy_arguments, ref(paths), _1, _2, _3, _4)));

			void *r1 = add_item(htree, 0, L"root #1");
			void *i11 = add_item(htree, r1, L"subitem #1");
			void *i12 = add_item(htree, r1, L"subitem #2");
			void *i121 = add_item(htree, i12, L"subsubitem #1");
			void *r2 = add_item(htree, 0, L"root #2");
			void *i21 = add_item(htree, r2, L"subitem #1");
			void *i211 = add_item(htree, i21, L"subsubitem #1");
			void *i212 = add_item(htree, i21, L"subsubitem #2");
			void *i22 = add_item(htree, r2, L"subitem #2");

			n.nmcd.hdr.code = NM_CUSTOMDRAW;

			// ACT
			n.nmcd.dwDrawStage = CDDS_ITEMPREPAINT;
			n.nmcd.dwItemSpec = (DWORD)r1;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);

			// ASSERT
			Assert::IsTrue(1 == paths.size());
			Assert::IsTrue(1 == paths[0].size());
			Assert::IsTrue(L"root #1" == paths[0][0]);

			// ACT
			n.nmcd.dwDrawStage = CDDS_ITEMPREPAINT | CDDS_SUBITEM;
			n.nmcd.dwItemSpec = (DWORD)i11;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i12;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i121;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)r2;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i21;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i211;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i212;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);
			n.nmcd.dwItemSpec = (DWORD)i22;
			::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);

			// ASSERT
			Assert::IsTrue(9 == paths.size());
			Assert::IsTrue(2 == paths[1].size());
			Assert::IsTrue(L"root #1" == paths[1][0]);
			Assert::IsTrue(L"subitem #1" == paths[1][1]);
			Assert::IsTrue(2 == paths[2].size());
			Assert::IsTrue(L"root #1" == paths[2][0]);
			Assert::IsTrue(L"subitem #2" == paths[2][1]);
			Assert::IsTrue(3 == paths[3].size());
			Assert::IsTrue(L"root #1" == paths[3][0]);
			Assert::IsTrue(L"subitem #2" == paths[3][1]);
			Assert::IsTrue(L"subsubitem #1" == paths[3][2]);
			Assert::IsTrue(1 == paths[4].size());
			Assert::IsTrue(L"root #2" == paths[4][0]);
			Assert::IsTrue(2 == paths[5].size());
			Assert::IsTrue(L"root #2" == paths[5][0]);
			Assert::IsTrue(L"subitem #1" == paths[5][1]);
			Assert::IsTrue(3 == paths[6].size());
			Assert::IsTrue(L"root #2" == paths[6][0]);
			Assert::IsTrue(L"subitem #1" == paths[6][1]);
			Assert::IsTrue(L"subsubitem #1" == paths[6][2]);
			Assert::IsTrue(3 == paths[7].size());
			Assert::IsTrue(L"root #2" == paths[7][0]);
			Assert::IsTrue(L"subitem #1" == paths[7][1]);
			Assert::IsTrue(L"subsubitem #2" == paths[7][2]);
			Assert::IsTrue(2 == paths[8].size());
			Assert::IsTrue(L"root #2" == paths[8][0]);
			Assert::IsTrue(L"subitem #2" == paths[8][1]);
		}
	};
}
