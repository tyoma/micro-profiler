#include <repository.h>

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

		[TestCleanup]
		void _Cleanup()
		{
			String ^path = Path::Combine(m_location, "sample");

			if (Directory::Exists(path))
				Directory::Delete(path, true);
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
			String ^dir2(Path::Combine(m_location, L"sample/inner"));

			Directory::CreateDirectory(Path::Combine(dir, L"cvs"));
			Directory::CreateDirectory(Path::Combine(dir2, L"cvs"));
			File::Create(Path::Combine(dir, L"cvs/entries"))->Close();
			File::Create(Path::Combine(dir2, L"cvs/entries"))->Close();

			// ACT / ASSERT (must not throw)
			shared_ptr<repository> r1 = repository::create_cvs_sc(make_native(dir), l);
			shared_ptr<repository> r2 = repository::create_cvs_sc(make_native(dir2), l);

			// ASSERT
			Assert::IsTrue(r1 != 0);
			Assert::IsTrue(r2 != 0);
		}


		[TestMethod]
		void GetStateOfUnversionedFiles()
		{
			// INIT
			stub_listener l;
			String ^dir1(Path::Combine(m_location, L"sample"));
			String ^dir2(Path::Combine(m_location, L"sample/inner"));

			Directory::CreateDirectory(Path::Combine(dir1, L"cvs"));
			Directory::CreateDirectory(Path::Combine(dir2, L"cvs"));
			File::Create(Path::Combine(dir1, L"cvs/entries"))->Close();
			File::Create(Path::Combine(dir2, L"cvs/entries"))->Close();

			File::Create(Path::Combine(dir1, L"file1.cpp"))->Close();
			File::Create(Path::Combine(dir2, L"file2.h"))->Close();
			File::Create(Path::Combine(dir2, L"file2.cpp"))->Close();

			shared_ptr<repository> r = repository::create_cvs_sc(make_native(dir1), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(make_native(Path::Combine(dir1, L"file1.cpp"))));
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(make_native(Path::Combine(dir2, L"file2.h"))));
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(make_native(Path::Combine(dir2, L"file2.cpp"))));
		}
	};
}
