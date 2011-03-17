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
using namespace mt;

namespace AppTests
{
	struct stub_listener : public repository::listener
	{
		stub_listener()
			: notification_arrived(false), calls(0)
		{	}

		virtual void modified(const vector< pair<wstring, repository::state> > &modifications)
		{
			++calls;
			this->modifications.insert(this->modifications.end(), modifications.begin(), modifications.end());
			notification_arrived.set();
		}

		event_monitor notification_arrived;
		vector< pair<wstring, repository::state> > modifications;
		int calls;
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
			temp_directory d_cvs(d.path() / L"CVS");

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


		[TestMethod]
		void NotifyOnFileUpdate()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			DateTime dt4(2011, 1, 29, 17, 18, 4, DateTimeKind::Utc);

			{
				entries_file e(d.path());

				e.append(L"thread.cpp", L"1.4", dt1);
				e.append(L"thread.h", L"1.5", dt2);
				e.append(L"mutex.cpp", L"1.6", dt3);
				e.append(L"mutex.h", L"1.7", dt4);
			}

			File::Create(make_managed(d.path() / L"thread.cpp"))->Close();
			File::Create(make_managed(d.path() / L"thread.h"))->Close();
			File::Create(make_managed(d.path() / L"mutex.cpp"))->Close();
			File::Create(make_managed(d.path() / L"mutex.h"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.h"), dt2);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"mutex.cpp"), dt3);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"mutex.h"), dt4);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			Assert::IsTrue(waitable::timeout == l.notification_arrived.wait(0));

			// ACT
			{
				StreamWriter ^s = File::AppendText(make_managed(d.path() / L"thread.h"));

				s->WriteLine("some new line");
				delete s;
			}
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			// ASSERT
			Assert::IsTrue(1 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"thread.h"), repository::state_modified) == l.modifications[0]);
		}


		[TestMethod]
		void NotifyOnSequentialFileUpdates()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			DateTime dt4(2011, 1, 29, 17, 18, 4, DateTimeKind::Utc);

			{
				entries_file e(d.path());

				e.append(L"thread.cpp", L"1.4", dt1);
				e.append(L"thread.h", L"1.5", dt2);
				e.append(L"mutex.cpp", L"1.6", dt3);
				e.append(L"mutex.h", L"1.7", dt4);
			}

			File::Create(make_managed(d.path() / L"thread.cpp"))->Close();
			File::Create(make_managed(d.path() / L"thread.h"))->Close();
			File::Create(make_managed(d.path() / L"mutex.cpp"))->Close();
			File::Create(make_managed(d.path() / L"mutex.h"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.h"), dt2);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"mutex.cpp"), dt3);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"mutex.h"), dt4);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			{
				StreamWriter ^s = File::AppendText(make_managed(d.path() / L"thread.cpp"));

				s->WriteLine("some new line");
				delete s;
			}
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			// ASSERT
			Assert::IsTrue(1 == l.calls);
			Assert::IsTrue(1 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"thread.cpp"), repository::state_modified) == l.modifications[0]);

			// INIT
			l.modifications.clear();
			l.calls = 0;
			l.notification_arrived.reset();

			// ACT / ASSERT
			{
				StreamWriter ^s = File::AppendText(make_managed(d.path() / L"mutex.cpp"));

				s->WriteLine("some new line");
				delete s;
			}
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			// ASSERT
			Assert::IsTrue(1 == l.calls);
			Assert::IsTrue(2 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"mutex.cpp"), repository::state_modified) == l.modifications[0]);
			Assert::IsTrue(make_pair(wstring(L"thread.cpp"), repository::state_modified) == l.modifications[1]);
		}


		[TestMethod]
		void NotifyOnNewUnversionedFile()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"sample");
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);

			{
				entries_file e(d.path());

				e.append(L"thread.cpp", L"1.4", dt1);
				e.append(L"thread.h", L"1.5", dt2);
			}

			File::Create(make_managed(d.path() / L"thread.cpp"))->Close();
			File::Create(make_managed(d.path() / L"thread.h"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d.path() / L"thread.h"), dt2);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT
			File::Create(make_managed(d.path() / L"process.h"))->Close();

			// ASSERT
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			Assert::IsTrue(1 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"process.h"), repository::state_unversioned) == l.modifications[0]);
		}


		[TestMethod]
		void NotifyOnNestedFileUpdates()
		{
			// INIT
			stub_listener l;
			temp_directory d(L"test"), d_inner(d.path() / L"Inner");
			DateTime dt1(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc);
			DateTime dt2(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc);
			DateTime dt3(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			DateTime dt4(2011, 1, 29, 17, 18, 4, DateTimeKind::Utc);

			{
				entries_file e(d.path());
				entries_file e_inner(d_inner.path());

				e_inner.append(L"thread.cpp", L"1.4", dt1);
				e_inner.append(L"thread.h", L"1.5", dt2);
				e_inner.append(L"mutex.cpp", L"1.6", dt3);
				e_inner.append(L"mutex.h", L"1.7", dt4);
			}

			File::Create(make_managed(d_inner.path() / L"thread.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"thread.h"))->Close();
			File::Create(make_managed(d_inner.path() / L"mutex.cpp"))->Close();
			File::Create(make_managed(d_inner.path() / L"mutex.h"))->Close();
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"thread.cpp"), dt1);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"thread.h"), dt2);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"mutex.cpp"), dt3);
			File::SetLastWriteTimeUtc(make_managed(d_inner.path() / L"mutex.h"), dt4);

			shared_ptr<repository> r = repository::create_cvs_sc(d.path(), l);

			// ACT / ASSERT
			{
				StreamWriter ^s = File::AppendText(make_managed(d_inner.path() / L"thread.cpp"));

				s->WriteLine("some new line");
				delete s;
			}
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			// ASSERT
			Assert::IsTrue(1 == l.calls);
			Assert::IsTrue(1 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"Inner/thread.cpp"), repository::state_modified) == l.modifications[0]);

			// INIT
			l.modifications.clear();
			l.calls = 0;
			l.notification_arrived.reset();

			// ACT / ASSERT
			File::Create(make_managed(d_inner.path() / L"process.h"))->Close();
			Assert::IsTrue(waitable::satisfied == l.notification_arrived.wait(waitable::infinite));

			// ASSERT
			Assert::IsTrue(1 == l.calls);
			Assert::IsTrue(2 == l.modifications.size());
			Assert::IsTrue(make_pair(wstring(L"Inner/process.h"), repository::state_unversioned) == l.modifications[0]);
			Assert::IsTrue(make_pair(wstring(L"Inner/thread.cpp"), repository::state_modified) == l.modifications[1]);
		}
	};
}
