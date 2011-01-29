#include <utilities.h>

#include <vector>

using namespace std;
using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	[TestClass]
	public ref class UtilitiesTests
	{
	public:
		[TestMethod]
		void SplitToSingleEmptyRangeOnEmptyString()
		{
			// INIT
			const char *text1 = "";
			const wchar_t *text2 = L"";
			vector< pair<const char *, const char *> > r1;
			vector< pair<const wchar_t *, const wchar_t *> > r2;

			// ACT
			split(text1, text1 + 0, '|', back_inserter(r1));
			split(text2, text2 + 0, '|', back_inserter(r2));

			// ASSERT
			Assert::IsTrue(1 == r1.size());
			Assert::IsTrue(r1[0].first == r1[0].second);
			Assert::IsTrue(r1[0].second == text1);
			Assert::IsTrue(1 == r2.size());
			Assert::IsTrue(r2[0].first == r2[0].second);
			Assert::IsTrue(r2[0].second == text2);
		}


		[TestMethod]
		void SplitToSingleRangeNonEmpty()
		{
			// INIT
			const char *text1 = "abc";
			wstring text2(L"bcd");
			vector< pair<const char *, const char *> > r1;
			vector< pair<wstring::const_iterator, wstring::const_iterator> > r2;

			// ACT
			split(text1, text1 + 3, '|', back_inserter(r1));
			split(text2.begin(), text2.end(), '|', back_inserter(r2));

			// ASSERT
			Assert::IsTrue(1 == r1.size());
			Assert::IsTrue(r1[0].first == text1);
			Assert::IsTrue(r1[0].second == text1 + 3);
			Assert::IsTrue(1 == r2.size());
			Assert::IsTrue(r2[0].first == text2.begin());
			Assert::IsTrue(r2[0].second == text2.end());
		}


		[TestMethod]
		void SplitToTwoRangesNoEmptyParts()
		{
			// INIT
			const char *text1 = "abc/|bcd";
			wstring text2(L"cd|/edf");
			vector< pair<const char *, const char *> > r1;
			vector< pair<wstring::const_iterator, wstring::const_iterator> > r2;

			// ACT
			split(text1, text1 + 8, '/', back_inserter(r1));
			split(text2.begin(), text2.end(), L'|', back_inserter(r2));

			// ASSERT
			Assert::IsTrue(2 == r1.size());
			Assert::IsTrue(r1[0].first == text1);
			Assert::IsTrue(r1[0].second == text1 + 3);
			Assert::IsTrue(r1[1].first == text1 + 4);
			Assert::IsTrue(r1[1].second == text1 + 8);
			Assert::IsTrue(2 == r2.size());
			Assert::IsTrue(r2[0].first == text2.begin());
			Assert::IsTrue(r2[0].second == text2.begin() + 2);
			Assert::IsTrue(r2[1].first == text2.begin() + 3);
			Assert::IsTrue(r2[1].second == text2.end());
		}


		[TestMethod]
		void SplitToThreeRangesWithEmptyPart()
		{
			// INIT
			wstring text(L"cda||edf");
			vector< pair<wstring::const_iterator, wstring::const_iterator> > r;

			// ACT
			split(text.begin(), text.end(), L'|', back_inserter(r));

			// ASSERT
			Assert::IsTrue(3 == r.size());
			Assert::IsTrue(r[0].first == text.begin());
			Assert::IsTrue(r[0].second == text.begin() + 3);
			Assert::IsTrue(r[1].first == text.begin() + 4);
			Assert::IsTrue(r[1].second == text.begin() + 4);
			Assert::IsTrue(r[2].first == text.begin() + 5);
			Assert::IsTrue(r[2].second == text.begin() + 8);
		}


		[TestMethod]
		void SplitWithEmptyRangesOnBorders()
		{
			// INIT
			wstring text(L"..|.");
			vector< pair<wstring::const_iterator, wstring::const_iterator> > r;

			// ACT
			split(text.begin(), text.end(), L'.', back_inserter(r));

			// ASSERT
			Assert::IsTrue(4 == r.size());
			Assert::IsTrue(r[0].first == text.begin());
			Assert::IsTrue(r[0].second == text.begin());
			Assert::IsTrue(r[1].first == text.begin() + 1);
			Assert::IsTrue(r[1].second == text.begin() + 1);
			Assert::IsTrue(r[2].first == text.begin() + 2);
			Assert::IsTrue(r[2].second == text.begin() + 3);
			Assert::IsTrue(r[3].first == text.begin() + 4);
			Assert::IsTrue(r[3].second == text.begin() + 4);
		}
	};
}
