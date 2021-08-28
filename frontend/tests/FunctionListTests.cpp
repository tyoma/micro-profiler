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
using namespace wpl;

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

			const timestamp_t test_ticks_per_second = 1;

			void increment(int *value)
			{	++*value;	}

			class invalidation_tracer
			{
				wpl::slot_connection _connection;

				void on_invalidate(table_model_base::index_type index, table_model_base *m)
				{
					counts.push_back(m->get_count());
					assert_equal(table_model_base::npos(), index);
				}

			public:
				template <typename ModelT>
				void bind_to_model(ModelT &to)
				{	_connection = to.invalidate += bind(&invalidation_tracer::on_invalidate, this, _1, &to);	}

				vector<table_model_base::index_type> counts;
			};


			class invalidation_at_sorting_check1
			{
				const richtext_table_model &_model;

				const invalidation_at_sorting_check1 &operator =(const invalidation_at_sorting_check1 &);

			public:
				invalidation_at_sorting_check1(richtext_table_model &m)
					: _model(m)
				{	}

				void operator ()(table_model_base::index_type /*count*/) const
				{
					string reference[][2] = {
						{	"00002995", "700",	},
						{	"00003001", "30",	},
						{	"00002978", "3",	},
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

			template <typename ModelT>
			table_model_base::index_type find_row(const ModelT &m, const string &name)
			{
				string result;

				for (table_model_base::index_type i = 0, c = m.get_count(); i != c; ++i)
					if (get_text(m, i, columns::name) == name)
						return i;
				return table_model_base::npos();
			}
		}


		begin_test_suite( FunctionListTests )

			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;
			scontext::wire dummy_context;

			FunctionListTests()
				: ser(_buffer), dser(_buffer)
			{	}

			init( CreatePrerequisites )
			{
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver.reset(new mocks::symbol_resolver(modules, mappings));
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
				wpl::slot_connection conn = fl->invalidate += bind(&increment, &invalidated_count);

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
				assert_equal("1", get_text(*fl, 0, 3));
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
				assert_equal(1u, ih.counts.size());
				assert_equal(1u, ih.counts.back()); //check what's coming as event arg

				// ACT
				shared_ptr<const trackable> first = fl->track(0); // 2229

				// ASSERT
				assert_equal(0u, first->index());

				// ACT
				fl->clear();

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal(2u, ih.counts.size());
				assert_equal(0u, ih.counts.back()); //check what's coming as event arg
				assert_equal(table_model_base::npos(), first->index());

				// INIT
				_buffer.rewind();

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(3u, ih.counts.size());
				assert_equal(1u, ih.counts.back()); //check what's coming as event arg
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

				shared_ptr<linked_statistics> children[] = {	fl->watch_children(addr(1123)), fl->watch_children(addr(1124)),	};

				it1.bind_to_model(*children[0]);
				it2.bind_to_model(*children[1]);

				// ACT
				fl->clear();

				// ASSERT
				table_model_base::index_type reference[] = { 0, };

				assert_equal(0u, children[0]->get_count());
				assert_equal(reference, it1.counts);
				assert_equal(0u, children[1]->get_count());
				assert_equal(reference, it2.counts);
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

				shared_ptr<linked_statistics> parents[] = {	fl->watch_parents(addr(1123)), fl->watch_parents(addr(1124)),	};

				it1.bind_to_model(*parents[0]);
				it2.bind_to_model(*parents[1]);

				// ACT
				fl->clear();

				// ASSERT
				table_model_base::index_type reference[] = { 0, };

				assert_equal(0u, parents[0]->get_count());
				assert_equal(reference, it1.counts);
				assert_equal(0u, parents[1]->get_count());
				assert_equal(reference, it2.counts);
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

				for (table_model_base::index_type i = 0; i < expected.size(); ++i)
					assert_equal(expected[i], i);

				assert_not_equal(table_model_base::npos(), idx1118);
				assert_not_equal(table_model_base::npos(), idx2229);
				assert_not_equal(table_model_base::npos(), idx5550);
				assert_equal(table_model_base::npos(), fl->get_index(addr(1234)));

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
				assert_equal(1u, ih.counts.size());
				assert_equal(2u, ih.counts.back()); //check what's coming as event arg
		
				string reference1[][8] = {
					{	"0000045E", "19", "31s", "29s", "1.63s", "1.53s", "0", "3s"	},
					{	"000008B5", "10", "7s", "5s", "700ms", "500ms", "3", "4s"	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *fl);

				assert_equal(0u, first->index());
				assert_equal(1u, second->index());

				// ACT
				dser(*fl, dummy_context);
				
				// ASSERT
				assert_equal(2u, ih.counts.size());
				assert_equal(3u, ih.counts.back()); //check what's coming as event arg

				string reference2[][8] = {
					{	"0000045E", "24", "41s", "36s", "1.71s", "1.5s", "0", "6s",	},
					{	"000008B5", "10", "7s", "5s", "700ms", "500ms", "3", "4s",	},
					{	"000015AE", "15", "1011s", "723s", "67.4s", "48.2s", "1024", "215s",	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *fl);

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // kind of moved down

				// ACT
				dser(*fl, dummy_context);
				
				// ASSERT
				assert_equal(3u, ih.counts.size());
				assert_equal(3u, ih.counts.back()); //check what's coming as event arg

				string reference3[][8] = {
					{	"0000045E", "100111222357", "1.7e+04s", "1.4e+04s", "170ns", "140ns", "0", "6s",	},
					{	"000008B5", "10", "7s", "5s", "700ms", "500ms", "3", "4s",	},
					{	"000015AE", "15", "1011s", "723s", "67.4s", "48.2s", "1024", "215s",	},
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
				string reference[][8] = {
					{	"0000045E", "1", "3.1ns", "2.9ns", "3.1ns", "2.9ns", "0", "2.9ns",	},
					{	"000008B5", "1", "4.53\xCE\xBCs", "3.67\xCE\xBCs", "4.53\xCE\xBCs", "3.67\xCE\xBCs", "0", "3.67\xCE\xBCs",	},
					{	"00000C2E", "1", "3.35ms", "3.23ms", "3.35ms", "3.23ms", "0", "3.23ms",	},
					{	"000015AE", "1", "6.55s", "2.35s", "6.55s", "2.35s", "0", "2.35s",	},
					{	"000011C6", "1", "6545s", "2347s", "6545s", "2347s", "0", "2347s",	},
					{	"00001A05", "1", "6.55e+06s", "2.35e+06s", "6.55e+06s", "2.35e+06s", "0", "2.35e+06s",	},

					// boundary cases
					{	"000007C6", "1", "999ns", "999ns", "999ns", "999ns", "0", "999ns",	},
					{	"000007D0", "1", "1\xCE\xBCs", "1\xCE\xBCs", "1\xCE\xBCs", "1\xCE\xBCs", "0", "1\xCE\xBCs",	},
					{	"00000BAE", "1", "999\xCE\xBCs", "999\xCE\xBCs", "999\xCE\xBCs", "999\xCE\xBCs", "0", "999\xCE\xBCs",	},
					{	"00000BB8", "1", "1ms", "1ms", "1ms", "1ms", "0", "1ms",	},
					{	"00000F96", "1", "999ms", "999ms", "999ms", "999ms", "0", "999ms",	},
					{	"00000FA0", "1", "1s", "1s", "1s", "1s", "0", "1s",	},
					{	"0000137E", "1", "999s", "999s", "999s", "999s", "0", "999s",	},
					{	"00001388", "1", "999.6s", "999.6s", "999.6s", "999.6s", "0", "999.6s",	},
					{	"00001766", "1", "9999s", "9999s", "9999s", "9999s", "0", "9999s",	},
					{	"00001770", "1", "1e+04s", "1e+04s", "1e+04s", "1e+04s", "0", "1e+04s",	},
				};

				assert_table_equivalent(name_times_inc_exc_iavg_eavg_reent_minc, reference, *fl);
			}


			test( FunctionListProvidesSelectionModel )
			{
				// INIT
				const auto fl = functions_list::create(test_ticks_per_second, resolver, tmodel);

				// INIT / ACT
				const shared_ptr< selection<function_key> > s1 = fl->create_selection();
				const shared_ptr< selection<function_key> > s2 = fl->create_selection();

				// ASSERT
				assert_not_null(s1);
				assert_not_null(s2);
				assert_not_equal(s2, s1);
			}


			test( FunctionListSorting )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				const auto s = fl->create_selection();
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
				s->add(1);

				// ACT / ASSERT
				assert_is_false(s->contains(0));
				assert_is_true(s->contains(1));
				assert_is_false(s->contains(2));
				assert_is_false(s->contains(3));

				// ASSERT
				assert_equal(2u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference1[][8] = {
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "2", "5s",	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "0", "3s",	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "1", "4s",	},
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "3", "6s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *fl);

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (times called, descending)
				fl->set_order(columns::times_called, false);

				// ACT / ASSERT
				assert_is_false(s->contains(0));
				assert_is_false(s->contains(1));
				assert_is_true(s->contains(2));
				assert_is_false(s->contains(3));

				// ASSERT
				assert_equal(3u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference2[][8] = {
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "3", "6s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "1", "4s"	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "0", "3s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "2", "5s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *fl);

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (name, ascending; after times called to see that sorting in asc direction works)
				fl->set_order(columns::name, true);

				// ASSERT
				assert_equal(4u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference3[][8] = {
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "0", "3s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "1", "4s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "2", "5s"	},
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "3", "6s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference3, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (name, descending)
				fl->set_order(columns::name, false);

				// ASSERT
				assert_equal(5u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference4[][8] = {
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "3", "6s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "2", "5s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "1", "4s"	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "0", "3s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference4, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (exclusive time, ascending)
				fl->set_order(columns::exclusive, true);

				// ASSERT
				assert_equal(6u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference5[][2] = {
					{	"000007C6", "15",	},
					{	"000007D0", "35",	},
					{	"00000BB8", "15233",	},
					{	"00000BAE", "2",	},
				};

				assert_table_equal(name_times, reference5, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (exclusive time, descending)
				fl->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(7u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference6[][2] = {
					{	"00000BAE", "2",	},
					{	"00000BB8", "15233",	},
					{	"000007D0", "35",	},
					{	"000007C6", "15",	},
				};

				assert_table_equal(name_times, reference6, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (inclusive time, ascending)
				fl->set_order(columns::inclusive, true);

				// ASSERT
				assert_equal(8u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference7[][2] = {
					{	"000007C6", "15",	},
					{	"000007D0", "35",	},
					{	"00000BB8", "15233",	},
					{	"00000BAE", "2",	},
				};

				assert_table_equal(name_times, reference7, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (inclusive time, descending)
				fl->set_order(columns::inclusive, false);

				// ASSERT
				assert_equal(9u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference8[][2] = {
					{	"00000BAE", "2",	},
					{	"00000BB8", "15233",	},
					{	"000007D0", "35",	},
					{	"000007C6", "15",	},
				};

				assert_table_equal(name_times, reference8, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());
				
				// ACT (avg. exclusive time, ascending)
				fl->set_order(columns::exclusive_avg, true);
				
				// ASSERT
				assert_equal(10u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference9[][2] = {
					{	"00000BB8", "15233",	},
					{	"000007C6", "15",	},
					{	"000007D0", "35",	},
					{	"00000BAE", "2",	},
				};

				assert_table_equal(name_times, reference9, *fl);

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (avg. exclusive time, descending)
				fl->set_order(columns::exclusive_avg, false);
				
				// ASSERT
				assert_equal(11u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference10[][2] = {
					{	"00000BAE", "2",	},
					{	"000007D0", "35",	},
					{	"000007C6", "15",	},
					{	"00000BB8", "15233",	},
				};

				assert_table_equal(name_times, reference10, *fl);

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (avg. inclusive time, ascending)
				fl->set_order(columns::inclusive_avg, true);
				
				// ASSERT
				assert_equal(12u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference11[][2] = {
					{	"000007C6", "15",	},
					{	"00000BB8", "15233",	},
					{	"000007D0", "35",	},
					{	"00000BAE", "2",	},
				};

				assert_table_equal(name_times, reference11, *fl);

				assert_equal(0u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (avg. inclusive time, descending)
				fl->set_order(columns::inclusive_avg, false);
				
				// ASSERT
				assert_equal(13u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference12[][2] = {
					{	"00000BAE", "2",	},
					{	"000007D0", "35",	},
					{	"00000BB8", "15233",	},
					{	"000007C6", "15",	},
				};

				assert_table_equal(name_times, reference12, *fl);

				assert_equal(3u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (max reentrance, ascending)
				fl->set_order(columns::max_reentrance, true);
				
				// ASSERT
				assert_equal(14u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference13[][2] = {
					{	"000007C6", "15",	},
					{	"000007D0", "35",	},
					{	"00000BAE", "2",	},
					{	"00000BB8", "15233",	},
				};

				assert_table_equal(name_times, reference13, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max reentrance, descending)
				fl->set_order(columns::max_reentrance, false);
				
				// ASSERT
				assert_equal(15u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference14[][2] = {
					{	"00000BB8", "15233",	},
					{	"00000BAE", "2",	},
					{	"000007D0", "35",	},
					{	"000007C6", "15",	},
				};

				assert_table_equal(name_times, reference14, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (max call time, ascending)
				fl->set_order(columns::max_time, true);
				
				// ASSERT
				assert_equal(16u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference15[][2] = {
					{	"000007C6", "15",	},
					{	"000007D0", "35",	},
					{	"00000BAE", "2",	},
					{	"00000BB8", "15233",	},
				};

				assert_table_equal(name_times, reference15, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max call time, descending)
				fl->set_order(columns::max_time, false);
				
				// ASSERT
				assert_equal(17u, ih.counts.size());
				assert_equal(data_size, ih.counts.back()); //check what's coming as event arg

				string reference16[][2] = {
					{	"00000BB8", "15233",	},
					{	"00000BAE", "2",	},
					{	"000007D0", "35",	},
					{	"000007C6", "15",	},
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
				string reference1[][2] = {
					{	"00001000", "100",	},
					{	"00001010", "1000",	},
					{	"00001020", "900",	},
					{	"00001030", "",	},
				};

				assert_table_equivalent(name_threadid, reference1, *fl);

				// INIT
				tmodel->add(9, 90, string());

				// ACT / ASSERT
				string reference2[][2] = {
					{	"00001000", "100",	},
					{	"00001010", "1000",	},
					{	"00001020", "900",	},
					{	"00001030", "90",	},
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
				ih.counts.clear();

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, true);

				// ASSERT
				columns::main ordering[] = {	columns::times_called,	};
				size_t reference_updates1[] = {	data_size,	};
				string reference1[][1] = {	{	"2",	}, {	"1",	}, {	"4",	}, {	"3",	}, {	"5",	},	};

				assert_table_equal(ordering, reference1, *fl);
				assert_equal(reference_updates1, ih.counts);

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, false);

				// ASSERT
				size_t reference_updates2[] = {	data_size, data_size,	};
				string reference2[][1] = {	{	"5",	}, {	"3",	},{	"4",	},{	"1",	},  {	"2",	}, 	};

				assert_table_equal(ordering, reference2, *fl);
				assert_equal(reference_updates2, ih.counts);
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
				assert_throws(fl1->watch_children(addr(1979)), out_of_range);
				assert_throws(fl1->watch_children(addr(1994)), out_of_range);
				assert_throws(fl2->watch_children(addr(2002)), out_of_range);
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
				assert_not_null(fl1->watch_children(addr(1978)));
				assert_not_null(fl1->watch_children(addr(1995)));
				assert_not_null(fl2->watch_children(addr(2001)));
				assert_not_null(fl2->watch_children(addr(2004)));
				assert_not_null(fl2->watch_children(addr(2011)));
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
				shared_ptr<linked_statistics> ls = fl->watch_children(addr(1973));

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
				shared_ptr<linked_statistics> ls_0 = fl->watch_children(addr(0x1978));
				shared_ptr<linked_statistics> ls_1 = fl->watch_children(addr(0x1995));

				// ACT / ASSERT
				assert_equal(1u, ls_0->get_count());
				assert_not_equal(table_model_base::npos(), find_row(*ls_0, "00002001"));
				assert_equal(3u, ls_1->get_count());
				assert_not_equal(table_model_base::npos(), find_row(*ls_1, "00002004"));
				assert_not_equal(table_model_base::npos(), find_row(*ls_1, "00002008"));
				assert_not_equal(table_model_base::npos(), find_row(*ls_1, "00002011"));
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

				shared_ptr<linked_statistics> ls = fl->watch_children(addr(0x1978));

				// ACT
				ls->set_order(columns::name, false);

				// ASSERT
				string reference1[][8] = {
					{	"00002011", "29", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002008", "18", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002004", "17", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002001", "22", "0s", "0s", "0s", "0s", "0", "0s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *ls);

				// ACT
				ls->set_order(columns::times_called, true);

				// ASSERT
				string reference2[][8] = {
					{	"00002004", "17", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002008", "18", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002001", "22", "0s", "0s", "0s", "0s", "0", "0s",	},
					{	"00002011", "29", "0s", "0s", "0s", "0s", "0", "0s",	},
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

				shared_ptr<linked_statistics> ls = fl->watch_children(addr(0x1978));
				ls->set_order(columns::name, true);

				// ACT / ASSERT
				string reference[][8] = {
					{	"00002001", "11", "100ms", "700ms", "9.09ms", "63.6ms", "0", "9.1s",	},
					{	"00002004", "17", "200ms", "800ms", "11.8ms", "47.1ms", "5", "9.7s",	},
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

				shared_ptr<linked_statistics> ls1 = fl->watch_children(addr(0x1978));
				shared_ptr<linked_statistics> ls2 = fl->watch_parents(addr(0x1978));

				fl->clear();
				t1.bind_to_model(*ls1);
				t2.bind_to_model(*ls2);

				// ACT (linked statistics are detached)
				dser(*fl, dummy_context);

				// ASSERT
				assert_is_empty(t1.counts);
				assert_is_empty(t2.counts);
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

				shared_ptr<linked_statistics> ls1 = fl->watch_children(addr(0x1978));
				shared_ptr<linked_statistics> ls2 = fl->watch_parents(addr(0x1978));

				t1.bind_to_model(*ls1);
				t2.bind_to_model(*ls2);

				// ACT
				fl.reset();

				// ASSERT
				table_model_base::index_type reference[] = { 0u, };

				assert_equal(reference, t1.counts);
				assert_equal(reference, t2.counts);
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

				shared_ptr<linked_statistics> ls = fl->watch_children(addr(0x1978));

				t.bind_to_model(*ls);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1u, t.counts.size());
				assert_equal(2u, t.counts.back());

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(2u, t.counts.size());
				assert_equal(3u, t.counts.back());

				// INIT
				unthreaded_statistic_types::map_detailed s3;

				s3[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				serialize_single_threaded(ser, s3);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(3u, t.counts.size());
				assert_equal(3u, t.counts.back());
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

				shared_ptr<linked_statistics> ls = fl->watch_children(addr(0x1978));

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
				assert_throws(fl1->watch_parents(addr(0x1977)), out_of_range);
				assert_throws(fl1->watch_parents(addr(0x1996)), out_of_range);
				assert_throws(fl2->watch_parents(addr(0x2000)), out_of_range);
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
				assert_not_null(fl1->watch_parents(addr(0x1978)));
				assert_not_null(fl1->watch_parents(addr(0x1995)));
				assert_not_null(fl2->watch_parents(addr(0x2001)));
				assert_not_null(fl2->watch_parents(addr(0x2004)));
				assert_not_null(fl2->watch_parents(addr(0x2008)));
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

				// ACT
				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p0 = fl->watch_parents(addr(2978));
				shared_ptr<linked_statistics> p1 = fl->watch_parents(addr(2995));
				shared_ptr<linked_statistics> p2 = fl->watch_parents(addr(3001));

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

				// ACT
				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(addr(2978));

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

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p0 = fl->watch_parents(addr(0x122F));
				shared_ptr<linked_statistics> p1 = fl->watch_parents(addr(0x2340));
				shared_ptr<linked_statistics> p2 = fl->watch_parents(addr(0x3451));

				// ACT / ASSERT
				string reference1[][2] = {	{	"00003451", "1",	},	};
				string reference2[][2] = {	{	"0000122F", "3",	},	};
				string reference3[][2] = {	{	"00002340", "5000000000",	},	};

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

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(addr(0x3001));

				// ACT
				p->set_order(columns::name, true);

				// ASSERT
				string reference1[][2] = {
					{	"00002978", "3",	},
					{	"00002995", "700",	},
					{	"00003001", "30",	},
				};

				assert_table_equal(name_times, reference1, *p);

				// ACT
				p->set_order(columns::name, false);

				// ASSERT
				string reference2[][2] = {
					{	"00003001", "30",	},
					{	"00002995", "700",	},
					{	"00002978", "3",	},
				};

				assert_table_equal(name_times, reference2, *p);

				// ACT
				p->set_order(columns::times_called, true);

				// ASSERT
				string reference3[][2] = {
					{	"00002978", "3",	},
					{	"00003001", "30",	},
					{	"00002995", "700",	},
				};

				assert_table_equal(name_times, reference3, *p);

				// ACT
				p->set_order(columns::times_called, false);

				// ASSERT
				string reference4[][2] = {
					{	"00002995", "700",	},
					{	"00003001", "30",	},
					{	"00002978", "3",	},
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

				dser(*fl, dummy_context);
				shared_ptr<linked_statistics> p = fl->watch_parents(addr(0x3001));

				t.bind_to_model(*p);

				// ACT
				p->set_order(columns::name, true);

				// ASSERT
				table_model_base::index_type reference1[] = { 3u, };

				assert_equal(reference1, t.counts);

				// ACT
				dser(*fl, dummy_context);
				p->set_order(columns::times_called, false);

				// ASSERT
				table_model_base::index_type reference2[] = { 3u, 4u, 4u, };

				assert_equal(reference2, t.counts);
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

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(addr(0x3001));

				p->set_order(columns::times_called, true);

				wpl::slot_connection c = p->invalidate += invalidation_at_sorting_check1(*p);

				// ACT / ASSERT
				p->set_order(columns::times_called, false);
			}


			test( ParentStatisticsIsUpdatedOnGlobalUpdates2 )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));

				unthreaded_addressed_function batch1[] = {
					make_statistics(0x2978u, 0, 0, 0, 0, 0,
						make_statistics_base(0x3001u, 3, 0, 0, 0, 0)),
					make_statistics(0x2995u, 0, 0, 0, 0, 0,
						make_statistics_base(0x3001u, 30, 0, 0, 0, 0)),
					make_statistics(0x3001u, 0, 0, 0, 0, 0,
						make_statistics_base(0x3001u, 50, 0, 0, 0, 0)),
				};
				unthreaded_addressed_function batch2[] = {
					make_statistics(0x2978u, 0, 0, 0, 0, 0,
						make_statistics_base(0x3001u, 3, 0, 0, 0, 0)),
					make_statistics(0x2995u, 0, 0, 0, 0, 0,
						make_statistics_base(0x3001u, 30, 0, 0, 0, 0)),
				};

				serialize_single_threaded(ser, mkvector(batch1));
				serialize_single_threaded(ser, mkvector(batch2));

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(addr(0x3001));

				p->set_order(columns::times_called, true);

				// pre-ASSERT
				string reference1[][2] = {
					{	"00002978", "3",	},
					{	"00002995", "30",	},
					{	"00003001", "50",	},
				};

				assert_table_equal(name_times, reference1, *p);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				string reference2[][2] = {
					{	"00002978", "6",	},
					{	"00003001", "50",	},
					{	"00002995", "60",	},
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

				dser(*fl, dummy_context);

				shared_ptr<linked_statistics> p = fl->watch_parents(addr(0x3001));

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
				scontext::wire collected_ids;

				ser(mkvector(data1));
				ser(mkvector(data2));

				// ACT
				dser(*fl, collected_ids);

				// ASSERT
				unsigned reference1[] = { 2u, 3u, };

				assert_equivalent(reference1, collected_ids.threads);

				// ACT
				dser(*fl, collected_ids);

				// ASSERT
				unsigned reference2[] = { 2u, 3u, 9112u, };

				assert_equivalent(reference2, collected_ids.threads);
			}


			test( OnlyAllowedItemsAreExposedByTheModelAfterFilterApplication )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_addressed_function functions[][2] = {
					{ make_statistics(0x1000u, 1, 0, 0, 0, 0), make_statistics(0x1010u, 2, 0, 0, 0, 0), },
					{ make_statistics(0x1020u, 3, 0, 0, 0, 0), make_statistics(0x1030u, 4, 0, 0, 0, 0), },
					{ make_statistics(0x1040u, 5, 0, 0, 0, 0), make_statistics(0x1050u, 6, 0, 0, 0, 0), },
				};
				pair< unsigned, vector<unthreaded_addressed_function> > data[] = {
					make_pair(0, mkvector(functions[0])),
					make_pair(2, mkvector(functions[1])),
					make_pair(3, mkvector(functions[2])),
				};

				ser(mkvector(data));
				dser(*fl, dummy_context);

				// ACT
				fl->set_filter([] (const functions_list::value_type &v) { return v.first.second == 3; });

				// ASSERT
				string reference1[][2] = {
					{	"00001040", "5",	},
					{	"00001050", "6",	},
				};

				assert_table_equivalent(name_times, reference1, *fl);

				// ACT
				fl->set_filter([] (const functions_list::value_type &v) { return v.first.second == 0; });

				// ASSERT
				string reference2[][2] = {
					{	"00001000", "1",	},
					{	"00001010", "2",	},
				};

				assert_table_equivalent(name_times, reference2, *fl);

				// ACT
				fl->set_filter([] (const functions_list::value_type &v) { return v.second.times_called > 3; });

				// ASSERT
				string reference3[][2] = {
					{	"00001030", "4",	},
					{	"00001040", "5",	},
					{	"00001050", "6",	},
				};

				assert_table_equivalent(name_times, reference3, *fl);

				// ACT
				fl->set_filter();

				// ASSERT
				string reference4[][2] = {
					{	"00001000", "1",	},
					{	"00001010", "2",	},
					{	"00001020", "3",	},
					{	"00001030", "4",	},
					{	"00001040", "5",	},
					{	"00001050", "6",	},
				};

				assert_table_equivalent(name_times, reference4, *fl);
			}


			test( InvalidationIsEmittedOnFilterChange )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver, tmodel));
				unthreaded_addressed_function functions[] = {
					make_statistics(0x1000u, 1, 0, 0, 0, 0), make_statistics(0x1010u, 2, 0, 0, 0, 0),
				};
				invalidation_tracer it;

				it.bind_to_model(*fl);

				serialize_single_threaded(ser, mkvector(functions));
				dser(*fl, dummy_context);
				it.counts.clear();

				// ACT
				fl->set_filter([] (const functions_list::value_type &v) { return v.second.times_called > 1; });

				// ASSERT
				table_model_base::index_type reference1[] = { 1u, };

				assert_equal(reference1, it.counts);

				// ACT
				fl->set_filter([] (const functions_list::value_type &v) { return v.second.times_called > 2; });

				// ASSERT
				table_model_base::index_type reference2[] = { 1u, 0u, };

				assert_equal(reference2, it.counts);

				// ACT
				fl->set_filter();

				// ASSERT
				table_model_base::index_type reference3[] = { 1u, 0u, 2u, };

				assert_equal(reference3, it.counts);
			}

		end_test_suite
	}
}
