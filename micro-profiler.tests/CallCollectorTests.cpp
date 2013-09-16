#include <collector/calls_collector.h>
#include <common/primitives.h>

#include "Helpers.h"
#include "TracedFunctions.h"

#include <vector>
#include <utility>
#include <algorithm>

using wpl::mt::thread;
using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct collection_acceptor : calls_collector_i::acceptor
			{
				collection_acceptor()
					: total_entries(0)
				{	}

				virtual void accept_calls(unsigned int threadid, const call_record *calls, size_t count)
				{
					collected.push_back(make_pair(threadid, vector<call_record>()));
					collected.back().second.assign(calls, calls + count);
					total_entries += count;
				}

				size_t total_entries;
				vector< pair< thread::id, vector<call_record> > > collected;
			};

			bool call_trace_less(const pair< thread::id, vector<call_record> > &lhs, const pair< thread::id, vector<call_record> > &rhs)
			{	return lhs.first < rhs.first;	}

			void emulate_n_calls(calls_collector &collector, size_t calls_number)
			{
				__int64 timestamp(0);

				for (size_t i = 0; i != calls_number; ++i)
				{
					call_record c1 = {	timestamp++, (void *)0x00001000	}, c2 = {	timestamp++, (void *)0	};

					collector.track(c1);
					collector.track(c2);
				}
			}
		}


		[TestClass]
		public ref class CallCollectorTests
		{
		public:
			[TestInitialize]
			void ClearCollectionTraces()
			{
				collection_acceptor a;

				calls_collector::instance()->read_collected(a);
				calls_collector::instance()->read_collected(a);
			}

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
				traced::sleep_20();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsFalse(a.collected.empty());
				Assert::IsTrue(threadex::current_thread_id() == a.collected[0].first);
				Assert::IsTrue(2 == a.collected[0].second.size());
				Assert::IsTrue(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				Assert::IsTrue(&traced::sleep_20 == (void*)(reinterpret_cast<unsigned __int64>(a.collected[0].second[0].callee) - 5));
			}


			[TestMethod]
			void ReadNothingAfterPreviousReadingAndNoCalls()
			{
				// INIT
				collection_acceptor a;

				traced::sleep_20();
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
				thread::id threadid1, threadid2;

				calls_collector::instance()->read_collected(a);
				a.collected.clear();

				// ACT
				{
					thread t1(&traced::sleep_20), t2(&traced::sleep_20);

					threadid1 = t1.get_id();
					threadid2 = t2.get_id();
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
				Assert::IsTrue(&traced::sleep_20 == (void*)(reinterpret_cast<unsigned __int64>(a.collected[0].second[0].callee) - 5));

				Assert::IsTrue(threadid2 == a.collected[1].first);
				Assert::IsTrue(2 == a.collected[1].second.size());
				Assert::IsTrue(a.collected[1].second[0].timestamp < a.collected[1].second[1].timestamp);
				Assert::IsTrue(&traced::sleep_20 == (void*)(reinterpret_cast<unsigned __int64>(a.collected[1].second[0].callee) - 5));
			}


			[TestMethod]
			void CollectEntryExitOnNestingFunction()
			{
				// INIT
				collection_acceptor a;

				// ACT
				traced::nesting1();
				calls_collector::instance()->read_collected(a);

				// ASSERT
				Assert::IsFalse(a.collected.empty());
				Assert::IsTrue(threadex::current_thread_id() == a.collected[0].first);
				Assert::IsTrue(4 == a.collected[0].second.size());
				Assert::IsTrue(a.collected[0].second[0].timestamp < a.collected[0].second[1].timestamp);
				Assert::IsTrue(a.collected[0].second[1].timestamp < a.collected[0].second[2].timestamp);
				Assert::IsTrue(a.collected[0].second[2].timestamp < a.collected[0].second[3].timestamp);
				Assert::IsTrue(&traced::nesting1 == (void*)(reinterpret_cast<unsigned __int64>(a.collected[0].second[0].callee) - 5));
				Assert::IsTrue(&traced::sleep_20 == (void*)(reinterpret_cast<unsigned __int64>(a.collected[0].second[1].callee) - 5));
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


			[TestMethod]
			void MaxTraceLengthIsLimited()
			{
				// INIT
				calls_collector c1(67), c2(123), c3(127);
				collection_acceptor a1, a2, a3;

				// ACT (blockage during this test is equivalent to the failure)
				thread t1(bind(&emulate_n_calls, ref(c1), 670));
				thread t2(bind(&emulate_n_calls, ref(c2), 1230));
				thread t3(bind(&emulate_n_calls, ref(c3), 635));

				while (a1.total_entries < 1340)
				{
					threadex::sleep(30);
					c1.read_collected(a1);
				}
				while (a2.total_entries < 2460)
				{
					threadex::sleep(30);
					c2.read_collected(a2);
				}
				while (a3.total_entries < 1270)
				{
					threadex::sleep(30);
					c3.read_collected(a3);
				}

				// ASSERT
				Assert::IsTrue(a1.collected.size() == 20);
				Assert::IsTrue(a2.collected.size() == 20);
				Assert::IsTrue(a3.collected.size() == 10);
			}


			[TestMethod]
			void ReplyMaxTraceLength()
			{
				// INIT
				calls_collector c1(67), c2(123), c3(10050);

				// ACT / ASSERT
				Assert::IsTrue(67 == c1.trace_limit());
				Assert::IsTrue(123 == c2.trace_limit());
				Assert::IsTrue(10050 == c3.trace_limit());
			}


			[TestMethod]
			void GlobalCollectorInstanceTraceLimitVerify()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(5000000 == calls_collector::instance()->trace_limit());
			}
		};
	}
}
