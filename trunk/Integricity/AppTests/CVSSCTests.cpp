#include <interface.h>

#include "TestHelpers.h"
#include <exception>
#include <vcclr.h>

using namespace System;
using namespace System::IO;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace AppTests
{
	class stub_listener : public repository::listener
	{
		virtual void modified(const std::vector< std::pair<std::wstring, repository::state> > &modifications)
		{
		}
	};

	[TestClass]
	public ref class CVSSourceControlTests
	{
		static String^ m_location;

		static wstring make_native(String ^managed_string)
		{
			pin_ptr<const wchar_t> buffer(PtrToStringChars(managed_string));

			return wstring(buffer);
		}

	public:
		[AssemblyInitialize]
		static void _Init(TestContext testContext)
		{
			m_location = testContext.TestDeploymentDir;
		}

		[TestMethod]
		void FailToCreateSCOnWrongPath()
		{
			// INIT
			stub_listener l;

			// ACT / ASSERT
			ASSERT_THROWS(repository::create_cvs_sc(L"", l), invalid_argument);
		}


		[TestMethod]
		void FailToCreateSCOnNonCVSPath()
		{
			// INIT
			stub_listener l;
			String ^dir(Path::Combine(m_location, L"sample"));

			Directory::CreateDirectory(Path::Combine(dir, L"cvs"));

			// ACT / ASSERT
			ASSERT_THROWS(repository::create_cvs_sc(make_native(dir), l), invalid_argument);
		}


		[TestMethod]
		void SuccessfullyCreateSCOnValidCVSPath()
		{
			// INIT
			stub_listener l;
			String ^dir(Path::Combine(m_location, L"sample"));

			Directory::CreateDirectory(Path::Combine(dir, L"cvs"));
			File::Create(Path::Combine(dir, L"cvs/entries"));

			// ACT / ASSERT (must not throw)
			shared_ptr<repository> r = repository::create_cvs_sc(make_native(dir), l);

			// ASSERT
			Assert::IsTrue(r != 0);
		}
	};
}
