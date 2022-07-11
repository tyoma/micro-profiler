#include <frontend/statistic_models.h>
#include <frontend/view_dump.h>

#include "helpers.h"
#include "mocks.h"
#include "primitive_helpers.h"

#include <frontend/columns_layout.h>
#include <frontend/keyer.h>
#include <frontend/models.h>
#include <iomanip>
#include <sstream>
#include <test-helpers/helpers.h>
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
			const main_columns::items name_times_inc_exc_iavg_eavg_reent_minc[] = {
				main_columns::name, main_columns::times_called, main_columns::inclusive, main_columns::exclusive,
				main_columns::inclusive_avg, main_columns::exclusive_avg, main_columns::max_time
			};

			const main_columns::items name_times[] = {	main_columns::name, main_columns::times_called,	};

			const main_columns::items name_threadid[] = {	main_columns::name, main_columns::threadid,	};

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
					if (get_text(m, i, main_columns::name) == name)
						return i;
				return table_model_base::npos();
			}
		}


		begin_test_suite( FunctionListTests )
			std::shared_ptr<tables::statistics> statistics;
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<tables::threads> threads;

			init( CreatePrerequisites )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver = make_shared<mocks::symbol_resolver>(modules, mappings);
				threads = make_shared<tables::threads>();
			}


			test( ConstructedFunctionListIsEmpty )
			{
				// INIT / ACT
				shared_ptr<richtext_table_model> fl1 = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1.0, resolver, threads, false), c_statistics_columns);
				shared_ptr<table_model> fl2 = make_table<table_model>(statistics,
					create_context(statistics, 1.0 , resolver, threads, false), c_statistics_columns);

				// ACT / ASSERT
				assert_equal(0u, fl1->get_count());
				assert_equal(0u, fl2->get_count());
			}


			test( FunctionListAcceptsUpdates )
			{
				// INIT
				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1.0, resolver, threads, false), c_statistics_columns);

				add_records(*statistics, plural
					+ make_call_statistics(1, 1, 0, 1123, 19, 0, 0, 0, 1)
					+ make_call_statistics(2, 1, 0, 2234, 29, 0, 0, 0, 1));

				// ACT / ASSERT
				assert_equal(0u, fl->get_count());

				// ACT
				statistics->invalidate();

				// ASSERT
				assert_equal(2u, fl->get_count());

				// ACT
				add_records(*statistics, plural
					+ make_call_statistics(3, 1, 0, 7234, 10, 1, 1, 1, 2));
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
				unsigned columns[] = {	main_columns::name, main_columns::inclusive, main_columns::exclusive, main_columns::inclusive_avg, main_columns::exclusive_avg, main_columns::max_time,	};

				add_records(*statistics, plural
					// ~ ns
					+ make_call_statistics(1, 1, 0, 0x45Eu, 1, 0, 31, 29, 29)
					+ make_call_statistics(2, 1, 0, 0x7C6u, 1, 0, 9994, 9994, 9994)

					// >= 1us
					+ make_call_statistics(3, 1, 0, 0x7D0u, 1, 0, 9996, 9996, 9996)
					+ make_call_statistics(4, 1, 0, 0x8B5u, 1, 0, 45340, 36666, 36666)
					+ make_call_statistics(5, 1, 0, 0xBAEu, 1, 0, 9994000, 9994000, 9994000)

					// >= 1ms
					+ make_call_statistics(6, 1, 0, 0xBB8u, 1, 0, 9996000, 9996000, 9996000)
					+ make_call_statistics(7, 1, 0, 0xC2Eu, 1, 0, 33450030, 32333333, 32333333)
					+ make_call_statistics(8, 1, 0, 0xF96u, 1, 0, 9994000000, 9994000000, 9994000000)

					// >= 1s
					+ make_call_statistics(9, 1, 0, 0xFA0u, 1, 0, 9996000000, 9996000000, 9996000000)
					+ make_call_statistics(10, 1, 0, 0x15AEu, 1, 0, 65450031030, 23470030000, 23470030000)
					+ make_call_statistics(11, 1, 0, 0x137Eu, 1, 0, 9994000000000, 9994000000000, 9994000000000)

					// >= 1000s
					+ make_call_statistics(12, 1, 0, 0x1388u, 1, 0, 9996000000000, 9996000000000, 9996000000000)
					+ make_call_statistics(13, 1, 0, 0x11C6u, 1, 0, 65450031030567, 23470030000987, 23470030000987)
					+ make_call_statistics(14, 1, 0, 0x1766u, 1, 0, 99990031030567, 99990030000987, 99990030000987)
				
					// >= 10000s
					+ make_call_statistics(15, 1, 0, 0x1770u, 1, 0, 99999031030567, 99999030000987, 99999030000987)
					+ make_call_statistics(16, 1, 0, 0x1A05u, 1, 0, 65450031030567000, 23470030000987000, 23470030000987000));

				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1e-10, resolver, threads, false), c_statistics_columns);

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


			test( HierarchicalTableIsFormedFromHierarchicalData )
			{
				// INIT
				auto statistics_ = make_shared< views::table<call_statistics> >();

				add_records(*statistics_, plural
					+ make_call_statistics(1, 0, 5, 0x00001122, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 0, 3, 0x00001123, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 0, 0, 0x00001124, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 0, 3, 0x00001125, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 0, 0, 0x00001126, 0, 0, 0, 0, 0)
					+ make_call_statistics(6, 0, 2, 0x00001127, 0, 0, 0, 0, 0));

				auto fl = make_table<richtext_table_model>(statistics_,
					create_context(statistics_, 1, resolver, threads, false), c_statistics_columns);
				unsigned columns[] = {	main_columns::name,	};

				// ACT
				auto text = get_text(*fl, columns);

				// ASSERT
				string reference1[][1] = {
					{	"00001124",	},
					{	"    00001123",	},
					{	"        00001127",	},
					{	"    00001125",	},
					{	"00001126",	},
					{	"    00001122",	},
				};

				assert_equal(mkvector(reference1), text);

				// INIT
				auto r = statistics_->create();

				*r = make_call_statistics(7, 0, 2, 0x0000F12F, 0, 0, 0, 0, 0);
				r.commit();

				// ACT
				statistics_->invalidate();
				text = get_text(*fl, columns);

				// ASSERT
				string reference2[][1] = {
					{	"00001124",	},
					{	"    00001123",	},
					{	"        00001127",	},
					{	"        0000F12F",	},
					{	"    00001125",	},
					{	"00001126",	},
					{	"    00001122",	},
				};

				assert_equal(mkvector(reference2), text);
			}


			test( FunctionListSorting )
			{
				// INIT
				invalidation_tracer ih;

				add_records(*statistics, plural
					+ make_call_statistics(1, 1, 0, 1990u, 15, 0, 31, 29, 3)
					+ make_call_statistics(2, 1, 0, 2000u, 35, 1, 453, 366, 4)
					+ make_call_statistics(3, 1, 0, 2990u, 2, 2, 33450030, 32333333, 5)
					+ make_call_statistics(4, 1, 0, 3000u, 15233, 3, 65460, 13470, 6));

				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);

				ih.bind_to_model(*fl);

				shared_ptr<const trackable> pt0 = fl->track(0);
				shared_ptr<const trackable> pt1 = fl->track(1);
				shared_ptr<const trackable> pt2 = fl->track(2);
				shared_ptr<const trackable> pt3 = fl->track(3);

				const trackable &t0 = *pt0;
				const trackable &t1 = *pt1;
				const trackable &t2 = *pt2;
				const trackable &t3 = *pt3;

				// ACT (times called, ascending)
				fl->set_order(main_columns::times_called, true);

				// ASSERT
				assert_equal(1u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

				string reference1[][7] = {
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "5s",	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "3s",	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "4s",	},
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "6s",	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference1, *fl);

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (times called, descending)
				fl->set_order(main_columns::times_called, false);

				// ASSERT
				assert_equal(2u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

				string reference2[][7] = {
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "6s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "4s"	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "3s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "5s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference2, *fl);

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (name, ascending; after times called to see that sorting in asc direction works)
				fl->set_order(main_columns::name, true);

				// ASSERT
				assert_equal(3u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

				string reference3[][7] = {
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "3s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "4s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "5s"	},
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "6s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference3, *fl);

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (name, descending)
				fl->set_order(main_columns::name, false);

				// ASSERT
				assert_equal(4u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

				string reference4[][7] = {
					{	"00000BB8", "15233", "6.55e+04s", "1.35e+04s", "4.3s", "884ms", "6s"	},
					{	"00000BAE", "2", "3.35e+07s", "3.23e+07s", "1.67e+07s", "1.62e+07s", "5s"	},
					{	"000007D0", "35", "453s", "366s", "12.9s", "10.5s", "4s"	},
					{	"000007C6", "15", "31s", "29s", "2.07s", "1.93s", "3s"	},
				};

				assert_table_equal(name_times_inc_exc_iavg_eavg_reent_minc, reference4, *fl);

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (exclusive time, ascending)
				fl->set_order(main_columns::exclusive, true);

				// ASSERT
				assert_equal(5u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::exclusive, false);

				// ASSERT
				assert_equal(6u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::inclusive, true);

				// ASSERT
				assert_equal(7u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::inclusive, false);

				// ASSERT
				assert_equal(8u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::exclusive_avg, true);
				
				// ASSERT
				assert_equal(9u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::exclusive_avg, false);
				
				// ASSERT
				assert_equal(10u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::inclusive_avg, true);
				
				// ASSERT
				assert_equal(11u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::inclusive_avg, false);
				
				// ASSERT
				assert_equal(12u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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

				// ACT (max call time, ascending)
				fl->set_order(main_columns::max_time, true);
				
				// ASSERT
				assert_equal(13u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				fl->set_order(main_columns::max_time, false);
				
				// ASSERT
				assert_equal(14u, ih.counts.size());
				assert_equal(4u, ih.counts.back()); //check what's coming as event arg

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
				unsigned columns[] = {	main_columns::name, main_columns::threadid,	};

				add_records(*statistics, plural
					+ make_call_statistics(1, 3, 0, 0x1000u, 1, 0, 0, 0, 0)
					+ make_call_statistics(1, 2, 0, 0x1010u, 1, 0, 0, 0, 0)
					+ make_call_statistics(1, 7, 0, 0x1020u, 1, 0, 0, 0, 0)
					+ make_call_statistics(1, 9, 0, 0x1030u, 1, 0, 0, 0, 0));

				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);

				add_records(*threads, plural
					+ make_thread_info(3, 100, string())
					+ make_thread_info(2, 1000, string())
					+ make_thread_info(7, 900, string()));

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
				add_records(*threads, plural
					+ make_thread_info(9, 90, string()));

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
				unsigned columns[] = {	main_columns::times_called,	};
				invalidation_tracer ih;

				add_records(*statistics, plural
					+ make_call_statistics(1, 0, 0, 0x10000u, 1, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 0, 0x10000u, 2, 0, 0, 0, 0)
					+ make_call_statistics(3, 2, 0, 0x10000u, 3, 0, 0, 0, 0)
					+ make_call_statistics(4, 3, 0, 0x10000u, 4, 0, 0, 0, 0)
					+ make_call_statistics(5, 4, 0, 0x10000u, 5, 0, 0, 0, 0));

				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);

				add_records(*threads, plural
					+ make_thread_info(0, 18, string())
					+ make_thread_info(1, 1, string())
					+ make_thread_info(2, 180, string())
					+ make_thread_info(3, 179, string())
					+ make_thread_info(4, 17900, string()));

				ih.bind_to_model(*fl);

				// ACT (times called, ascending)
				fl->set_order(main_columns::threadid, true);
				auto text = get_text(*fl, columns);

				// ASSERT
				string reference1[][1] = {	{	"2",	}, {	"1",	}, {	"4",	}, {	"3",	}, {	"5",	},	};

				assert_equivalent(mkvector(reference1), text);
				assert_equal(plural + (size_t)5u, ih.counts);

				// ACT (times called, ascending)
				fl->set_order(main_columns::threadid, false);

				// ASSERT
				string reference2[][1] = {	{	"5",	}, {	"3",	},{	"4",	},{	"1",	},  {	"2",	}, 	};

				assert_equivalent(mkvector(reference2), text);
				assert_equal(plural + (size_t)5u + (size_t)5u, ih.counts);
			}


			test( FunctionListPrintItsContent )
			{
				// INIT
				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);
				string result;

				add_records(*threads, plural
					+ make_thread_info(1, 1711, ""));

				// ACT
				dump::as_tab_separated(result, *fl);

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal("\"#\"" "\t"
					"\"Function\r\nqualified name\"" "\t"
					"\"Thread\r\nid\"" "\t"
					"\"Called\r\ntimes\"" "\t"
					"\"Exclusive\r\ntotal\"" "\t"
					"\"Inclusive\r\ntotal\"" "\t"
					"\"Exclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\nmaximum/call\"" "\r\n", result);

				// INIT
				add_records(*statistics, plural
					+ make_call_statistics(1, 1, 0, 1990, 15, 0, 31, 29, 2)
					+ make_call_statistics(2, 1, 0, 2000, 35, 1, 453, 366, 3)
					+ make_call_statistics(3, 1, 0, 2990, 2, 2, 33450030, 32333333, 4));
				statistics->invalidate();

				// ACT
				fl->set_order(main_columns::times_called, true);
				dump::as_tab_separated(result, *fl);

				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(dp2cl("\"#\"" "\t"
					"\"Function\r\nqualified name\"" "\t"
					"\"Thread\r\nid\"" "\t"
					"\"Called\r\ntimes\"" "\t"
					"\"Exclusive\r\ntotal\"" "\t"
					"\"Inclusive\r\ntotal\"" "\t"
					"\"Exclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\nmaximum/call\"" "\r\n"
					"1\t00000BAE\t1711\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t4\r\n"
					"2\t000007C6\t1711\t15\t29\t31\t1.93333\t2.06667\t2\r\n"
					"3\t000007D0\t1711\t35\t366\t453\t10.4571\t12.9429\t3\r\n"), result);

				// INIT
				add_records(*threads, plural
					+ make_thread_info(1, 1713, ""), keyer::external_id());

				// ACT
				fl->set_order(main_columns::exclusive_avg, true);
				dump::as_tab_separated(result, *fl);

				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(dp2cl("\"#\"" "\t"
					"\"Function\r\nqualified name\"" "\t"
					"\"Thread\r\nid\"" "\t"
					"\"Called\r\ntimes\"" "\t"
					"\"Exclusive\r\ntotal\"" "\t"
					"\"Inclusive\r\ntotal\"" "\t"
					"\"Exclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\naverage/call\"" "\t"
					"\"Inclusive\r\nmaximum/call\"" "\r\n"
					"1\t000007C6\t1713\t15\t29\t31\t1.93333\t2.06667\t2\r\n"
					"2\t000007D0\t1713\t35\t366\t453\t10.4571\t12.9429\t3\r\n"
					"3\t00000BAE\t1713\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t4\r\n"), result);
			}


			test( TrackableIsUsableOnReleasingModel )
			{
				// INIT
				add_records(*statistics, plural
					+ make_call_statistics(1, 1, 0, 0x2001, 11, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 0, 0x2004, 17, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 0, 0x2008, 18, 0, 0, 0, 0));

				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);

				// ACT
				shared_ptr<const trackable> t(fl->track(1));

				fl.reset();

				// ACT / ASSERT
				assert_equal(trackable::npos(), t->index());
			}


			test( SymbolResolverInvalidationIsForwarded )
			{
				// INIT
				auto invalidations = 0;
				auto fl = make_table<richtext_table_model>(statistics,
					create_context(statistics, 1, resolver, threads, false), c_statistics_columns);
				auto c = fl->invalidate += [&] (richtext_table_model::index_type i) {
					invalidations++;
					assert_equal(richtext_table_model::npos(), i);
				};

				// ACT
				resolver->invalidate();

				// ASSERT
				assert_equal(1, invalidations);
			}
		end_test_suite
	}
}
