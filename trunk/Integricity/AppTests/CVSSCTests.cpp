#include <repository.h>

#include "TestHelpers.h"

#include <fs.h>
#include <exception>

using namespace System;
using namespace System::IO;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;
using namespace fs;
using namespace ut;

namespace AppTests
{
	class stub_listener : public repository::listener
	{
		virtual void modified(const vector< pair<wstring, repository::state> > &modifications)
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
			ASSERT_THROWS(repository::create_cvs_sc(L"", l), invalid_argument);
		}


		[TestMethod]
		void FailToCreateSCOnNonCVSPath()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			temp_directory d_cvs(d.path() / L"cvs");

			// ACT / ASSERT
			ASSERT_THROWS(repository::create_cvs_sc(d.path(), l), invalid_argument);
		}


		[TestMethod]
		void SuccessfullyCreateSCOnValidCVSPath()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"inner");
			entries_file e1(d.path());
			entries_file e2(d_inner.path());

			// ACT / ASSERT (must not throw)
			shared_ptr<repository> r1 = repository::create_cvs_sc(d.path(), l);
			shared_ptr<repository> r2 = repository::create_cvs_sc(d_inner.path(), l);

			// ASSERT
			Assert::IsTrue(r1 != 0);
			Assert::IsTrue(r2 != 0);
		}


		[TestMethod]
		void GetStateOfUnversionedFiles()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"inner");
			entries_file e1(d.path());
			entries_file e2(d_inner.path());

			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.h"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.cpp"))->Close();

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(d.path() / L"file1.cpp"));
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(d_inner.path() / L"file2.h"));
			Assert::IsTrue(repository::state_unversioned == r->get_filestate(d_inner.path() / L"file2.cpp"));
		}


		[TestMethod]
		void GetStateOfIntactVersionedFiles()
		{
			// INIT
			stub_listener l;
			DateTime dt1(2009, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 11, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 17, 4, DateTimeKind::Utc);
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"i");

			{
				entries_file e1(d.path());
				entries_file e2(d_inner.path());

				e1.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e1.append(L"file1.cpp", L"1.5", dt1);
				e1.append(L"file123.cpp", L"1.5", DateTime(2008, 7, 7, 12, 50, 26));

				e2.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e2.append(L"file2.h", L"1.5", dt2);
				e2.append(L"efile123.cpp", L"2.3", DateTime(2008, 7, 7, 12, 50, 26));
				e2.append(L"file2.cpp", L"2.1", dt3);
			}

			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.h"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.cpp"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"file1.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"file2.h"), dt2);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"file2.cpp"), dt3);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_intact == r->get_filestate(d.path() / L"file1.cpp"));
			Assert::IsTrue(repository::state_intact == r->get_filestate(d_inner.path() / L"file2.h"));
			Assert::IsTrue(repository::state_intact == r->get_filestate(d_inner.path() / L"file2.cpp"));
		}


		[TestMethod]
		void GetStateOfMissingVersionedFiles()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"i");

			{
				entries_file e1(d.path());
				entries_file e2(d_inner.path());

				e1.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e1.append(L"file6.cpp", L"1.5", DateTime(2009, 7, 7, 15, 50, 26, DateTimeKind::Utc));
				e1.append(L"file123.cpp", L"1.5", DateTime(2008, 7, 7, 12, 50, 26));

				e2.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e2.append(L"file3.h", L"1.5", DateTime(2008, 3, 11, 12, 51, 13, DateTimeKind::Utc));
				e2.append(L"efile123.cpp", L"2.3", DateTime(2008, 7, 7, 12, 50, 26));
				e2.append(L"file3.cpp", L"2.1", DateTime(2010, 11, 29, 17, 17, 4, DateTimeKind::Utc));
			}

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_missing == r->get_filestate(d.path() / L"file6.cpp"));
			Assert::IsTrue(repository::state_missing == r->get_filestate(d_inner.path() / L"file3.h"));
			Assert::IsTrue(repository::state_missing == r->get_filestate(d_inner.path() / L"file3.cpp"));
		}


		[TestMethod]
		void GetStateOfUpdatedVersionedFiles()
		{
			// INIT
			stub_listener l;
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"t");

			{
				entries_file e1(d.path());
				entries_file e2(d_inner.path());

				e1.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e1.append(L"file1.cpp", L"1.5", dt1.AddYears(-1));
				e1.append(L"file123.cpp", L"1.5", DateTime(2008, 7, 7, 12, 50, 26));

				e2.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e2.append(L"file2.h", L"1.5", dt2.AddMonths(-1));
				e2.append(L"efile123.cpp", L"2.3", DateTime(2008, 7, 7, 12, 50, 26));
				e2.append(L"file2.cpp", L"2.1", dt3.AddHours(-1));
			}

			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.h"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.cpp"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"file1.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"file2.h"), dt2);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"file2.cpp"), dt3);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_modified == r->get_filestate(d.path() / L"file1.cpp"));
			Assert::IsTrue(repository::state_modified == r->get_filestate(d_inner.path() / L"file2.h"));
			Assert::IsTrue(repository::state_modified == r->get_filestate(d_inner.path() / L"file2.cpp"));
		}


		[TestMethod]
		void GetStateOfNewVersionedFiles()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			temp_directory d_inner(d.path() / L"t");

			{
				entries_file e1(d.path());
				entries_file e2(d_inner.path());

				e1.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e1.append_new(L"file1.cpp");
				e1.append(L"file123.cpp", L"1.5", DateTime(2008, 7, 7, 12, 50, 26));

				e2.append(L"CustomerExperienceAgent.config", L"1.4", DateTime(2009, 7, 7, 15, 50, 26));
				e2.append_new(L"file2.h");
				e2.append(L"efile123.cpp", L"2.3", DateTime(2008, 7, 7, 12, 50, 26));
				e2.append_new(L"file2.cpp");
			}

			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.h"))->Close();
			File::Create(make_managed(d_inner.path() / L"file2.cpp"))->Close();

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(repository::state_new == r->get_filestate(d.path() / L"file1.cpp"));
			Assert::IsTrue(repository::state_new == r->get_filestate(d_inner.path() / L"file2.h"));
			Assert::IsTrue(repository::state_new == r->get_filestate(d_inner.path() / L"file2.cpp"));
		}
	};
}
