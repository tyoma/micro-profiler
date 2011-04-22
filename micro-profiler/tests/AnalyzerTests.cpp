#include <analyzer.h>

#include <pod_vector.h>

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
			trace.append(_call_record((void *)2234, 12300));

			// ACT
			as_acceptor.accept_calls(1, trace.data(), trace.size());
			as_acceptor.accept_calls(2, trace.data(), trace.size());

			// ASSERT
			Assert::IsTrue(a.begin() == a.end());
		}
	};
}
