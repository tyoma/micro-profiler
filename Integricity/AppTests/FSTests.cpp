#include <fs.h>

#include "TestHelpers.h"

#include <stdexcept>

using namespace fs;
using namespace ut;
using namespace std;
using namespace System;
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
			ASSERT_THROWS(create_change_notifier(L"abcdsdfsdgsd", false), runtime_error);
		}


		[TestMethod]
		void CreateChangeNotifierOnValidPath()
		{
			// INIT
			temp_directory d(L"test");

			// ACT
			shared_ptr<mt::waitable> w = create_change_notifier(d.path(), false);

			// ASSERT
			Assert::IsTrue(w != 0);
		}
	};
}
