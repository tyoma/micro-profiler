#include <windowing.h>

#include "TestHelpers.h"
#include <tchar.h>
#include <vector>
#include <functional>

using namespace std;
using namespace placeholders;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	struct handler
	{
		vector<MSG> messages;
		LRESULT myresult;

		handler()
			: myresult(0)
		{	}

		LRESULT on_message(UINT message, WPARAM wparam, LPARAM lparam, const function<LRESULT (UINT, WPARAM, LPARAM)> &previous)
		{
			MSG m = { 0 };

			m.message = message;
			m.wParam = wparam;
			m.lParam = lparam;

			messages.push_back(m);
			if (message == WM_SETTEXT && _tcscmp((LPCTSTR)lparam, _T("disallowed")) == 0)
				return 0;
			return !myresult ? previous(message, wparam, lparam) : myresult;
		}
	};

	LRESULT checker_handler(HWND hwnd, bool *exists)
	{
		*exists = 0 != ::GetProp(hwnd, _T("IntegricityWrapperPtr"));
		return 0;
	}

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
		void MessagesAreDelegatedToOriginalWndProcIfNotIntercepted()
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


		[TestMethod]
		void MessagesAreDelegatedToUserCBIfIntercepted()
		{
			// INIT
			HWND hwnd = create();
			handler h;
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));
			LPCTSTR s1 = _T("Text #1");
			LPCTSTR s2 = _T("Text #2");
			TCHAR buffer[5] = { 0 };

			// ACT
			shared_ptr<destructible> c(w->advise(bind(&handler::on_message, &h, _1, _2, _3, _4)));
			::SetWindowText(hwnd, s1);
			::SetWindowText(hwnd, s2);
			::GetWindowText(hwnd, buffer, 3);
			::GetWindowText(hwnd, buffer, 4);

			// ASSERT
			Assert::IsTrue(h.messages.size() == 4);
			Assert::IsTrue(h.messages[0].message == WM_SETTEXT);
			Assert::IsTrue(reinterpret_cast<LPCTSTR>(h.messages[0].lParam) == s1);
			Assert::IsTrue(h.messages[1].message == WM_SETTEXT);
			Assert::IsTrue(reinterpret_cast<LPCTSTR>(h.messages[1].lParam) == s2);
			Assert::IsTrue(h.messages[2].message == WM_GETTEXT);
			Assert::IsTrue(h.messages[2].wParam == 3);
			Assert::IsTrue(reinterpret_cast<LPCTSTR>(h.messages[2].lParam) == buffer);
			Assert::IsTrue(h.messages[3].message == WM_GETTEXT);
			Assert::IsTrue(h.messages[3].wParam == 4);
			Assert::IsTrue(reinterpret_cast<LPCTSTR>(h.messages[3].lParam) == buffer);
		}


		[TestMethod]
		void ResultIsProvidedFromInterceptor()
		{
			// INIT
			HWND hwnd = create();
			handler h;
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));

			shared_ptr<destructible> c(w->advise(bind(&handler::on_message, &h, _1, _2, _3, _4)));

			// ACT
			h.myresult = 123;
			int r1 = ::GetWindowTextLength(hwnd);
			h.myresult = 321;
			int r2 = ::GetWindowTextLength(hwnd);

			// ASSERT
			Assert::IsTrue(r1 == 123);
			Assert::IsTrue(r2 == 321);
		}


		[TestMethod]
		void MessageIsNotPassedToPreviousHandlerIfUserHandlerDoesNotCallIt()
		{
			// INIT
			HWND hwnd = create();
			handler h;
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));

			shared_ptr<destructible> c(w->advise(bind(&handler::on_message, &h, _1, _2, _3, _4)));

			// ACT
			::SetWindowText(hwnd, _T("allowed"));

			// ASSERT
			Assert::IsTrue(::GetWindowTextLength(hwnd) == 7);

			// ACT
			::SetWindowText(hwnd, _T("disallowed"));

			// ASSERT
			Assert::IsTrue(::GetWindowTextLength(hwnd) == 7);
		}


		[TestMethod]
		void InterceptionStopsIfConnectionDestroyed()
		{
			// INIT
			HWND hwnd = create();
			handler h;
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));
			shared_ptr<destructible> c(w->advise(bind(&handler::on_message, &h, _1, _2, _3, _4)));

			::SetWindowText(hwnd, _T("allowed"));
			::SetWindowText(hwnd, _T("disallowed"));

			// preASSERT
			Assert::IsTrue(::GetWindowTextLength(hwnd) == 7);

			// INIT
			size_t calls = h.messages.size();

			// ACT
			c.reset();
			::SetWindowText(hwnd, _T("disallowed"));

			// ASSERT
			Assert::IsTrue(::GetWindowTextLength(hwnd) == 10);
			Assert::IsTrue(h.messages.size() == calls);
		}


		[TestMethod]
		void WrapperPtrPropertyIsRemovedInNCDestroy()
		{
			// INIT
			HWND hwnd = create();
			bool property_exists = true;
			shared_ptr<window_wrapper> w(window_wrapper::attach(hwnd));
			shared_ptr<destructible> c(w->advise(bind(&checker_handler, hwnd, &property_exists)));

			// ACT / ASSERT
			Assert::IsFalse(::GetProp(hwnd, _T("IntegricityWrapperPtr")) == 0);

			// ACT
			::DestroyWindow(hwnd);

			// ASSERT
			Assert::IsFalse(property_exists);
		}
	};
}
