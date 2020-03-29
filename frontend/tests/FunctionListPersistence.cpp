#include <frontend/persistence.h>

#include "helpers.h"
#include "mocks.h"

#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
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
		}

		begin_test_suite( FunctionListPersistenceTests )
			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;

			function<void (unsigned persistent_id)> get_requestor()
			{	return [this] (unsigned /*persistent_id*/) { };	}

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

				s[1], s[17];
				serialize_single_threaded(ser, s);
				dser(*fl1);

				s.clear();
				s[7], s[11], s[131], s[113];
				serialize_single_threaded(ser, s);
				dser(*fl2);

				// ACT
				save(ser, *fl1);

				// ASSERT
				symbol_resolver r(get_requestor());

				dser(ticks_per_second);
				dser(r);
				dser(read);

				assert_equal(16, ticks_per_second);
				assert_equal("Lorem", r.symbol_name_by_va(1));
				assert_equal("Amet", r.symbol_name_by_va(17));

				// ACT
				save(ser, *fl2);

				// ASSERT
				dser(ticks_per_second);
				dser(r);
				dser(read);

				assert_equal(25000000000, ticks_per_second);
				assert_equal("A", r.symbol_name_by_va(7));
				assert_equal("B", r.symbol_name_by_va(11));
				assert_equal("D", r.symbol_name_by_va(131));
				assert_equal("E", r.symbol_name_by_va(113));
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

				s[1], s[17], s[13];
				serialize_single_threaded(ser, s);
				dser(*fl);

				// ACT
				save(ser, *fl);

				// ASSERT
				symbol_resolver r(get_requestor());
				statistic_types::map_detailed stats_read;

				dser(ticks_per_second);
				dser(r);
				dser(stats_read);

				assert_equal(3u, stats_read.size());
			}


			test( FunctionListIsComletelyRestoredWithSymbols )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 123, s[addr(17)].times_called = 127, s[addr(13)].times_called = 12, s[addr(123)].times_called = 12000;
				s[addr(5)].inclusive_time = 1000, s[addr(123)].inclusive_time = 250;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);

				// ACT
				shared_ptr<functions_list> fl = load_functions_list(dser);
				fl->set_order(columns::name, true);

				// ASSERT
				columns::main ordering[] = {	columns::name, columns::times_called, columns::inclusive,	};
				wstring reference[][3] = {
					{	L"Amet", L"127", L"0s",	},
					{	L"Ipsum", L"12", L"0s",	},
					{	L"Lorem", L"123", L"2s",	},
					{	L"dolor", L"12000", L"500ms",	},
				};

				assert_table_equivalent(ordering, reference, *fl);
			}

		end_test_suite
	}
}
