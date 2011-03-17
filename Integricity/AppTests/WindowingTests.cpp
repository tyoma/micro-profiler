#include <windowing.h>

#include "TestHelpers.h"
#include <tchar.h>
#include <vector>

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	[TestClass]
	public ref class WindowingTests
	{
		vector<HWND> *_tocleanup;

		HWND create(const TCHAR *class_name)
		{
			HWND hwnd = ::CreateWindow(class_name, NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, NULL, NULL);

			_tocleanup->push_back(hwnd);
			return hwnd;
		}

		HWND create()
		{	return create(_T("static"));	}

	public:
		WindowingTests()
			: _tocleanup(new vector<HWND>())
		{	}
		~WindowingTests()
		{	delete _tocleanup;	}

		[TestCleanup]
		void CleanupCreatedWindows()
		{
			for (vector<HWND>::const_iterator i = _tocleanup->begin(); i != _tocleanup->end(); ++i)
				if (::IsWindow(*i))
					::DestroyWindow(*i);
		}

		[TestMethod]
		void FailToWrapNonWindow()
		{
			// INIT / ACT / ASSERT
			ASSERT_THROWS(window_wrapper::attach((HWND)0x12345678), invalid_argument);
		}

	
		[TestMethod]
		void WrapWindowAndReturnWrapper()
		{
			// INIT
			HWND hwnd = create();

			// ACT (must not throw)
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));

			// ASSERT
			Assert::IsTrue(w != 0);
		}

	
		[TestMethod]
		void WrappersForDifferentWindowsAreDifferent()
		{
			// INIT
			HWND hwnd1 = create(), hwnd2 = create();

			// ACT
			shared_ptr<window_wrapper> w1(window_wrapper::attach(hwnd1));
			shared_ptr<window_wrapper> w2(window_wrapper::attach(hwnd2));

			// ASSERT
			Assert::IsTrue(w1 != w2);
		}

	
		[TestMethod]
		void WrapperHoldsHWND()
		{
			// INIT
			HWND hwnd1 = create(), hwnd2 = create();
			shared_ptr<window_wrapper> w1(window_wrapper::attach(hwnd1));
			shared_ptr<window_wrapper> w2(window_wrapper::attach(hwnd2));

			// ACT / ASSERT
			Assert::IsTrue(hwnd1 == w1->hwnd());
			Assert::IsTrue(hwnd2 == w2->hwnd());
		}

	
		[TestMethod]
		void WrapperForTheSameWindowIsTheSame()
		{
			// INIT
			HWND hwnd = create();

			// ACT
			shared_ptr<window_wrapper> w1(window_wrapper::attach(hwnd));
			shared_ptr<window_wrapper> w2(window_wrapper::attach(hwnd));

			// ASSERT
			Assert::IsTrue(w1 == w2);
		}

	
		[TestMethod]
		void SameWrapperForTheSameWindowIsReclaimed()
		{
			// INIT
			HWND hwnd = create();

			// ACT
			window_wrapper * w1(window_wrapper::attach(hwnd).get());	// do not hold reference
			shared_ptr<window_wrapper> w2(window_wrapper::attach(hwnd));

			// ASSERT
			Assert::IsTrue(w2.get() == w1);
		}

	
		[TestMethod]
		void WrapperDestroyedOnDestroyWindow()
		{
			// INIT
			HWND hwnd = create();

			// ACT
			weak_ptr<window_wrapper> w(window_wrapper::attach(hwnd));

			// ASSERT
			Assert::IsFalse(w.expired());

			// ACT
			::DestroyWindow(hwnd);

			// ASSERT
			Assert::IsTrue(w.expired());
		}

	
		[TestMethod]
		void MessagesAreDelegatedToOriginalWndProc()
		{
			// INIT
			HWND hwnd = create();
			TCHAR buffer[256] = { 0 };
			
			window_wrapper::attach(hwnd);

			// ACT
			::SetWindowText(hwnd, _T("First message..."));
			::GetWindowText(hwnd, buffer, 255);

			// ASSERT
			Assert::IsTrue(0 == _tcscmp(buffer, _T("First message...")));

			// ACT
			::SetWindowText(hwnd, _T("Message #2"));
			::GetWindowText(hwnd, buffer, 255);

			// ASSERT
			Assert::IsTrue(0 == _tcscmp(buffer, _T("Message #2")));
		}
	};
}
