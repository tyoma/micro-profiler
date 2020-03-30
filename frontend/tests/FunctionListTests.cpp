#include <frontend/function_list.h>

#include <frontend/serialization.h>

#include "helpers.h"
#include "mocks.h"

#include <iomanip>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <sstream>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace tests
	{
		namespace 
		{
			typedef statistic_types_t<long_address_t> unthreaded_statistic_types;
			typedef pair<unsigned, statistic_types_t<unsigned>::function_detailed> unthreaded_addressed_function;

			const columns::main name_times_inc_exc_iavg_eavg_reent_minc[] = {
				columns::name, columns::times_called, columns::inclusive, columns::exclusive,
				columns::inclusive_avg, columns::exclusive_avg, columns::max_reentrance, columns::max_time
			};

			const columns::main name_times[] = {	columns::name, columns::times_called,	};

			const columns::main name_threadid[] = {	columns::name, columns::threadid,	};

			timestamp_t test_ticks_per_second = 1;
			const double c_tolerance = 0.000001;

			template <typename T>
			wstring to_string(const T &value)
			{
				wstringstream s;
				s << value;
				return s.str();
			}

			void increment(int *value)
			{	++*value;	}

			class invalidation_tracer
			{
				wpl::slot_connection _connection;

				void on_invalidate(table_model::index_type count)
				{	invalidations.push_back(count);	}

			public:
				typedef vector<table_model::index_type> _invalidations_log_t;

			public:
				void bind_to_model(table_model &to)
				{	_connection = to.invalidated += bind(&invalidation_tracer::on_invalidate, this, _1);	}

				_invalidations_log_t invalidations;
			};


			class invalidation_at_sorting_check1
			{
				const table_model &_model;

				const invalidation_at_sorting_check1 &operator =(const invalidation_at_sorting_check1 &);

			public:
				invalidation_at_sorting_check1(table_model &m)
					: _model(m)
				{	}

				void operator ()(table_model::index_type /*count*/) const
				{
					wstring reference[][2] = {
						{	L"00002995", L"700",	},
						{	L"00003001", L"30",	},
						{	L"00002978", L"3",	},
					};

					assert_table_equal(name_times, reference, _model);
				}
			};


			// convert decimal point to current(default) locale
			string dp2cl(const char *input, char stub_char = '.')
			{
				static char decimal_point = use_facet< numpunct<char> > (locale("")).decimal_point();

				string result = input;
				replace(result.begin(), result.end(), stub_char, decimal_point);
				return result;
			}

			table_model::index_type find_row(const table_model &m, const wstring &name)
			{
				wstring result;

				for (table_model::index_type i = 0, c = m.get_count(); i != c; ++i)
					if (m.get_text(i, columns::name, result), result == name)
						return i;
				return table_model::npos();
			}
		}


		begin_test_suite( FunctionListTests )

			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;
			vector<unsigned int> dummy_context;

			function<void (unsigned persistent_id)> get_requestor()
			{	return [this] (unsigned /*persistent_id*/) { };	}

			FunctionListTests()
				: ser(_buffer), dser(_buffer)
			{	}

			init( CreatePrerequisites )
			{
				resolver.reset(new mocks::symbol_resolver);
				tmodel.reset(new mocks::threads_model);
			}


			test( CanCreateEmptyFunctionList )
			{
				// INIT / ACT
				shared_ptr<functions_list> sp_fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				const functions_list &fl = *sp_fl;

				// ACT / ASSERT
				assert_equal(0u, fl.get_count());
				assert_equal(resolver, fl.get_resolver());
				assert_is_true(fl.updates_enabled);
			}


			test( FunctionListAcceptsUpdates )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2234]) = function_statistics(10, 3, 7, 5);
				serialize_single_threaded(ser, s);

				// ACT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(2u, fl->get_count());
			}


			test( FunctionListDoesNotAcceptUpdatesIfNotEnabled )
			{
				// INIT
				int invalidated_count = 0;
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				wpl::slot_connection conn = fl->invalidated += bind(&increment, &invalidated_count);

				static_cast<function_statistics &>(s[1123]) = function_statistics(1, 0, 30, 20);
				serialize_single_threaded(ser, s);

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2234]) = function_statistics(10, 3, 7, 5);
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);
				invalidated_count = 0;

				// ACT
				fl->updates_enabled = false;
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(L"1", get_text(*fl, 0, 3));
				assert_equal(0, invalidated_count);
			}


			test( FunctionListCanBeClearedAndUsedAgain )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				serialize_single_threaded(ser, s);

				// ACT
				invalidation_tracer ih;
				ih.bind_to_model(*fl);
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(1u, ih.invalidations.size());
				assert_equal(1u, ih.invalidations.back()); //check what's coming as event arg

				// ACT
				shared_ptr<const trackable> first = fl->track(0); // 2229

				// ASSERT
				assert_equal(0u, first->index());

				// ACT
				fl->clear();

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal(2u, ih.invalidations.size());
				assert_equal(0u, ih.invalidations.back()); //check what's coming as event arg
				assert_equal(table_model::npos(), first->index());

				// INIT
				_buffer.rewind();

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(3u, ih.invalidations.size());
				assert_equal(1u, ih.invalidations.back()); //check what's coming as event arg
				assert_equal(0u, first->index()); // kind of side effect
			}


			test( ClearingTheListResetsWatchedChildren )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer it1, it2;

				s[1123].callees[11000];
				s[1123].callees[11001];
				s[1124].callees[11100];
				serialize_single_threaded(ser, s);
				dser(*fl, dummy_context);

				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> children[] = {	fl->watch_children(0), fl->watch_children(1),	};

				it1.bind_to_model(*children[0]);
				it2.bind_to_model(*children[1]);

				// ACT
				fl->clear();

				// ASSERT
				table_model::index_type reference[] = { 0, };

				assert_equal(0u, children[0]->get_count());
				assert_equal(reference, it1.invalidations);
				assert_equal(0u, children[1]->get_count());
				assert_equal(reference, it2.invalidations);
			}


			test( ClearingTheListResetsWatchedParents )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer it1, it2;

				s[1123].callees[11000];
				s[1123].callees[11001];
				s[1124].callees[11000];
				serialize_single_threaded(ser, s);
				dser(*fl, dummy_context);

				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> parents[] = {	fl->watch_parents(2), fl->watch_parents(2),	};

				it1.bind_to_model(*parents[0]);
				it2.bind_to_model(*parents[1]);

				// ACT
				fl->clear();

				// ASSERT
				table_model::index_type reference[] = { 0, };

				assert_equal(0u, parents[0]->get_count());
				assert_equal(reference, it1.invalidations);
				assert_equal(0u, parents[1]->get_count());
				assert_equal(reference, it2.invalidations);
			}


			test( FunctionListGetByAddress )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				vector<functions_list::index_type> expected;

				static_cast<function_statistics &>(s[1118]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2229]) = function_statistics(10, 3, 7, 5);
				static_cast<function_statistics &>(s[5550]) = function_statistics(5, 0, 10, 7);
				serialize_single_threaded(ser, s);
				
				// ACT
				dser(*fl, dummy_context);

				functions_list::index_type idx1118 = fl->get_index(addr(1118));
				functions_list::index_type idx2229 = fl->get_index(addr(2229));
				functions_list::index_type idx5550 = fl->get_index(addr(5550));

				expected.push_back(idx1118);
				expected.push_back(idx2229);
				expected.push_back(idx5550);
				sort(expected.begin(), expected.end());
				
				// ASSERT
				assert_equal(3u, fl->get_count());

				for (table_model::index_type i = 0; i < expected.size(); ++i)
					assert_equal(expected[i], i);

				assert_not_equal(table_model::npos(), idx1118);
				assert_not_equal(table_model::npos(), idx2229);
				assert_not_equal(table_model::npos(), idx5550);
				assert_equal(table_model::npos(), fl->get_index(addr(1234)));

				//Check twice. Kind of regularity check.
				assert_equal(fl->get_index(addr(1118)), idx1118);
				assert_equal(fl->get_index(addr(2229)), idx2229);
				assert_equal(fl->get_index(addr(5550)), idx5550);
			}


			test( FunctionListCollectsUpdates )
			{
				//TODO: add 2 entries of same function in one burst
				//TODO: possibly trackable on update tests should see that it works with every sorting given.

				// INIT
				unthreaded_statistic_types::map_detailed s1, s2, s3;

				static_cast<function_statistics &>(s1[1118]) = function_statistics(19, 0, 31, 29, 3);
				static_cast<function_statistics &>(s1[2229]) = function_statistics(10, 3, 7, 5, 4);
				static_cast<function_statistics &>(s2[1118]) = function_statistics(5, 0, 10, 7, 6);
				static_cast<function_statistics &>(s2[5550]) = function_statistics(15, 1024, 1011, 723, 215);
				static_cast<function_statistics &>(s3[1118]) = function_statistics(100111222333, 0, 17000, 14000, 4);

				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				fl->set_order(columns::times_called, true);
				
				invalidation_tracer ih;
				ih.bind_to_model(*fl);

				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);
				serialize_single_threaded(ser, s3);

				// ACT
				dser(*fl, dummy_context);
				shared_ptr<const trackable> first = fl->track(0); // 2229
				shared_ptr<const trackable> second = fl->track(1); // 1118

				// ASSERT
				assert_equal(1u, ih.invalidations.size());
				assert_equal(2u, ih.invalidations.back()); //check what's coming as event arg
		
				wstring reference1[][8] = {
					{	L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0", L"3s"	},
					{	L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s"	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *fl);

				assert_equal(0u, first->index());
				assert_equal(1u, second->index());

				// ACT
				dser(*fl, dummy_context);
				
				// ASSERT
				assert_equal(2u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				wstring reference2[][8] = {
					{	L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0", L"6s",	},
					{	L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s",	},
					{	L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s",	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *fl);

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // kind of moved down

				// ACT
				dser(*fl, dummy_context);
				
				// ASSERT
				assert_equal(3u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				wstring reference3[][8] = {
					{	L"0000045E", L"100111222357", L"1.7e+04s", L"1.4e+04s", L"170ns", L"140ns", L"0", L"6s",	},
					{	L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s",	},
					{	L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s",	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference3, *fl);

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // stand still
			}


			test( FunctionListTimeFormatter )
			{
				// INIT
				unthreaded_addressed_function functions[] = {
					// ~ ns
					make_statistics(1118u, 1, 0, 31, 29, 29),
					make_statistics(1990u, 1, 0, 9994, 9994, 9994),

					// >= 1us
					make_statistics(2000u, 1, 0, 9996, 9996, 9996),
					make_statistics(2229u, 1, 0, 45340, 36666, 36666),
					make_statistics(2990u, 1, 0, 9994000, 9994000, 9994000),

					// >= 1ms
					make_statistics(3000u, 1, 0, 9996000, 9996000, 9996000),
					make_statistics(3118u, 1, 0, 33450030, 32333333, 32333333),
					make_statistics(3990u, 1, 0, 9994000000, 9994000000, 9994000000),

					// >= 1s
					make_statistics(4000u, 1, 0, 9996000000, 9996000000, 9996000000),
					make_statistics(5550u, 1, 0, 65450031030, 23470030000, 23470030000),
					make_statistics(4990u, 1, 0, 9994000000000, 9994000000000, 9994000000000),

					// >= 1000s
					make_statistics(5000u, 1, 0, 9996000000000, 9996000000000, 9996000000000),
					make_statistics(4550u, 1, 0, 65450031030567, 23470030000987, 23470030000987),
					make_statistics(5990u, 1, 0, 99990031030567, 99990030000987, 99990030000987),
				
					// >= 10000s
					make_statistics(6000u, 1, 0, 99999031030567, 99999030000987, 99999030000987),
					make_statistics(6661u, 1, 0, 65450031030567000, 23470030000987000, 23470030000987000),
				};
				shared_ptr<functions_list> fl(functions_list::create(10000000000, resolver, tmodel)); // 10 * billion ticks per second

				serialize_single_threaded(ser, mkvector(functions));

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				wstring reference[][8] = {
					{	L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0", L"2.9ns",	},
					{	L"000008B5", L"1", L"4.53\x03bcs", L"3.67\x03bcs", L"4.53\x03bcs", L"3.67\x03bcs", L"0", L"3.67\x03bcs",	},
					{	L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0", L"3.23ms",	},
					{	L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0", L"2.35s",	},
					{	L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0", L"2347s",	},
					{	L"00001A05", L"1", L"6.55e+06s", L"2.35e+06s", L"6.55e+06s", L"2.35e+06s", L"0", L"2.35e+06s",	},

					// boundary cases
					{	L"000007C6", L"1", L"999ns", L"999ns", L"999ns", L"999ns", L"0", L"999ns",	},
					{	L"000007D0", L"1", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"0", L"1\x03bcs",	},
					{	L"00000BAE", L"1", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"0", L"999\x03bcs",	},
					{	L"00000BB8", L"1", L"1ms", L"1ms", L"1ms", L"1ms", L"0", L"1ms",	},
					{	L"00000F96", L"1", L"999ms", L"999ms", L"999ms", L"999ms", L"0", L"999ms",	},
					{	L"00000FA0", L"1", L"1s", L"1s", L"1s", L"1s", L"0", L"1s",	},
					{	L"0000137E", L"1", L"999s", L"999s", L"999s", L"999s", L"0", L"999s",	},
					{	L"00001388", L"1", L"999.6s", L"999.6s", L"999.6s", L"999.6s", L"0", L"999.6s",	},
					{	L"00001766", L"1", L"9999s", L"9999s", L"9999s", L"9999s", L"0", L"9999s",	},
					{	L"00001770", L"1", L"1e+04s", L"1e+04s", L"1e+04s", L"1e+04s", L"0", L"1e+04s",	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference, *fl);
			}


			test( FunctionListSorting )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer ih;
				const size_t data_size = 4;
				unthreaded_addressed_function functions[data_size] = {
					make_statistics(1990u, 15, 0, 31, 29, 3),
					make_statistics(2000u, 35, 1, 453, 366, 4),
					make_statistics(2990u, 2, 2, 33450030, 32333333, 5),
					make_statistics(3000u, 15233, 3, 65460, 13470, 6),
				};

				serialize_single_threaded(ser, mkvector(functions));
				ih.bind_to_model(*fl);
				dser(*fl, dummy_context);

				shared_ptr<const trackable> pt0 = fl->track(fl->get_index(addr(1990)));
				shared_ptr<const trackable> pt1 = fl->track(fl->get_index(addr(2000)));
				shared_ptr<const trackable> pt2 = fl->track(fl->get_index(addr(2990)));
				shared_ptr<const trackable> pt3 = fl->track(fl->get_index(addr(3000)));

				const trackable &t0 = *pt0;
				const trackable &t1 = *pt1;
				const trackable &t2 = *pt2;
				const trackable &t3 = *pt3;

				// ACT (times called, ascending)
				fl->set_order(columns::times_called, true);
				
				// ASSERT
				assert_equal(2u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference1[][8] = {
					{	L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s",	},
					{	L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s",	},
					{	L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s",	},
					{	L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *fl);

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (times called, descending)
				fl->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(3u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference2[][8] = {
					{	L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"	},
					{	L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"	},
					{	L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"	},
					{	L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *fl);

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (name, ascending; after times called to see that sorting in asc direction works)
				fl->set_order(columns::name, true);

				// ASSERT
				assert_equal(4u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference3[][8] = {
					{	L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"	},
					{	L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"	},
					{	L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"	},
					{	L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference3, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (name, descending)
				fl->set_order(columns::name, false);

				// ASSERT
				assert_equal(5u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference4[][8] = {
					{	L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"	},
					{	L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"	},
					{	L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"	},
					{	L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference4, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (exclusive time, ascending)
				fl->set_order(columns::exclusive, true);

				// ASSERT
				assert_equal(6u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference5[][2] = {
					{	L"000007C6", L"15",	},
					{	L"000007D0", L"35",	},
					{	L"00000BB8", L"15233",	},
					{	L"00000BAE", L"2",	},
				};

				assert_table_equal(name_times, reference5, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (exclusive time, descending)
				fl->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(7u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference6[][2] = {
					{	L"00000BAE", L"2",	},
					{	L"00000BB8", L"15233",	},
					{	L"000007D0", L"35",	},
					{	L"000007C6", L"15",	},
				};

				assert_table_equal(name_times, reference6, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (inclusive time, ascending)
				fl->set_order(columns::inclusive, true);

				// ASSERT
				assert_equal(8u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference7[][2] = {
					{	L"000007C6", L"15",	},
					{	L"000007D0", L"35",	},
					{	L"00000BB8", L"15233",	},
					{	L"00000BAE", L"2",	},
				};

				assert_table_equal(name_times, reference7, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (inclusive time, descending)
				fl->set_order(columns::inclusive, false);

				// ASSERT
				assert_equal(9u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference8[][2] = {
					{	L"00000BAE", L"2",	},
					{	L"00000BB8", L"15233",	},
					{	L"000007D0", L"35",	},
					{	L"000007C6", L"15",	},
				};

				assert_table_equal(name_times, reference8, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());
				
				// ACT (avg. exclusive time, ascending)
				fl->set_order(columns::exclusive_avg, true);
				
				// ASSERT
				assert_equal(10u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference9[][2] = {
					{	L"00000BB8", L"15233",	},
					{	L"000007C6", L"15",	},
					{	L"000007D0", L"35",	},
					{	L"00000BAE", L"2",	},
				};

				assert_table_equal(name_times, reference9, *fl);

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (avg. exclusive time, descending)
				fl->set_order(columns::exclusive_avg, false);
				
				// ASSERT
				assert_equal(11u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference10[][2] = {
					{	L"00000BAE", L"2",	},
					{	L"000007D0", L"35",	},
					{	L"000007C6", L"15",	},
					{	L"00000BB8", L"15233",	},
				};

				assert_table_equal(name_times, reference10, *fl);

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (avg. inclusive time, ascending)
				fl->set_order(columns::inclusive_avg, true);
				
				// ASSERT
				assert_equal(12u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference11[][2] = {
					{	L"000007C6", L"15",	},
					{	L"00000BB8", L"15233",	},
					{	L"000007D0", L"35",	},
					{	L"00000BAE", L"2",	},
				};

				assert_table_equal(name_times, reference11, *fl);

				assert_equal(0u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (avg. inclusive time, descending)
				fl->set_order(columns::inclusive_avg, false);
				
				// ASSERT
				assert_equal(13u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference12[][2] = {
					{	L"00000BAE", L"2",	},
					{	L"000007D0", L"35",	},
					{	L"00000BB8", L"15233",	},
					{	L"000007C6", L"15",	},
				};

				assert_table_equal(name_times, reference12, *fl);

				assert_equal(3u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (max reentrance, ascending)
				fl->set_order(columns::max_reentrance, true);
				
				// ASSERT
				assert_equal(14u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference13[][2] = {
					{	L"000007C6", L"15",	},
					{	L"000007D0", L"35",	},
					{	L"00000BAE", L"2",	},
					{	L"00000BB8", L"15233",	},
				};

				assert_table_equal(name_times, reference13, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max reentrance, descending)
				fl->set_order(columns::max_reentrance, false);
				
				// ASSERT
				assert_equal(15u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference14[][2] = {
					{	L"00000BB8", L"15233",	},
					{	L"00000BAE", L"2",	},
					{	L"000007D0", L"35",	},
					{	L"000007C6", L"15",	},
				};

				assert_table_equal(name_times, reference14, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (max call time, ascending)
				fl->set_order(columns::max_time, true);
				
				// ASSERT
				assert_equal(16u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference15[][2] = {
					{	L"000007C6", L"15",	},
					{	L"000007D0", L"35",	},
					{	L"00000BAE", L"2",	},
					{	L"00000BB8", L"15233",	},
				};

				assert_table_equal(name_times, reference15, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max call time, descending)
				fl->set_order(columns::max_time, false);
				
				// ASSERT
				assert_equal(17u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				wstring reference16[][2] = {
					{	L"00000BB8", L"15233",	},
					{	L"00000BAE", L"2",	},
					{	L"000007D0", L"35",	},
					{	L"000007C6", L"15",	},
				};

				assert_table_equal(name_times, reference16, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());
			}


			test( FunctionListTakesNativeIDFromThreadModel )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_addressed_function functions[][1] = {
					{ make_statistics(0x1000u, 1, 0, 0, 0, 0), },
					{ make_statistics(0x1010u, 1, 0, 0, 0, 0), },
					{ make_statistics(0x1020u, 1, 0, 0, 0, 0), },
					{ make_statistics(0x1030u, 1, 0, 0, 0, 0), },
				};
				pair< unsigned, vector<unthreaded_addressed_function> > data[] = {
					make_pair(3, mkvector(functions[0])),
					make_pair(2, mkvector(functions[1])),
					make_pair(7, mkvector(functions[2])),
					make_pair(9, mkvector(functions[3])),
				};

				ser(mkvector(data));
				dser(*fl, dummy_context);

				tmodel->add(3, 100, string());
				tmodel->add(2, 1000, string());
				tmodel->add(7, 900, string());

				fl->set_order(columns::name, true);

				// ACT / ASSERT
				wstring reference1[][2] = {
					{	L"00001000", L"100",	},
					{	L"00001010", L"1000",	},
					{	L"00001020", L"900",	},
					{	L"00001030", L"",	},
				};

				assert_table_equivalent(name_threadid, reference1, *fl);

				// INIT
				tmodel->add(9, 90, string());

				// ACT / ASSERT
				wstring reference2[][2] = {
					{	L"00001000", L"100",	},
					{	L"00001010", L"1000",	},
					{	L"00001020", L"900",	},
					{	L"00001030", L"90",	},
				};

				assert_table_equivalent(name_threadid, reference2, *fl);
			}


			test( FunctionListCanBeSortedByThreadID )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer ih;
				const size_t data_size = 5;
				unthreaded_addressed_function functions[data_size][1] = {
					{	make_statistics(10000u, 1, 0, 1, 1, 1),	},
					{	make_statistics(10000u, 2, 0, 1, 1, 1),	},
					{	make_statistics(10000u, 3, 0, 1, 1, 1),	},
					{	make_statistics(10000u, 4, 0, 1, 1, 1),	},
					{	make_statistics(10000u, 5, 0, 1, 1, 1),	},
				};
				pair< unsigned, vector<unthreaded_addressed_function> > data[] = {
					make_pair(0, mkvector(functions[0])),
					make_pair(1, mkvector(functions[1])),
					make_pair(2, mkvector(functions[2])),
					make_pair(3, mkvector(functions[3])),
					make_pair(4, mkvector(functions[4])),
				};

				tmodel->add(0, 18, string());
				tmodel->add(1, 1, string());
				tmodel->add(2, 180, string());
				tmodel->add(3, 179, string());
				tmodel->add(4, 17900, string());

				ser(mkvector(data));

				ih.bind_to_model(*fl);
				dser(*fl, dummy_context);
				ih.invalidations.clear();

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, true);

				// ASSERT
				columns::main ordering[] = {	columns::times_called,	};
				size_t reference_updates1[] = {	data_size,	};
				wstring reference1[][1] = {	{	L"2",	}, {	L"1",	}, {	L"4",	}, {	L"3",	}, {	L"5",	},	};

				assert_table_equal(ordering, reference1, *fl);
				assert_equal(reference_updates1, ih.invalidations);

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, false);

				// ASSERT
				size_t reference_updates2[] = {	data_size, data_size,	};
				wstring reference2[][1] = {	{	L"5",	}, {	L"3",	},{	L"4",	},{	L"1",	},  {	L"2",	}, 	};

				assert_table_equal(ordering, reference2, *fl);
				assert_equal(reference_updates2, ih.invalidations);
			}


			test( FunctionListPrintItsContent )
			{
				// INIT
				unthreaded_statistic_types::map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				string result;

				static_cast<function_statistics &>(s[1990]) = function_statistics(15, 0, 31, 29, 2);
				static_cast<function_statistics &>(s[2000]) = function_statistics(35, 1, 453, 366, 3);
				static_cast<function_statistics &>(s[2990]) = function_statistics(2, 2, 33450030, 32333333, 4);
				serialize_single_threaded(ser, s);

				// ACT
				fl->print(result);

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
					"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n", result);

				// ACT
				dser(*fl, dummy_context);
				fl->set_order(columns::times_called, true);
				fl->print(result);

				// ASSERT
				assert_equal(s.size(), fl->get_count());
				assert_equal(dp2cl("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										"00000BAE\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t2\t4\r\n"
										"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"), result);

				// ACT
				fl->set_order(columns::exclusive_avg, true);
				fl->print(result);

				// ASSERT
				assert_equal(s.size(), fl->get_count());
				assert_equal(dp2cl("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"
										"00000BAE\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t2\t4\r\n"), result);
			}


			test( FailOnGettingChildrenListFromNonEmptyRootList )
			{
				// INIT
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver, tmodel));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[1978];
				s1[1995];
				s2[2001];
				s2[2004];
				s2[2011];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl1, dummy_context);
				dser(*fl2, dummy_context);

				// ACT / ASSERT
				assert_throws(fl1->watch_children(2), out_of_range);
				assert_throws(fl1->watch_children(20), out_of_range);
				assert_throws(fl1->watch_children(table_model::npos()), out_of_range);
				assert_throws(fl2->watch_children(3), out_of_range);
				assert_throws(fl2->watch_children(30), out_of_range);
				assert_throws(fl2->watch_children(table_model::npos()), out_of_range);
			}


			test( ReturnChildrenModelForAValidRecord )
			{
				// INIT
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver, tmodel));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[1978];
				s1[1995];
				s2[2001];
				s2[2004];
				s2[2011];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl1, dummy_context);
				dser(*fl2, dummy_context);

				// ACT / ASSERT
				assert_not_null(fl1->watch_children(0));
				assert_not_null(fl1->watch_children(1));
				assert_not_null(fl2->watch_children(0));
				assert_not_null(fl2->watch_children(1));
				assert_not_null(fl2->watch_children(2));
			}


			test( LinkedStatisticsObjectIsReturnedForChildren )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[1973];
				s[1990];
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);

				// ACT
				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				// ASSERT
				assert_not_null(ls);
				assert_equal(0u, ls->get_count());
			}


			test( ChildrenStatisticsForNonEmptyChildren )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x1978].callees[0x2001];
				s[0x1995].callees[0x2004];
				s[0x1995].callees[0x2008];
				s[0x1995].callees[0x2011];
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);
				
				// ACT
				shared_ptr<linked_statistics> ls_0 = fl->watch_children(find_row(*fl, L"00001978"));
				shared_ptr<linked_statistics> ls_1 = fl->watch_children(find_row(*fl, L"00001995"));

				// ACT / ASSERT
				assert_equal(1u, ls_0->get_count());
				assert_not_equal(table_model::npos(), find_row(*ls_0, L"00002001"));
				assert_equal(3u, ls_1->get_count());
				assert_not_equal(table_model::npos(), find_row(*ls_1, L"00002004"));
				assert_not_equal(table_model::npos(), find_row(*ls_1, L"00002008"));
				assert_not_equal(table_model::npos(), find_row(*ls_1, L"00002011"));
			}


			test( ChildrenStatisticsSorting )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[0x1978].callees[0x2001] = function_statistics(11);
				s2[0x1978].callees[0x2001] = function_statistics(11);
				s2[0x1978].callees[0x2004] = function_statistics(17);
				s2[0x1978].callees[0x2008] = function_statistics(18);
				s2[0x1978].callees[0x2011] = function_statistics(29);
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl, dummy_context);
				dser(*fl, dummy_context);
				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				// ACT
				ls->set_order(columns::name, false);

				// ASSERT
				wstring reference1[][8] = {
					{	L"00002011", L"29", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002008", L"18", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002004", L"17", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002001", L"22", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *ls);

				// ACT
				ls->set_order(columns::times_called, true);

				// ASSERT
				wstring reference2[][8] = {
					{	L"00002004", L"17", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002008", L"18", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002001", L"22", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
					{	L"00002011", L"29", L"0s", L"0s", L"0s", L"0s", L"0", L"0s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *ls);
			}


			test( ChildrenStatisticsGetText )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(10, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);
				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);
				ls->set_order(columns::name, true);

				// ACT / ASSERT
				wstring reference[][8] = {
					{	L"00002001", L"11", L"100ms", L"700ms", L"9.09ms", L"63.6ms", L"0", L"9.1s",	},
					{	L"00002004", L"17", L"200ms", L"800ms", L"11.8ms", L"47.1ms", L"5", L"9.7s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference, *ls);
			}


			test( IncomingDetailStatisticsUpdateNoLinkedStatisticsAfterClear )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer t1, t2;
				unthreaded_statistic_types::map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);
				s[0x1978].callees.clear();
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> ls1 = fl->watch_children(0);
				shared_ptr<linked_statistics> ls2 = fl->watch_parents(0);

				fl->clear();
				t1.bind_to_model(*ls1);
				t2.bind_to_model(*ls2);

				// ACT (linked statistics are detached)
				dser(*fl, dummy_context);

				// ASSERT
				assert_is_empty(t1.invalidations);
				assert_is_empty(t2.invalidations);
			}


			test( LinkedStatisticsInvalidatedToEmptyOnMasterDestruction )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer t1, t2;
				unthreaded_statistic_types::map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> ls1 = fl->watch_children(0);
				shared_ptr<linked_statistics> ls2 = fl->watch_parents(0);

				t1.bind_to_model(*ls1);
				t2.bind_to_model(*ls2);

				// ACT
				fl.reset();

				// ASSERT
				table_model::index_type reference[] = { 0u, };

				assert_equal(reference, t1.invalidations);
				assert_equal(reference, t2.invalidations);
			}


			test( IncomingDetailStatisticsUpdatenoChildrenStatisticsUpdatesScenarios )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer t;
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s1[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				s2[0x1978].callees[0x2004] = function_statistics(11, 0, 1, 7, 91);
				s2[0x1978].callees[0x2007] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl, dummy_context);
				_buffer.rewind();
				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				t.bind_to_model(*ls);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, t.invalidations.size());
				assert_equal(2u, t.invalidations.back());

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(2u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());

				// INIT
				unthreaded_statistic_types::map_detailed s3;

				s3[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				serialize_single_threaded(ser, s3);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(3u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());
			}


			test( GetFunctionAddressFromLinkedChildrenStatistics )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11);
				s[0x1978].callees[0x2004] = function_statistics(17);
				s[0x1978].callees[0x2008] = function_statistics(18);
				s[0x1978].callees[0x2011] = function_statistics(29);
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);
				fl->set_order(columns::name, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				ls->set_order(columns::name, true);

				// ACT / ASSERT
				assert_equal(addr(0x2001u), ls->get_key(0));
				assert_equal(addr(0x2004u), ls->get_key(1));
				assert_equal(addr(0x2008u), ls->get_key(2));
				assert_equal(addr(0x2011u), ls->get_key(3));
			}


			test( TrackableIsUsableOnReleasingModel )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				static_cast<function_statistics &>(s[0x2001]) = function_statistics(11);
				static_cast<function_statistics &>(s[0x2004]) = function_statistics(17);
				static_cast<function_statistics &>(s[0x2008]) = function_statistics(18);
				serialize_single_threaded(ser, s);

				dser(*fl, dummy_context);

				// ACT
				shared_ptr<const trackable> t(fl->track(1));

				fl = shared_ptr<functions_list>();

				// ACT / ASSERT
				assert_equal(trackable::npos(), t->index());
			}


			test( FailOnGettingParentsListFromNonEmptyRootList )
			{
				// INIT
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver, tmodel));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[0x1978];
				s1[0x1995];
				serialize_single_threaded(ser, s1);
				s2[0x2001];
				s2[0x2004];
				s2[0x2008];
				serialize_single_threaded(ser, s2);

				dser(*fl1, dummy_context);
				dser(*fl2, dummy_context);

				// ACT / ASSERT
				assert_throws(fl1->watch_parents(2), out_of_range);
				assert_throws(fl1->watch_parents(20), out_of_range);
				assert_throws(fl1->watch_parents(table_model::npos()), out_of_range);
				assert_throws(fl2->watch_parents(3), out_of_range);
				assert_throws(fl2->watch_parents(30), out_of_range);
				assert_throws(fl2->watch_parents(table_model::npos()), out_of_range);
			}


			test( ReturnParentsModelForAValidRecord )
			{
				// INIT
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver, tmodel));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[0x1978];
				s1[0x1995];
				serialize_single_threaded(ser, s1);
				s2[0x2001];
				s2[0x2004];
				s2[0x2008];
				serialize_single_threaded(ser, s2);

				dser(*fl1, dummy_context);
				dser(*fl2, dummy_context);

				// ACT / ASSERT
				assert_not_null(fl1->watch_parents(0));
				assert_not_null(fl1->watch_parents(1));
				assert_not_null(fl2->watch_parents(0));
				assert_not_null(fl2->watch_parents(1));
				assert_not_null(fl2->watch_parents(2));
			}


			test( SizeOfParentsListIsReturnedFromParentsModel )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[2978].callees[3001];
				s[2995].callees[3001];
				s[3001].callees[2995];
				s[3001].callees[3001];
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				// ACT
				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p0 = fl->watch_parents(0);
				shared_ptr<linked_statistics> p1 = fl->watch_parents(1);
				shared_ptr<linked_statistics> p2 = fl->watch_parents(2);

				// ASSERT
				assert_equal(0u, p0->get_count());
				assert_equal(1u, p1->get_count());
				assert_equal(3u, p2->get_count());
			}


			test( ParentStatisticsIsUpdatedOnGlobalUpdates1 )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[2978].callees[2978];
				s1[2995].callees[2978];
				s1[3001].callees[2978];
				s2[3002].callees[2978];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				fl->set_order(columns::name, true);

				// ACT
				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(0);

				// ASSERT
				assert_equal(3u, p->get_count());

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(4u, p->get_count());
			}


			test( ParentStatisticsValuesAreFormatted )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				static_cast<function_statistics &>(s[0x122F]) = function_statistics(1);
				s[0x122F].callees[0x2340] = function_statistics(3);
				static_cast<function_statistics &>(s[0x2340]) = function_statistics(3);
				s[0x2340].callees[0x3451] = function_statistics(5000000000);
				static_cast<function_statistics &>(s[0x3451]) = function_statistics(5000000000);
				s[0x3451].callees[0x122F] = function_statistics(1);
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p0 = fl->watch_parents(0);
				shared_ptr<linked_statistics> p1 = fl->watch_parents(1);
				shared_ptr<linked_statistics> p2 = fl->watch_parents(2);

				// ACT / ASSERT
				wstring reference1[][2] = {	{	L"00003451", L"1",	},	};
				wstring reference2[][2] = {	{	L"0000122F", L"3",	},	};
				wstring reference3[][2] = {	{	L"00002340", L"5000000000",	},	};

				assert_table_equal(name_times, reference1, *p0);
				assert_table_equal(name_times, reference2, *p1);
				assert_table_equal(name_times, reference3, *p2);
			}


			test( ParentStatisticsSorting )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(700);
				s[0x3001].callees[0x3001] = function_statistics(30);
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				// ACT
				p->set_order(columns::name, true);

				// ASSERT
				wstring reference1[][2] = {
					{	L"00002978", L"3",	},
					{	L"00002995", L"700",	},
					{	L"00003001", L"30",	},
				};

				assert_table_equal(name_times, reference1, *p);

				// ACT
				p->set_order(columns::name, false);

				// ASSERT
				wstring reference2[][2] = {
					{	L"00003001", L"30",	},
					{	L"00002995", L"700",	},
					{	L"00002978", L"3",	},
				};

				assert_table_equal(name_times, reference2, *p);

				// ACT
				p->set_order(columns::times_called, true);

				// ASSERT
				wstring reference3[][2] = {
					{	L"00002978", L"3",	},
					{	L"00003001", L"30",	},
					{	L"00002995", L"700",	},
				};

				assert_table_equal(name_times, reference3, *p);

				// ACT
				p->set_order(columns::times_called, false);

				// ASSERT
				wstring reference4[][2] = {
					{	L"00002995", L"700",	},
					{	L"00003001", L"30",	},
					{	L"00002978", L"3",	},
				};

				assert_table_equal(name_times, reference4, *p);
			}


			test( ParentStatisticsResortingCausesInvalidation )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				invalidation_tracer t;
				unthreaded_statistic_types::map_detailed s1, s2;

				s1[0x2978].callees[0x3001];
				s1[0x2995].callees[0x3001];
				s1[0x3001].callees[0x3001];
				serialize_single_threaded(ser, s1);
				s2[0x3002].callees[0x3001];
				serialize_single_threaded(ser, s2);

				fl->set_order(columns::name, true);
				dser(*fl, dummy_context);
				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				t.bind_to_model(*p);

				// ACT
				p->set_order(columns::name, true);

				// ASSERT
				table_model::index_type reference1[] = { 3u, };

				assert_equal(reference1, t.invalidations);

				// ACT
				dser(*fl, dummy_context);
				p->set_order(columns::times_called, false);

				// ASSERT
				table_model::index_type reference2[] = { 3u, 4u, 4u, };

				assert_equal(reference2, t.invalidations);
			}


			test( ParentStatisticsCausesInvalidationAfterTheSort )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(700);
				s[0x3001].callees[0x3001] = function_statistics(30);
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(columns::times_called, true);

				wpl::slot_connection c = p->invalidated += invalidation_at_sorting_check1(*p);

				// ACT / ASSERT
				p->set_order(columns::times_called, false);
			}


			test( ParentStatisticsIsUpdatedOnGlobalUpdates2 )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(30);
				s[0x3001].callees[0x3001] = function_statistics(50);
				serialize_single_threaded(ser, s);
				s.erase(0x3001);
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(columns::times_called, true);

				// pre-ASSERT
				wstring reference1[][2] = {
					{	L"00002978", L"3",	},
					{	L"00002995", L"30",	},
					{	L"00003001", L"50",	},
				};

				assert_table_equal(name_times, reference1, *p);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				wstring reference2[][2] = {
					{	L"00002978", L"6",	},
					{	L"00003001", L"50",	},
					{	L"00002995", L"60",	},
				};

				assert_table_equal(name_times, reference2, *p);
			}


			test( GettingAddressOfParentStatisticsItem )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_statistic_types::map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(30);
				s[0x3001].callees[0x3001] = function_statistics(50);
				serialize_single_threaded(ser, s);

				fl->set_order(columns::name, true);

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(columns::times_called, true);

				// ACT / ASSERT
				assert_equal(addr(0x2978), p->get_key(0));
				assert_equal(addr(0x2995), p->get_key(1));
				assert_equal(addr(0x3001), p->get_key(2));
			}


			test( ThreadsAreAccumulatedInTheContext )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_addressed_function functions[] = { make_statistics(0x1000u, 1, 0, 0, 0, 0), };
				pair< unsigned, vector<unthreaded_addressed_function> > data1[] = {
					make_pair(3, mkvector(functions)), make_pair(2, mkvector(functions)),
				};
				pair< unsigned, vector<unthreaded_addressed_function> > data2[] = {
					make_pair(3, mkvector(functions)), make_pair(2, mkvector(functions)), make_pair(9112, mkvector(functions)),
				};
				vector<unsigned> collected_ids;

				ser(mkvector(data1));
				ser(mkvector(data2));

				// ACT
				dser(*fl, collected_ids);

				// ASSERT
				unsigned reference1[] = { 2u, 3u, };

				assert_equivalent(reference1, collected_ids);

				// ACT
				dser(*fl, collected_ids);

				// ASSERT
				unsigned reference2[] = { 2u, 3u, 9112u, };

				assert_equivalent(reference2, collected_ids);
			}

		end_test_suite
	}
}
