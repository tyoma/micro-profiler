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


		[TestMethod]
		void GetStateOfIntactVersionedFiles()
		{
			// INIT
			stub_listener l;
			DateTime dt1(2009, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 11, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 17, 4, DateTimeKind::Utc);
			String ^dir1(Path::Combine(m_location, L"sample"));
			String ^dir2(Path::Combine(m_location, L"sample/i"));

			Directory::CreateDirectory(Path::Combine(dir1, L"cvs"));
			Directory::CreateDirectory(Path::Combine(dir2, L"cvs"));
			FileStream ^entries1 = File::Create(Path::Combine(dir1, L"cvs/entries"));
			TextWriter ^tw1 = gcnew StreamWriter(entries1);
			FileStream ^entries2 = File::Create(Path::Combine(dir2, L"cvs/entries"));
			TextWriter ^tw2 = gcnew StreamWriter(entries2);

			tw1->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file1.cpp/1.5/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file123.cpp/1.5/Tue Jul  7 12:50:26 2008//");

			tw2->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw2->WriteLine("/file2.h/1.5/Tue Mar 11 12:51:13 2008//");
			tw2->WriteLine("/efile123.cpp/2.3/Tue Jul  7 12:50:26 2008//");
			tw2->WriteLine("/file2.cpp/2.1/Mon Nov 29 17:17:04 2010//");

			delete tw1;
			delete tw2;
			delete entries1;
			delete entries2;

			File::Create(Path::Combine(dir1, L"file1.cpp"))->Close();
			File::Create(Path::Combine(dir2, L"file2.h"))->Close();
			File::Create(Path::Combine(dir2, L"file2.cpp"))->Close();
			File::SetLastWriteTimeUtc(Path::Combine(dir1, L"file1.cpp"), dt1);
			File::SetLastWriteTimeUtc(Path::Combine(dir2, L"file2.h"), dt2);
			File::SetLastWriteTimeUtc(Path::Combine(dir2, L"file2.cpp"), dt3);

			shared_ptr<repository> r = repository::create_cvs_sc(make_native(dir1), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_intact == r->get_filestate(make_native(Path::Combine(dir1, L"file1.cpp"))));
			Assert::IsTrue(repository::state_intact == r->get_filestate(make_native(Path::Combine(dir2, L"file2.h"))));
			Assert::IsTrue(repository::state_intact == r->get_filestate(make_native(Path::Combine(dir2, L"file2.cpp"))));
		}


		[TestMethod]
		void GetStateOfMissingVersionedFiles()
		{
			// INIT
			stub_listener l;
			String ^dir1(Path::Combine(m_location, L"sample"));
			String ^dir2(Path::Combine(m_location, L"sample/i"));

			Directory::CreateDirectory(Path::Combine(dir1, L"cvs"));
			Directory::CreateDirectory(Path::Combine(dir2, L"cvs"));
			FileStream ^entries1 = File::Create(Path::Combine(dir1, L"cvs/entries"));
			TextWriter ^tw1 = gcnew StreamWriter(entries1);
			FileStream ^entries2 = File::Create(Path::Combine(dir2, L"cvs/entries"));
			TextWriter ^tw2 = gcnew StreamWriter(entries2);

			tw1->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file6.cpp/1.5/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file123.cpp/1.5/Tue Jul  7 12:50:26 2008//");

			tw2->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw2->WriteLine("/file3.h/1.5/Tue Mar 11 12:51:13 2008//");
			tw2->WriteLine("/efile123.cpp/2.3/Tue Jul  7 12:50:26 2008//");
			tw2->WriteLine("/file3.cpp/2.1/Mon Nov 29 17:17:04 2010//");

			delete tw1;
			delete tw2;
			delete entries1;
			delete entries2;

			shared_ptr<repository> r = repository::create_cvs_sc(make_native(dir1), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_missing == r->get_filestate(make_native(Path::Combine(dir1, L"file6.cpp"))));
			Assert::IsTrue(repository::state_missing == r->get_filestate(make_native(Path::Combine(dir2, L"file3.h"))));
			Assert::IsTrue(repository::state_missing == r->get_filestate(make_native(Path::Combine(dir2, L"file3.cpp"))));
		}


		[TestMethod]
		void GetStateOfUpdatedVersionedFiles()
		{
			// INIT
			stub_listener l;
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			String ^dir1(Path::Combine(m_location, L"sample"));
			String ^dir2(Path::Combine(m_location, L"sample/t"));

			Directory::CreateDirectory(Path::Combine(dir1, L"cvs"));
			Directory::CreateDirectory(Path::Combine(dir2, L"cvs"));
			FileStream ^entries1 = File::Create(Path::Combine(dir1, L"cvs/entries"));
			TextWriter ^tw1 = gcnew StreamWriter(entries1);
			FileStream ^entries2 = File::Create(Path::Combine(dir2, L"cvs/entries"));
			TextWriter ^tw2 = gcnew StreamWriter(entries2);

			tw1->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file1.cpp/1.5/Tue Jul  7 15:50:26 2009//");
			tw1->WriteLine("/file123.cpp/1.5/Tue Jul  7 12:50:26 2008//");

			tw2->WriteLine("/CustomerExperienceAgent.config/1.4/Tue Jul  7 15:50:26 2009//");
			tw2->WriteLine("/file2.h/1.5/Tue Mar 11 12:51:13 2008//");
			tw2->WriteLine("/efile123.cpp/2.3/Tue Jul  7 12:50:26 2008//");
			tw2->WriteLine("/file2.cpp/2.1/Mon Nov 29 17:17:04 2010//");

			delete tw1;
			delete tw2;
			delete entries1;
			delete entries2;

			File::Create(Path::Combine(dir1, L"file1.cpp"))->Close();
			File::Create(Path::Combine(dir2, L"file2.h"))->Close();
			File::Create(Path::Combine(dir2, L"file2.cpp"))->Close();
			File::SetLastWriteTimeUtc(Path::Combine(dir1, L"file1.cpp"), dt1);
			File::SetLastWriteTimeUtc(Path::Combine(dir2, L"file2.h"), dt2);
			File::SetLastWriteTimeUtc(Path::Combine(dir2, L"file2.cpp"), dt3);

			shared_ptr<repository> r = repository::create_cvs_sc(make_native(dir1), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_modified == r->get_filestate(make_native(Path::Combine(dir1, L"file1.cpp"))));
			Assert::IsTrue(repository::state_modified == r->get_filestate(make_native(Path::Combine(dir2, L"file2.h"))));
			Assert::IsTrue(repository::state_modified == r->get_filestate(make_native(Path::Combine(dir2, L"file2.cpp"))));
		}
	};
}
