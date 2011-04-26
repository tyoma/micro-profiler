#include "Helpers.h"

#include <calls_collector.h>

#include <vector>
#include <utility>
#include <algorithm>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		void sleep_20();
		void nesting1();

		namespace
		{
			struct collection_acceptor : calls_collector::acceptor
			{
				virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
				{
					collected.push_back(make_pair(threadid, vector<call_record>()));
					collected.back().second.assign(calls, calls + count);
				}

				vector< pair< unsigned int, vector<call_record> > > collected;
			};

			bool call_trace_less(const pair< unsigned int, vector<call_record> > &lhs, const pair< unsigned int, vector<call_record> > &rhs)
			{	return lhs.first < rhs.first;	}

			class executor : public thread_function
			{
				void (*_function)();

			public:
				explicit executor(void (*function)())
					: _function(function)
				{	}

				virtual void operator ()()
				{	_function();	}
			};
		}

		[TestClass]
		public ref class CallCollectorTests
		{
		public:
			[TestMethod]
			void CollectNothingOnNoCalls()
			{
				// INIT
				collection_acceptor a;

				// ACT
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsTrue(a.collected.empty());
			}


			[TestMethod]
			void CollectEntryExitOnSimpleExternalFunction()
			{
				// INIT
				collection_acceptor a;

				// ACT
				sleep_20();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsFalse(a.collected.empty());
				Assert::IsTrue(thread::current_thread_id() == a.collected[0].first);
				Assert::IsTrue(2 == a.collected[0].second.size());
				Assert::IsTrue(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				Assert::IsTrue(&sleep_20 == (void*)(reinterpret_cast<unsigned int>(a.collected[0].second[0].callee) - 5));
			}


			[TestMethod]
			void ReadNothingAfterPreviousReadingAndNoCalls()
			{
				// INIT
				collection_acceptor a;

				sleep_20();
				calls_collector::instance()->read_collected(a);

				a.collected.clear();

				// ACT
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsTrue(a.collected.empty());
			}


			[TestMethod]
			void ReadCollectedFromOtherThreads()
			{
				// INIT
				collection_acceptor a;
				unsigned threadid1, threadid2;

				calls_collector::instance()->read_collected(a);
				a.collected.clear();

				// ACT
				{
					executor e(sleep_20);
					thread t1(e, true), t2(e, true);

					threadid1 = t1.id();
					threadid2 = t2.id();
					t1.resume();
					t2.resume();
				}

				calls_collector::instance()->read_collected(a);

				// ASSERT
				sort(a.collected.begin(), a.collected.end(), &call_trace_less);
				if (threadid1 > threadid2)
					swap(threadid1, threadid2);

				Assert::IsTrue(2 == a.collected.size());

				Assert::IsTrue(threadid1 == a.collected[0].first);
				Assert::IsTrue(2 == a.collected[0].second.size());
				Assert::IsTrue(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				Assert::IsTrue(&sleep_20 == (void*)(reinterpret_cast<unsigned int>(a.collected[0].second[0].callee) - 5));

				Assert::IsTrue(threadid2 == a.collected[1].first);
				Assert::IsTrue(2 == a.collected[1].second.size());
				Assert::IsTrue(a.collected[1].second[0].timestamp < a.collected[1].second[1].timestamp);
				Assert::IsTrue(&sleep_20 == (void*)(reinterpret_cast<unsigned int>(a.collected[1].second[0].callee) - 5));
			}


			[TestMethod]
			void CollectEntryExitOnNestingFunction()
			{
				// INIT
				collection_acceptor a;

				// ACT
				nesting1();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsFalse(a.collected.empty());
				Assert::IsTrue(thread::current_thread_id() == a.collected[0].first);
				Assert::IsTrue(4 == a.collected[0].second.size());
				Assert::IsTrue(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				Assert::IsTrue(a.collected[0].second[1].timestamp < a.collected[0].second[2].timestamp);
				Assert::IsTrue(a.collected[0].second[2].timestamp < a.collected[0].second[3].timestamp);
				Assert::IsTrue(&nesting1 == (void*)(reinterpret_cast<unsigned int>(a.collected[0].second[0].callee) - 5));
				Assert::IsTrue(&sleep_20 == (void*)(reinterpret_cast<unsigned int>(a.collected[0].second[1].callee) - 5));
				Assert::IsTrue(0 == a.collected[0].second[2].callee);
				Assert::IsTrue(0 == a.collected[0].second[3].callee);
			}


			[TestMethod]
			void ProfilerLatencyGreaterThanZero()
			{
				// INIT / ACT
				unsigned __int64 profiler_latency = calls_collector::instance()->profiler_latency();

				// ASSERT
				Assert::IsTrue(profiler_latency > 0);
				System::Diagnostics::Debug::WriteLine(profiler_latency);
			}
		};
	}
}
