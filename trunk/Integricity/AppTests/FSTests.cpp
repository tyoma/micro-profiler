#include <fs.h>

#include "TestHelpers.h"

#include <stdexcept>
#include <vector>

using namespace fs;
using namespace mt;
using namespace ut;
using namespace std;
using namespace System;
using namespace System::IO;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	namespace
	{
		static void wait_for_change(shared_ptr<waitable> w, unsigned int timeout, waitable::wait_status *ws)
		{ *ws = w->wait(timeout); }

		void sort(vector<directory_entry> entries)
		{	sort(entries.begin(), entries.end(), [] (const directory_entry &lhs, const directory_entry &rhs) { return lhs.name < rhs.name; });	}
	}

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
			shared_ptr<waitable> w = create_directory_monitor(d.path(), false);

			// ASSERT
			Assert::IsTrue(w != 0);
		}


		[TestMethod]
		void WaitingWithoutChangesResultsInTimeout()
		{
			// INIT
			temp_directory d(L"test");
			shared_ptr<waitable> w = create_directory_monitor(d.path(), false);

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
			shared_ptr<waitable> w = create_directory_monitor(d.path(), false);

			// ACT
			File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			waitable::wait_status s = w->wait(waitable::infinite);

			// ASSERT
			Assert::IsTrue(waitable::satisfied == s);
		}


		[TestMethod]
		void WaitingWithChangesResultsInWaitSatisfiedAsync()
		{
			// INIT
			temp_directory d(L"test-2");
			shared_ptr<waitable> w = create_directory_monitor(d.path(), false);
			waitable::wait_status ws = waitable::timeout;

			{
			// ACT
				thread t(bind(&wait_for_change, w, waitable::infinite, &ws));

			// ASSERT
				Assert::IsTrue(waitable::timeout == ws);

			// ACT
				File::Create(make_managed(d.path() / L"file1.cpp"))->Close();
			}

			// ASSERT
			Assert::IsTrue(waitable::satisfied == ws);
		}


		[TestMethod]
		void WaitingWithSubsequentChangesResultsInWaitSatisfiedSync()
		{
			// INIT
			temp_directory d(L"test-2");
			shared_ptr<waitable> w = create_directory_monitor(d.path(), false);

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


		[TestMethod]
		void IteratorForEmptyDirectoryEvalsToFalse()
		{
			// INIT
			temp_directory d(L"test-2");

			// ACT
			directory_iterator i(d.path());

			// ACT / ASSERT
			Assert::IsFalse(i);
		}


		[TestMethod]
		void IteratorForNonEmptyDirectoryEvalsToTrue()
		{
			// INIT
			temp_directory d(L"test-2");

			delete File::Create(make_managed(d.path() / L"somefile"));

			// ACT
			directory_iterator i(d.path());

			// ACT / ASSERT
			Assert::IsTrue(i);
		}


		[TestMethod]
		void GetEntryInfoForFiles()
		{
			// INIT
			temp_directory d1(L"test-1"), d2(L"test-2");
			DateTime dt1_c(2010, 7, 7, 15, 50, 26, DateTimeKind::Utc), dt1_m(2008, 3, 12, 12, 51, 13, DateTimeKind::Utc), dt1_a(2010, 11, 29, 17, 18, 4, DateTimeKind::Utc);
			DateTime dt2_c(2001, 7, 7, 15, 50, 26, DateTimeKind::Utc), dt2_m(2002, 3, 12, 12, 51, 13, DateTimeKind::Utc), dt2_a(2011, 11, 29, 17, 18, 4, DateTimeKind::Utc);

			delete File::Create(make_managed(d1.path() / L"somefile"));
			File::SetCreationTimeUtc(make_managed(d1.path() / L"somefile"), dt1_c);
			File::SetLastWriteTimeUtc(make_managed(d1.path() / L"somefile"), dt1_m);
			File::SetLastAccessTimeUtc(make_managed(d1.path() / L"somefile"), dt1_a);
			delete File::Create(make_managed(d2.path() / L"Another File"));
			File::SetCreationTimeUtc(make_managed(d2.path() / L"Another File"), dt2_c);
			File::SetLastWriteTimeUtc(make_managed(d2.path() / L"Another File"), dt2_m);
			File::SetLastAccessTimeUtc(make_managed(d2.path() / L"Another File"), dt2_a);

			// ACT
			directory_iterator i1(d1.path());
			directory_iterator i2(d2.path());

			// ACT / ASSERT
			const directory_entry &e1 = *i1;
			const directory_entry &e2 = *i2;

			Assert::IsTrue(e1.type == entry_file);
			Assert::IsTrue(e1.created == make_filetime(dt1_c));
			Assert::IsTrue(e1.modified == make_filetime(dt1_m));
			Assert::IsTrue(e1.accessed == make_filetime(dt1_a));
			Assert::IsTrue(e1.name == L"somefile");
			Assert::IsTrue(e2.type == entry_file);
			Assert::IsTrue(e2.created == make_filetime(dt2_c));
			Assert::IsTrue(e2.modified == make_filetime(dt2_m));
			Assert::IsTrue(e2.accessed == make_filetime(dt2_a));
			Assert::IsTrue(e2.name == L"Another File");
		}


		[TestMethod]
		void GetEntryInfoForDirectories()
		{
			// INIT
			temp_directory d1(L"test-1"), d2(L"test-2");
			temp_directory d_inner1(d1.path() / L"inner test-1"), d_inner2(d2.path() / L"inner test #2");

			// ACT
			directory_iterator i1(d1.path());
			directory_iterator i2(d2.path());

			// ACT / ASSERT
			const directory_entry &e1 = *i1;
			const directory_entry &e2 = *i2;

			Assert::IsTrue(e1.type == entry_directory);
			Assert::IsTrue(e1.name == L"inner test-1");
			Assert::IsTrue(e2.type == entry_directory);
			Assert::IsTrue(e2.name == L"inner test #2");
		}


		[TestMethod]
		void IterateThroughDirectoryEntries()
		{
			// INIT
			temp_directory d1(L"test-1"), d2(L"test-2");

			delete File::Create(make_managed(d1.path() / L"somefile"));
			delete File::Create(make_managed(d1.path() / L"somefile2"));
			delete File::Create(make_managed(d1.path() / L"somefile3"));
			delete File::Create(make_managed(d2.path() / L"Another File"));
			delete File::Create(make_managed(d2.path() / L"Yet Another File"));

			// ACT
			directory_iterator i1(d1.path());
			directory_iterator i2(d2.path());

			// ACT / ASSERT
			Assert::IsTrue(++++i1);
			Assert::IsFalse(++i1);
			Assert::IsTrue(++i2);
			Assert::IsFalse(++i2);
		}


		[TestMethod]
		void IterateThroughDirectoryEntriesAndGatherData()
		{
			// INIT
			temp_directory d(L"test-1"), d_inner(d.path() / L"inner");
			vector<directory_entry> entries;

			delete File::Create(make_managed(d.path() / L"somefile"));
			delete File::Create(make_managed(d.path() / L"somefile2"));
			delete File::Create(make_managed(d.path() / L"somefile3"));
			delete File::Create(make_managed(d.path() / L"Another File"));
			delete File::Create(make_managed(d.path() / L"Yet Another File"));

			directory_iterator i(d.path());

			// ACT
			for (directory_iterator i(d.path()); i; ++i)
				entries.push_back(*i);

			// ASSERT
			sort(entries);

			Assert::IsTrue(L"Another File" == entries[0].name);
			Assert::IsTrue(L"inner" == entries[1].name);
			Assert::IsTrue(L"somefile" == entries[2].name);
			Assert::IsTrue(L"somefile2" == entries[3].name);
			Assert::IsTrue(L"somefile3" == entries[4].name);
			Assert::IsTrue(L"Yet Another File" == entries[5].name);
		}
	};
}
