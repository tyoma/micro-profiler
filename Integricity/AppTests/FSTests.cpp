#include <fs.h>

#include "TestHelpers.h"

#include <stdexcept>

using namespace fs;
using namespace mt;
using namespace ut;
using namespace std;
using namespace System;
using namespace System::IO;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	[TestClass]
	public ref class FSTests
	{
	public: 
		[TestMethod]
		void ThrowOnInvalidPathForChangeWait()
		{
			// INIT / ACT / ASSERT
			ASSERT_THROWS(create_directory_monitor(L"abcdsdfsdgsd", false), runtime_error);
		}


		[TestMethod]
		void CreateChangeNotifierOnValidPath()
		{
			// INIT
			temp_directory d(L"test");

			// ACT
			shared_ptr<mt::waitable> w = create_directory_monitor(d.path(), false);

			// ASSERT
			Assert::IsTrue(w != 0);
		}


		[TestMethod]
		void WaitingWithoutChangesResultsInTimeout()
		{
			// INIT
			temp_directory d(L"test");
			shared_ptr<mt::waitable> w = create_directory_monitor(d.path(), false);

			// ACT
			waitable::wait_status s = w->wait(100);

			// ASSERT
			Assert::IsTrue(waitable::timeout == s);
		}


		[TestMethod]
		void WaitingWithChangesResultsInWaitSatisfiedSync()
		{
			// INIT
			temp_directory d(L"test-2");
			shared_ptr<mt::waitable> w = create_directory_monitor(d.path(), false);

			// ACT
			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			waitable::wait_status s = w->wait(waitable::infinite);

			// ASSERT
			Assert::IsTrue(waitable::satisfied == s);
		}


		[TestMethod]
		void WaitingWithSubsequentChangesResultsInWaitSatisfiedSync()
		{
			// INIT
			temp_directory d(L"test-2");
			shared_ptr<mt::waitable> w = create_directory_monitor(d.path(), false);

			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			w->wait(waitable::infinite);

			// ACT
			waitable::wait_status s = w->wait(100);

			// ASSERT
			Assert::IsTrue(waitable::timeout == s);

			// INIT
			File::Create(make_managed(d.path() / L"file2.h"))->Close();

			// ACT
			s = w->wait(waitable::infinite);

			// ASSERT
			Assert::IsTrue(waitable::satisfied == s);
		}
	};
}
