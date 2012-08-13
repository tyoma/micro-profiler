#include "Helpers.h"

#include <collector/calls_collector.h>
#include <common/primitives.h>

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

			class enterexit_emulator : public thread_function
			{
				calls_collector &_collector;
				size_t _calls_number;

				void operator =(const enterexit_emulator &);

			public:
				enterexit_emulator(calls_collector &collector, size_t calls_number)
					: _collector(collector), _calls_number(calls_number)
				{	}

				virtual void operator ()()
				{
					__int64 timestamp(0);

					for (unsigned int i = 0; i != _calls_number; ++i)
					{
						call_record c1 = {	timestamp++, (void *)0x00001000	}, c2 = {	timestamp++, (void *)0	};

						_collector.track(c1);
						_collector.track(c2);
					}
				}
			};
		}

		void clear_collection_traces()
		{
			collection_acceptor a;

			calls_collector::instance()->read_collected(a);
			calls_collector::instance()->read_collected(a);
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

				calls_collector::instance()->read_collected(a);
				a.collected.clear();

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


			[TestMethod]
			void MaxTraceLengthIsLimited()
			{
				// INIT
				calls_collector c1(67), c2(123), c3(127);
				enterexit_emulator e1(c1, 670), e2(c2, 1230), e3(c3, 635);
				collection_acceptor a1, a2, a3;

				// ACT (blockage during this test is equivalent to the failure)
				thread t1(e1), t2(e2), t3(e3);

				while (a1.total_entries < 1340)
				{
					thread::sleep(30);
					c1.read_collected(a1);
				}
				while (a2.total_entries < 2460)
				{
					thread::sleep(30);
					c2.read_collected(a2);
				}
				while (a3.total_entries < 1270)
				{
					thread::sleep(30);
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
