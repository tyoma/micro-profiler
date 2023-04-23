#include <scheduler/task_node.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		namespace
		{
			template <typename T>
			exception_ptr make_exception(const T &e)
			{
				try
				{	throw e;	}
				catch (...)
				{	return current_exception();	}
			}

			template <typename T, typename F>
			struct continuation_impl : continuation<T>, F
			{
				continuation_impl(const F &from)
					: F(from)
				{	}

				virtual void begin(const shared_ptr< const async_result<T> > &antecedant) override
				{	(*this)(*antecedant);	}
			};

			template <typename F, typename ArgT>
			struct task_result
			{	typedef decltype((*static_cast<F *>(nullptr))(*static_cast<async_result<ArgT> *>(nullptr))) type;	};

			template <typename T, typename F>
			typename continuation<T>::ptr make_continuation(const F &f)
			{	return make_shared< continuation_impl<T, F> >(f);	}
		}

		begin_test_suite( TaskNodeTests )
			test( SettingResultInvokesRegisteredContinuations )
			{
				// INIT
				vector< pair<int, int> > log1;
				vector< pair<int, string> > log2;
				auto log3 = 0;
				auto n1 = make_shared< task_node<int> >();
				auto n2 = make_shared< task_node<int> >();
				auto n3 = make_shared< task_node<string> >();
				auto n4 = make_shared< task_node<string> >();
				auto n5 = make_shared< task_node<void> >();
				auto n6 = make_shared< task_node<void> >();

				// INIT / ACT
				n1->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(make_pair(0, *v));	}));
				n1->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(make_pair(1, *v));	}));
				n1->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(make_pair(2, *v));	}));
				n2->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(make_pair(3, *v));	}));

				n3->continue_with(make_continuation<string>([&] (const async_result<string> &v) {	log2.push_back(make_pair(0, *v));	}));
				n3->continue_with(make_continuation<string>([&] (const async_result<string> &v) {	log2.push_back(make_pair(1, *v));	}));
				n4->continue_with(make_continuation<string>([&] (const async_result<string> &v) {	log2.push_back(make_pair(2, *v));	}));

				n5->continue_with(make_continuation<void>([&] (const async_result<void> &v) {	*v, log3++;	}));
				n5->continue_with(make_continuation<void>([&] (const async_result<void> &v) {	*v, log3++;	}));
				n6->continue_with(make_continuation<void>([&] (const async_result<void> &v) {	*v, log3++;	}));

				// ACT
				n1->set(3141);

				// ASSERT
				assert_equal(plural + make_pair(0, 3141) + make_pair(1, 3141) + make_pair(2, 3141), log1);

				// INIT
				log1.clear();

				// ACT
				n2->set(314);

				// ASSERT
				assert_equal(plural + make_pair(3, 314), log1);

				// ACT
				n3->set(string("lorem"));

				// ASSERT
				assert_equal(plural + make_pair(0, string("lorem")) + make_pair(1, string("lorem")), log2);

				// INIT
				log2.clear();

				// ACT
				n4->set(string("Lorem Ipsum Amet Dolor"));

				// ASSERT
				assert_equal(plural + make_pair(2, string("Lorem Ipsum Amet Dolor")), log2);

				// ACT
				n5->set();

				// ASSERT
				assert_equal(2, log3);

				// ACT
				n6->set();

				// ASSERT
				assert_equal(3, log3);
		}


			test( RegisteringContinuationWhenResultIsSetCallsThemImmediately )
			{
				// INIT
				vector<int> log1;
				vector<string> log2;
				auto log3 = 0;
				auto n1 = make_shared< task_node<int> >();
				auto n2 = make_shared< task_node<int> >();
				auto n3 = make_shared< task_node<string> >();
				auto n4 = make_shared< task_node<void> >();

				// INIT / ACT
				n1->set(1211);
				n2->set(17);
				n3->set(string("Lorem Ipsum Amet"));
				n4->set();

				// ACT
				n1->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(*v);	}));
				n2->continue_with(make_continuation<int>([&] (const async_result<int> &v) {	log1.push_back(*v);	}));
				n3->continue_with(make_continuation<string>([&] (const async_result<string> &v) {	log2.push_back(*v);	}));
				n4->continue_with(make_continuation<void>([&] (const async_result<void> &v) {	*v, log3++;	}));

				// ASSERT
				assert_equal(plural + 1211 + 17, log1);
				assert_equal(plural + string("Lorem Ipsum Amet"), log2);
				assert_equal(1, log3);
			}


			test( SettingFailureIsPropogatedToCallbacks )
			{
				// INIT
				auto faults = 0;
				auto n1 = make_shared< task_node<void> >();
				auto n2 = make_shared< task_node<void> >();

				n1->continue_with(make_continuation<void>([&] (const async_result<void> &v) {
					try
					{	*v;	}
					catch (int e)
					{
						assert_equal(18131, e);
						faults++;
					}
				}));
				n2->continue_with(make_continuation<void>([&] (const async_result<void> &v) {
					try
					{	*v;	}
					catch (string e)
					{
						assert_equal("LoremIpsum", e);
						faults++;
					}
				}));

				// ACT
				n1->fail(make_exception(18131));

				// ASSERT
				assert_equal(1, faults);

				// ACT
				n2->fail(make_exception(string("LoremIpsum")));

				// ASSERT
				assert_equal(2, faults);

				// ACT / ASSERT
				n2->continue_with(make_continuation<void>([&] (const async_result<void> &v) {
					try
					{	*v;	}
					catch (string e)
					{
						assert_equal("LoremIpsum", e);
						faults++;
					}
				}));

				// ASSERT
				assert_equal(3, faults);
			}
		end_test_suite
	}
}
