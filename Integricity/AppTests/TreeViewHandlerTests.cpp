#include <treeview_handler.h>

#include "TestHelpers.h"
#include <windows.h>
#include <commctrl.h>

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
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
			shared_ptr<treeview_handler> tvh(new treeview_handler(htree));
			LRESULT result = ::SendMessage(::GetParent((HWND)htree), WM_NOTIFY, 0, (LPARAM)&n);

			// ASSERT
			Assert::IsTrue(CDRF_NOTIFYITEMDRAW == result);
		}


		[TestMethod]
		void PassthroughToPrevious()
		{
			// INIT
			void *htree = create_tree();
			shared_ptr<treeview_handler> tvh(new treeview_handler(htree));

			::SetWindowText(::GetParent((HWND)htree), _T("abcdefg"));

			// ACT / ASSERT
			Assert::IsTrue(7 == ::GetWindowTextLength(::GetParent((HWND)htree)));
		}
	};
}
