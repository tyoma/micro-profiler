#include <analyzer.h>

#include <pod_vector.h>

#include <map>

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			call_record _call_record(void *callee, unsigned __int64 timestamp)
			{
				call_record r = {	callee, timestamp	};

				return r;
			}
		}

		[TestClass]
		public ref class AnalyzerTests
		{
		public: 
			[TestMethod]
			void NewAnalyzerHasNoFunctionRecords()
			{
				// INIT / ACT
				analyzer a;

				// ACT / ASSERT
				Assert::IsTrue(a.begin() == a.end());
			}


			[TestMethod]
			void EnteringNewFunctionDoesNotCreateARecord()
			{
				// INIT
				analyzer a;
				calls_collector::acceptor &as_acceptor(a);
				pod_vector<call_record> trace;

				trace.append(_call_record((void *)1234, 12300));
				trace.append(_call_record((void *)2234, 12305));

				// ACT
				as_acceptor.accept_calls(1, trace.data(), trace.size());
				as_acceptor.accept_calls(2, trace.data(), trace.size());

				// ASSERT
				Assert::IsTrue(a.begin() == a.end());
			}


			[TestMethod]
			void EvaluateSeveralFunctionDurations()
			{
				// INIT
				analyzer a;
				pod_vector<call_record> trace;

				trace.append(_call_record((void *)1234, 12300));
				trace.append(_call_record(0, 12305));
				trace.append(_call_record((void *)2234, 12310));
				trace.append(_call_record(0, 12317));
				trace.append(_call_record((void *)2234, 12320));
				trace.append(_call_record((void *)12234, 12322));
				trace.append(_call_record(0, 12325));
				trace.append(_call_record(0, 12327));

				// ACT
				a.accept_calls(1, trace.data(), trace.size());

				// ASSERT
				map<void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(3 == m.size());

				map<void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				Assert::IsTrue(1 == i1->second.times_called);
				Assert::IsTrue(5 == i1->second.inclusive_time);
				Assert::IsTrue(5 == i1->second.exclusive_time);

				Assert::IsTrue(2 == i2->second.times_called);
				Assert::IsTrue(14 == i2->second.inclusive_time);
				Assert::IsTrue(11 == i2->second.exclusive_time);

				Assert::IsTrue(1 == i3->second.times_called);
				Assert::IsTrue(3 == i3->second.inclusive_time);
				Assert::IsTrue(3 == i3->second.exclusive_time);
			}


			[TestMethod]
			void DifferentShadowStacksAreMaintainedForEachThread()
			{
				// INIT
				analyzer a;
				pod_vector<call_record> trace1, trace2, trace3, trace4;
				map<void *, function_statistics> m;

				trace1.append(_call_record((void *)1234, 12300));
				trace2.append(_call_record((void *)1234, 12313));
				trace3.append(_call_record(0, 12307));
				trace4.append(_call_record(0, 12319));
				trace4.append(_call_record((void *)1234, 12323));

				// ACT
				a.accept_calls(1, trace1.data(), trace1.size());
				a.accept_calls(2, trace2.data(), trace2.size());

				// ASSERT
				Assert::IsTrue(a.begin() == a.end());

				// ACT
				a.accept_calls(1, trace3.data(), trace3.size());

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(1 == a.begin()->second.times_called);
				Assert::IsTrue(7 == a.begin()->second.inclusive_time);
				Assert::IsTrue(7 == a.begin()->second.exclusive_time);

				// ACT
				a.accept_calls(2, trace4.data(), trace4.size());

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(2 == a.begin()->second.times_called);
				Assert::IsTrue(13 == a.begin()->second.inclusive_time);
				Assert::IsTrue(13 == a.begin()->second.exclusive_time);
			}
		};
	}
}
