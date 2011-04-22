#include <analyzer.h>

#include <pod_vector.h>

#include <map>

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace tests
{
	namespace
	{
		micro_profiler::call_record _call_record(void *callee, unsigned __int64 timestamp)
		{
			micro_profiler::call_record r = {	callee, timestamp	};

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
			micro_profiler::analyzer a;

			// ACT / ASSERT
			Assert::IsTrue(a.begin() == a.end());
		}


		[TestMethod]
		void EnteringNewFunctionDoesNotCreateARecord()
		{
			// INIT
			micro_profiler::analyzer a;
			micro_profiler::calls_collector::acceptor &as_acceptor(a);
			micro_profiler::pod_vector<micro_profiler::call_record> trace;

			trace.append(_call_record((void *)1234, 12300));
			trace.append(_call_record((void *)2234, 12305));

			// ACT
			as_acceptor.accept_calls(1, trace.data(), trace.size());
			as_acceptor.accept_calls(2, trace.data(), trace.size());

			// ASSERT
			Assert::IsTrue(a.begin() == a.end());
		}


		[TestMethod]
		void EvaluateSeparateNonNestingFunctionDurations()
		{
			// INIT
			micro_profiler::analyzer a;
			micro_profiler::pod_vector<micro_profiler::call_record> trace;

			trace.append(_call_record((void *)1234, 12300));
			trace.append(_call_record(0, 12305));
			trace.append(_call_record((void *)2234, 12310));
			trace.append(_call_record(0, 12317));

			// ACT
			a.accept_calls(1, trace.data(), trace.size());

			// ASSERT
			map<void *, micro_profiler::function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting
			map<void *, micro_profiler::function_statistics>::const_iterator f1(m.begin()), f2(m.begin());

			++f2;

			Assert::IsTrue(2 == m.size());

			Assert::IsTrue(1 == f1->second.times_called);
			Assert::IsTrue(5 == f1->second.inclusive_time);
			Assert::IsTrue(5 == f1->second.exclusive_time);

			Assert::IsTrue(1 == f2->second.times_called);
			Assert::IsTrue(7 == f2->second.inclusive_time);
			Assert::IsTrue(7 == f2->second.exclusive_time);
		}
	};
}
