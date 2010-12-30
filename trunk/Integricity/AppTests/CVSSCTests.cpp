#include <interface.h>

#include "TestHelpers.h"
#include <exception>

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace testing;
using namespace std;

namespace AppTests
{
	class stub_listener : public source_control::listener
	{
		virtual void modified(const std::vector< std::pair<std::wstring, source_control::state> > &modifications)
		{
		}
	};

	[TestClass]
	public ref class CVSSourceControlTests
	{
	public: 
		[TestMethod]
		void FailToCreateSCOnWrongPath()
		{
			// INIT
			stub_listener l;

			// ACT / ASSERT
			try
			{
				source_control::create_cvs_sc(L"", l);
				Assert::Fail("Must throw an exception!");
			}
			catch (invalid_argument &)
			{
			}
			catch (...)
			{
				Assert::Fail("Unexpected exception thrown!");
			}
		}
	};
}
