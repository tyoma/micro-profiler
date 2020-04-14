#include <frontend/persistence.h>

#include "helpers.h"
#include "mocks.h"

#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<long_address_t> unthreaded_statistic_types;
			typedef pair<function_key, statistic_types::function_detailed> addressed_function;
		}

		begin_test_suite( FunctionListPersistenceTests )
			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;
			vector<unsigned int> dummy_context;
			
			function<void (unsigned persistent_id)> get_requestor()
			{	return [this] (unsigned /*persistent_id*/) { };	}

			function<void (const vector<unsigned> &)> get_requestor_threads()
			{	return [this] (const vector<unsigned> &) { };	}

			FunctionListPersistenceTests()
				: ser(_buffer), dser(_buffer)
			{	}

			init( CreatePrerequisites )
			{
				resolver.reset(new mocks::symbol_resolver);
				tmodel.reset(new mocks::threads_model);
			}


			test( SymbolsRequiredAndTicksPerSecondAreSerializedForFile )
			{
				// INIT
				pair<long_address_t, string> symbols1[] = {
					make_pair(1, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl1(functions_list::create(16, mocks::symbol_resolver::create(symbols1), tmodel));
				pair<long_address_t, string> symbols2[] = {
					make_pair(7, "A"), make_pair(11, "B"), make_pair(19, "C"), make_pair(131, "D"), make_pair(113, "E"),
				};
				shared_ptr<functions_list> fl2(functions_list::create(25000000000, mocks::symbol_resolver::create(symbols2), tmodel));
				unthreaded_statistic_types::map_detailed s;
				statistic_types::map_detailed read;
				timestamp_t ticks_per_second;
				threads_model dummy_threads(get_requestor_threads());

				s[1], s[17];
				serialize_single_threaded(ser, s);
				dser(*fl1, dummy_context);

				s.clear();
				s[7], s[11], s[131], s[113];
				serialize_single_threaded(ser, s);
				dser(*fl2, dummy_context);

				// ACT
				save(ser, *fl1);

				// ASSERT
				symbol_resolver r(get_requestor());

				dser(ticks_per_second);
				dser(r);
				dser(read);
				dser(dummy_threads);

				assert_equal(16, ticks_per_second);
				assert_equal("Lorem", r.symbol_name_by_va(1));
				assert_equal("Amet", r.symbol_name_by_va(17));

				// ACT
				save(ser, *fl2);

				// ASSERT
				dser(ticks_per_second);
				dser(r);
				dser(read);
				dser(dummy_threads);

				assert_equal(25000000000, ticks_per_second);
				assert_equal("A", r.symbol_name_by_va(7));
				assert_equal("B", r.symbol_name_by_va(11));
				assert_equal("D", r.symbol_name_by_va(131));
				assert_equal("E", r.symbol_name_by_va(113));
			}


			test( ThreadsModelIsSerializedForFile )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(1, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<mocks::threads_model> tmodel1(new mocks::threads_model());
				shared_ptr<mocks::threads_model> tmodel2(new mocks::threads_model());
				shared_ptr<functions_list> fl1(functions_list::create(16, mocks::symbol_resolver::create(symbols), tmodel1));
				shared_ptr<functions_list> fl2(functions_list::create(16, mocks::symbol_resolver::create(symbols), tmodel2));

				tmodel1->add(0, 1211, "thread A");
				tmodel1->add(1, 1212, "thread B");
				tmodel2->add(1, 111, "#1");
				tmodel2->add(21, 112, "#2");
				tmodel2->add(14, 113, "#3");

				// ACT
				save(ser, *fl1);

				// ASSERT
				long long dummy_frequency;
				symbol_resolver dummy_resolver(get_requestor());
				statistic_types::map_detailed dummy_data;
				threads_model threads1(get_requestor_threads());
				threads_model threads2(get_requestor_threads());
				unsigned int native_id;

				dser(dummy_frequency);
				dser(dummy_resolver);
				dser(dummy_data);
				dser(threads1);

				assert_equal(3u, threads1.get_count());
				assert_is_true(threads1.get_native_id(native_id, 0));
				assert_equal(1211u, native_id);
				assert_is_true(threads1.get_native_id(native_id, 1));
				assert_equal(1212u, native_id);

				// ACT
				save(ser, *fl2);

				// ASSERT
				dser(dummy_frequency);
				dser(dummy_resolver);
				dser(dummy_data);
				dser(threads2);

				assert_equal(4u, threads2.get_count());
				assert_is_true(threads2.get_native_id(native_id, 1));
				assert_equal(111u, native_id);
				assert_is_true(threads2.get_native_id(native_id, 21));
				assert_equal(112u, native_id);
				assert_is_true(threads2.get_native_id(native_id, 14));
				assert_equal(113u, native_id);
			}


			test( StatisticsFollowsTheSymbolsInFileSerialization )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(1, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl(functions_list::create(1, mocks::symbol_resolver::create(symbols), tmodel));
				unthreaded_statistic_types::map_detailed s;
				timestamp_t ticks_per_second;
				threads_model dummy_threads(get_requestor_threads());

				s[1], s[17], s[13];
				serialize_single_threaded(ser, s);
				dser(*fl, dummy_context);

				// ACT
				save(ser, *fl);

				// ASSERT
				symbol_resolver r(get_requestor());
				statistic_types::map_detailed stats_read;

				dser(ticks_per_second);
				dser(r);
				dser(stats_read);
				dser(dummy_threads);

				assert_equal(3u, stats_read.size());
			}


			test( FunctionListIsComletelyRestoredWithSymbols )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				addressed_function s[] = {
					make_statistics(addr(5), 123, 0, 1000, 0, 0),
					make_statistics(addr(13, 3), 12, 0, 0, 0, 0),
					make_statistics(addr(17, 5), 127, 0, 0, 0, 0),
					make_statistics(addr(123), 12000, 0, 250, 0, 0),
				};
				mocks::threads_model threads;

				threads.add(5, 17000, "abc");
				threads.add(3, 19001, "zee");

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), mkvector(s), threads);

				// ACT
				shared_ptr<functions_list> fl = load_functions_list(dser);
				fl->set_order(columns::name, true);

				// ASSERT
				columns::main ordering[] = {	columns::name, columns::threadid, columns::times_called, columns::inclusive,	};
				wstring reference[][4] = {
					{	L"Amet", L"17000", L"127", L"0s",	},
					{	L"Ipsum", L"19001", L"12", L"0s",	},
					{	L"Lorem", L"", L"123", L"2s",	},
					{	L"dolor", L"", L"12000", L"500ms",	},
				};

				assert_table_equivalent(ordering, reference, *fl);
			}

		end_test_suite
	}
}
