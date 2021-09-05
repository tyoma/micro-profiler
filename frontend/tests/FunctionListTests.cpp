#include <frontend/function_list.h>

#include "helpers.h"
#include "mocks.h"

#include <iomanip>
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
			const columns::main name_times_inc_exc_iavg_eavg_reent_minc[] = {
				columns::name, columns::times_called, columns::inclusive, columns::exclusive,
				columns::inclusive_avg, columns::exclusive_avg, columns::max_reentrance, columns::max_time
			};

			const columns::main name_times[] = {	columns::name, columns::times_called,	};

			const columns::main name_threadid[] = {	columns::name, columns::threadid,	};

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
			std::shared_ptr<tables::statistics> statistics;
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;

			init( CreatePrerequisites )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver.reset(new mocks::symbol_resolver(modules, mappings));
				tmodel.reset(new mocks::threads_model);
			}


			test( ConstructedFunctionListIsEmpty )
			{
				// INIT / ACT
				auto fl = make_shared<functions_list>(statistics, 1.0 , resolver, tmodel);

				// ACT / ASSERT
				assert_equal(0u, fl->get_count());
			}


			test( FunctionListAcceptsUpdates )
			{
				// INIT
				auto fl = make_shared<functions_list>(statistics, 1.0, resolver, tmodel);

				assign(*statistics, plural
					+ make_statistics(addr(1123), 19, 0, 0, 0, 1)
					+ make_statistics(addr(2234), 29, 0, 0, 0, 1));

				// ACT / ASSERT
				assert_equal(0u, fl->get_count());

				// ACT
				statistics->invalidate();

				// ASSERT
				assert_equal(2u, fl->get_count());

				// ACT
				static_cast<function_statistics &>((*statistics)[addr(7234)]) = function_statistics(10, 1, 1, 1);
				statistics->invalidate();

				// ASSERT
				assert_equal(3u, fl->get_count());

				// ACT
				statistics->clear();

				// ASSERT
				assert_equal(0u, fl->get_count());
			}


			test( FunctionListTimeFormatter )
			{
				// INIT
				unsigned columns[] = {	1, 5, 4, 7, 6, 9,};
				auto functions = plural
					// ~ ns
					+ make_statistics(addr(0x45Eu), 1, 0, 31, 29, 29)
					+ make_statistics(addr(0x7C6u), 1, 0, 9994, 9994, 9994)

					// >= 1us
					+ make_statistics(addr(0x7D0u), 1, 0, 9996, 9996, 9996)
					+ make_statistics(addr(0x8B5u), 1, 0, 45340, 36666, 36666)
					+ make_statistics(addr(0xBAEu), 1, 0, 9994000, 9994000, 9994000)

					// >= 1ms
					+ make_statistics(addr(0xBB8u), 1, 0, 9996000, 9996000, 9996000)
					+ make_statistics(addr(0xC2Eu), 1, 0, 33450030, 32333333, 32333333)
					+ make_statistics(addr(0xF96u), 1, 0, 9994000000, 9994000000, 9994000000)

					// >= 1s
					+ make_statistics(addr(0xFA0u), 1, 0, 9996000000, 9996000000, 9996000000)
					+ make_statistics(addr(0x15AEu), 1, 0, 65450031030, 23470030000, 23470030000)
					+ make_statistics(addr(0x137Eu), 1, 0, 9994000000000, 9994000000000, 9994000000000)

					// >= 1000s
					+ make_statistics(addr(0x1388u), 1, 0, 9996000000000, 9996000000000, 9996000000000)
					+ make_statistics(addr(0x11C6u), 1, 0, 65450031030567, 23470030000987, 23470030000987)
					+ make_statistics(addr(0x1766u), 1, 0, 99990031030567, 99990030000987, 99990030000987)
				
					// >= 10000s
					+ make_statistics(addr(0x1770u), 1, 0, 99999031030567, 99999030000987, 99999030000987)
					+ make_statistics(addr(0x1A05u), 1, 0, 65450031030567000, 23470030000987000, 23470030000987000);

				assign(*statistics, functions);

				auto fl = make_shared<functions_list>(statistics, 1e-10, resolver, tmodel);

				// ACT
				auto text = get_text(*fl, columns);

				// ASSERT
				string reference[][6] = {
					{	"0000045E", "3.1ns", "2.9ns", "3.1ns", "2.9ns", "2.9ns",	},
					{	"000008B5", "4.53\xCE\xBCs", "3.67\xCE\xBCs", "4.53\xCE\xBCs", "3.67\xCE\xBCs", "3.67\xCE\xBCs",	},
					{	"00000C2E", "3.35ms", "3.23ms", "3.35ms", "3.23ms", "3.23ms",	},
					{	"000015AE", "6.55s", "2.35s", "6.55s", "2.35s", "2.35s",	},
					{	"000011C6", "6545s", "2347s", "6545s", "2347s", "2347s",	},
					{	"00001A05", "6.55e+06s", "2.35e+06s", "6.55e+06s", "2.35e+06s", "2.35e+06s",	},

					// boundary cases
					{	"000007C6", "999ns", "999ns", "999ns", "999ns", "999ns",	},
					{	"000007D0", "1\xCE\xBCs", "1\xCE\xBCs", "1\xCE\xBCs", "1\xCE\xBCs", "1\xCE\xBCs",	},
					{	"00000BAE", "999\xCE\xBCs", "999\xCE\xBCs", "999\xCE\xBCs", "999\xCE\xBCs", "999\xCE\xBCs",	},
					{	"00000BB8", "1ms", "1ms", "1ms", "1ms", "1ms",	},
					{	"00000F96", "999ms", "999ms", "999ms", "999ms", "999ms",	},
					{	"00000FA0", "1s", "1s", "1s", "1s", "1s",	},
					{	"0000137E", "999s", "999s", "999s", "999s", "999s",	},
					{	"00001388", "999.6s", "999.6s", "999.6s", "999.6s", "999.6s",	},
					{	"00001766", "9999s", "9999s", "9999s", "9999s", "9999s",	},
					{	"00001770", "1e+04s", "1e+04s", "1e+04s", "1e+04s", "1e+04s",	},
				};

				assert_equivalent(mkvector(reference), text);
			}


			test( FunctionListProvidesSelectionModel )
			{
				// INIT
				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

				// INIT / ACT
				const shared_ptr< selection<function_key> > s1 = fl->create_selection();
				const shared_ptr< selection<function_key> > s2 = fl->create_selection();

				// ASSERT
				assert_not_null(s1);
				assert_not_null(s2);
				assert_not_equal(s2, s1);
			}


			test( SelectionModelIsFunctionalAfterParentModelIsDestroyed )
			{
				// INIT
				auto functions = plural
					+ make_statistics(addr(1990u), 15, 0, 31, 29, 3)
					+ make_statistics(addr(2000u), 35, 1, 453, 366, 4)
					+ make_statistics(addr(2990u), 2, 2, 33450030, 32333333, 5)
					+ make_statistics(addr(3000u), 15233, 3, 65460, 13470, 6);

				assign(*statistics, functions);

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

				// INIT / ACT
				auto sel = fl->create_selection();

				// ACT
				sel->add(1);
				fl.reset();

				// ASSERT
				assert_is_false(sel->contains(0));
				assert_is_true(sel->contains(1));
				assert_is_false(sel->contains(2));
				assert_is_false(sel->contains(3));
			}


			test( FunctionListSorting )
			{
				// INIT
				invalidation_tracer ih;
				auto functions = plural
					+ make_statistics(addr(1990u), 15, 0, 31, 29, 3)
					+ make_statistics(addr(2000u), 35, 1, 453, 366, 4)
					+ make_statistics(addr(2990u), 2, 2, 33450030, 32333333, 5)
					+ make_statistics(addr(3000u), 15233, 3, 65460, 13470, 6);

				assign(*statistics, functions);

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);
				const auto s = fl->create_selection();

				ih.bind_to_model(*fl);

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
				assert_equal(1u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(2u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(3u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(4u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(5u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(6u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(7u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(8u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(9u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(10u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(11u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(12u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(13u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(14u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(15u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				assert_equal(16u, ih.counts.size());
				assert_equal(functions.size(), ih.counts.back()); //check what's coming as event arg

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
				unsigned columns[] = {	1, 2,	};
				auto functions = plural
					+ make_statistics(addr(0x1000u, 3), 1, 0, 0, 0, 0)
					+ make_statistics(addr(0x1010u, 2), 1, 0, 0, 0, 0)
					+ make_statistics(addr(0x1020u, 7), 1, 0, 0, 0, 0)
					+ make_statistics(addr(0x1030u, 9), 1, 0, 0, 0, 0);

				assign(*statistics, functions);

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

				tmodel->add(3, 100, string());
				tmodel->add(2, 1000, string());
				tmodel->add(7, 900, string());

				// ACT
				auto text = get_text(*fl, columns);

				// ASSERT
				string reference1[][2] = {
					{	"00001000", "100",	},
					{	"00001010", "1000",	},
					{	"00001020", "900",	},
					{	"00001030", "",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				tmodel->add(9, 90, string());

				// ACT
				text = get_text(*fl, columns);

				// ACT / ASSERT
				string reference2[][2] = {
					{	"00001000", "100",	},
					{	"00001010", "1000",	},
					{	"00001020", "900",	},
					{	"00001030", "90",	},
				};

				assert_equivalent(mkvector(reference2), text);
			}


			test( FunctionListCanBeSortedByThreadID )
			{
				// INIT
				unsigned columns[] = {	columns::times_called,	};
				invalidation_tracer ih;
				auto functions = plural
					+ make_statistics(addr(0x10000u, 0), 1, 0, 0, 0, 0)
					+ make_statistics(addr(0x10000u, 1), 2, 0, 0, 0, 0)
					+ make_statistics(addr(0x10000u, 2), 3, 0, 0, 0, 0)
					+ make_statistics(addr(0x10000u, 3), 4, 0, 0, 0, 0)
					+ make_statistics(addr(0x10000u, 4), 5, 0, 0, 0, 0);

				assign(*statistics, functions);

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

				tmodel->add(0, 18, string());
				tmodel->add(1, 1, string());
				tmodel->add(2, 180, string());
				tmodel->add(3, 179, string());
				tmodel->add(4, 17900, string());

				ih.bind_to_model(*fl);

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, true);
				auto text = get_text(*fl, columns);

				// ASSERT
				string reference1[][1] = {	{	"2",	}, {	"1",	}, {	"4",	}, {	"3",	}, {	"5",	},	};

				assert_equivalent(mkvector(reference1), text);
				assert_equal(plural + (size_t)5u, ih.counts);

				// ACT (times called, ascending)
				fl->set_order(columns::threadid, false);

				// ASSERT
				string reference2[][1] = {	{	"5",	}, {	"3",	},{	"4",	},{	"1",	},  {	"2",	}, 	};

				assert_equivalent(mkvector(reference2), text);
				assert_equal(plural + (size_t)5u + (size_t)5u, ih.counts);
			}


			test( FunctionListPrintItsContent )
			{
				// INIT
				auto functions = plural
					+ make_statistics(addr(1990), 15, 0, 31, 29, 2)
					+ make_statistics(addr(2000), 35, 1, 453, 366, 3)
					+ make_statistics(addr(2990), 2, 2, 33450030, 32333333, 4);

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);
				string result;

				// ACT
				fl->print(result);

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
					"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n", result);

				// INIT
				assign(*statistics, functions);
				statistics->invalidate();

				// ACT
				fl->set_order(columns::times_called, true);
				fl->print(result);

				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(dp2cl("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										"00000BAE\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t2\t4\r\n"
										"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"), result);

				// ACT
				fl->set_order(columns::exclusive_avg, true);
				fl->print(result);

				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(dp2cl("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"
										"00000BAE\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t2\t4\r\n"), result);
			}


			test( TrackableIsUsableOnReleasingModel )
			{
				// INIT
				assign(*statistics, plural
					+ make_statistics(addr(0x2001), 11, 0, 0, 0, 0)
					+ make_statistics(addr(0x2004), 17, 0, 0, 0, 0)
					+ make_statistics(addr(0x2008), 18, 0, 0, 0, 0));

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

				// ACT
				shared_ptr<const trackable> t(fl->track(1));

				fl.reset();

				// ACT / ASSERT
				assert_equal(trackable::npos(), t->index());
			}


			test( OnlyAllowedItemsAreExposedByTheModelAfterFilterApplication )
			{
				// INIT
				assign(*statistics, plural
					+ make_statistics(addr(0x1000u, 0), 1, 0, 0, 0, 0) + make_statistics(addr(0x1010u, 0), 2, 0, 0, 0, 0)
					+ make_statistics(addr(0x1020u, 2), 3, 0, 0, 0, 0) + make_statistics(addr(0x1030u, 2), 4, 0, 0, 0, 0)
					+ make_statistics(addr(0x1040u, 3), 5, 0, 0, 0, 0) + make_statistics(addr(0x1050u, 3), 6, 0, 0, 0, 0));

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);

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
				assign(*statistics, plural
					+ make_statistics(addr(0x1000u), 1, 0, 0, 0, 0) + make_statistics(addr(0x1010u), 2, 0, 0, 0, 0));

				auto fl = make_shared<functions_list>(statistics, 1, resolver, tmodel);
				invalidation_tracer it;

				it.bind_to_model(*fl);

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
