#include <frontend/function_list.h>

#include "mocks.h"

#include <test-helpers/helpers.h>

#include <frontend/serialization.h>

#include <iomanip>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <sstream>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace tests
	{
		namespace 
		{
			timestamp_t test_ticks_per_second = 1;
			const double c_tolerance = 0.000001;

			template <typename T>
			wstring to_string(const T &value)
			{
				wstringstream s;
				s << value;
				return s.str();
			}

			wstring get_text(const table_model &fl, unsigned row, unsigned column)
			{
				wstring text;

				return fl.get_text(row, column, text), text;
			}

			void assert_row(const table_model &fl, table_model::index_type row, const wchar_t* name, const wchar_t* times_called)
			{
				wstring result;

				fl.get_text(row, 0, result); // row number
				assert_equal(to_string(row + 1), result);
				fl.get_text(row, 1, result); // name
				assert_equal(name, result);
				fl.get_text(row, 2, result); // times called
				assert_equal(times_called, result);
			}

			void assert_row(
				const table_model &fl, 
				table_model::index_type row, 
				const wchar_t* name, 
				const wchar_t* times_called, 
				const wchar_t* inclusive_time, 
				const wchar_t* exclusive_time, 
				const wchar_t* avg_inclusive_time, 
				const wchar_t* avg_exclusive_time, 
				const wchar_t* max_reentrance,
				const wchar_t* max_call_time = L"")
			{
				wstring result;

				assert_row(fl, row, name, times_called);
				fl.get_text(row, 3, result); // exclusive time
				assert_equal(exclusive_time, result);
				fl.get_text(row, 4, result); // inclusive time
				assert_equal(inclusive_time, result);
				fl.get_text(row, 5, result); // avg. exclusive time
				assert_equal(avg_exclusive_time, result);
				fl.get_text(row, 6, result); // avg. inclusive time
				assert_equal(avg_inclusive_time, result);
				fl.get_text(row, 7, result); // max reentrance
				assert_equal(max_reentrance, result);
				fl.get_text(row, 8, result); // max reentrance
				assert_equal(max_call_time, result);
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
					assert_row(_model, 0, L"00002995", L"700");
					assert_row(_model, 1, L"00003001", L"30");
					assert_row(_model, 2, L"00002978", L"3");
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
					if (m.get_text(i, 1, result), result == name)
						return i;
				return table_model::npos();
			}

			template <typename ArchiveT, typename DataT>
			void emulate_save(ArchiveT &archive, signed long long ticks_per_second, const symbol_resolver &resolver, const DataT &data)
			{
				archive(ticks_per_second);
				archive(resolver);
				archive(data);
			}

			template <typename ArchiveT, typename ContainerT>
			void serialize_single_threaded(ArchiveT &archive, const ContainerT &container, unsigned int threadid = 1u)
			{
				vector< pair< unsigned /*threadid*/, ContainerT > > data(1, make_pair(threadid, container));

				archive(data);
			}
		}


		begin_test_suite( FunctionListTests )

			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			shared_ptr<symbol_resolver> resolver;

			function<void (unsigned persistent_id)> get_requestor()
			{	return [this] (unsigned /*persistent_id*/) { };	}

			FunctionListTests()
				: ser(_buffer), dser(_buffer)
			{	}

			init( CreateResolver )
			{
				resolver.reset(new mocks::symbol_resolver);
			}


			test( CanCreateEmptyFunctionList )
			{
				// INIT / ACT
				shared_ptr<functions_list> sp_fl(functions_list::create(test_ticks_per_second, resolver));
				const functions_list &fl = *sp_fl;

				// ACT / ASSERT
				assert_equal(0u, fl.get_count());
				assert_equal(resolver, fl.get_resolver());
				assert_is_true(fl.updates_enabled);
			}


			test( FunctionListAcceptsUpdates )
			{
				// INIT
				statistics_map_detailed s;

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2234]) = function_statistics(10, 3, 7, 5);
				serialize_single_threaded(ser, s);
				
				// ACT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				dser(*fl);

				// ASSERT
				assert_equal(2u, fl->get_count());
			}


			test( FunctionListDoesNotAcceptsUpdatesIfNotEnabled )
			{
				// INIT
				int invalidated_count = 0;
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				slot_connection conn = fl->invalidated += bind(&increment, &invalidated_count);

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2234]) = function_statistics(10, 3, 7, 5);
				serialize_single_threaded(ser, s);
				
				// ACT
				fl->updates_enabled = false;
				dser(*fl);

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal(0, invalidated_count);
			}


			test( FunctionListCanBeClearedAndUsedAgain )
			{
				// INIT
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));

				static_cast<function_statistics &>(s[1123]) = function_statistics(19, 0, 31, 29);
				serialize_single_threaded(ser, s);

				// ACT
				invalidation_tracer ih;
				ih.bind_to_model(*fl);
				dser(*fl);

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
				dser(*fl);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(3u, ih.invalidations.size());
				assert_equal(1u, ih.invalidations.back()); //check what's coming as event arg
				assert_equal(0u, first->index()); // kind of side effect
			}


			test( ClearingTheListResetsWatchedChildren )
			{
				// INIT
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer it1, it2;

				s[1123].callees[11000];
				s[1123].callees[11001];
				s[1124].callees[11100];
				serialize_single_threaded(ser, s);
				dser(*fl);

				fl->set_order(1, true);

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
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer it1, it2;

				s[1123].callees[11000];
				s[1123].callees[11001];
				s[1124].callees[11000];
				serialize_single_threaded(ser, s);
				dser(*fl);

				fl->set_order(1, true);

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
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				vector<functions_list::index_type> expected;

				static_cast<function_statistics &>(s[1118]) = function_statistics(19, 0, 31, 29);
				static_cast<function_statistics &>(s[2229]) = function_statistics(10, 3, 7, 5);
				static_cast<function_statistics &>(s[5550]) = function_statistics(5, 0, 10, 7);
				serialize_single_threaded(ser, s);
				
				// ACT
				dser(*fl);

				functions_list::index_type idx1118 = fl->get_index(1118);
				functions_list::index_type idx2229 = fl->get_index(2229);
				functions_list::index_type idx5550 = fl->get_index(5550);

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
				assert_equal(table_model::npos(), fl->get_index(1234));

				//Check twice. Kind of regularity check.
				assert_equal(fl->get_index(1118), idx1118);
				assert_equal(fl->get_index(2229), idx2229);
				assert_equal(fl->get_index(5550), idx5550);
			}


			test( FunctionListCollectsUpdates )
			{
				//TODO: add 2 entries of same function in one burst
				//TODO: possibly trackable on update tests should see that it works with every sorting given.

				// INIT
				statistics_map_detailed s1, s2, s3;

				static_cast<function_statistics &>(s1[1118]) = function_statistics(19, 0, 31, 29, 3);
				static_cast<function_statistics &>(s1[2229]) = function_statistics(10, 3, 7, 5, 4);
				static_cast<function_statistics &>(s2[1118]) = function_statistics(5, 0, 10, 7, 6);
				static_cast<function_statistics &>(s2[5550]) = function_statistics(15, 1024, 1011, 723, 215);
				static_cast<function_statistics &>(s3[1118]) = function_statistics(100111222333, 0, 17000, 14000, 4);

				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				fl->set_order(2, true); // by times called
				
				invalidation_tracer ih;
				ih.bind_to_model(*fl);

				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);
				serialize_single_threaded(ser, s3);

				// ACT
				dser(*fl);
				shared_ptr<const trackable> first = fl->track(0); // 2229
				shared_ptr<const trackable> second = fl->track(1); // 1118

				// ASSERT
				assert_equal(2u, fl->get_count());
				assert_equal(1u, ih.invalidations.size());
				assert_equal(2u, ih.invalidations.back()); //check what's coming as event arg
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */
				assert_row(*fl, fl->get_index(1118), L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0", L"3s");
				assert_row(*fl, fl->get_index(2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");

				assert_equal(0u, first->index());
				assert_equal(1u, second->index());

				// ACT
				dser(*fl);
				
				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(2u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, fl->get_index(1118), L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0", L"6s");
				assert_row(*fl, fl->get_index(2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index(5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // kind of moved down

				// ACT
				dser(*fl);
				
				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(3u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, fl->get_index(1118), L"0000045E", L"100111222357", L"1.7e+04s", L"1.4e+04s", L"170ns", L"140ns", L"0", L"6s");
				assert_row(*fl, fl->get_index(2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index(5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // stand still
			}


			test( FunctionListTimeFormatter )
			{
				// INIT
				// ~ ns
				function_statistics s1(1, 0, 31, 29, 29);
				function_statistics s1ub(1, 0, 9994, 9994, 9994);

				// >= 1us
				function_statistics s2lb(1, 0, 9996, 9996, 9996);
				function_statistics s2(1, 0, 45340, 36666, 36666);
				function_statistics s2ub(1, 0, 9994000, 9994000, 9994000);

				// >= 1ms
				function_statistics s3lb(1, 0, 9996000, 9996000, 9996000);
				function_statistics s3(1, 0, 33450030, 32333333, 32333333);
				function_statistics s3ub(1, 0, 9994000000, 9994000000, 9994000000);

				// >= 1s
				function_statistics s4lb(1, 0, 9996000000, 9996000000, 9996000000);
				function_statistics s4(1, 0, 65450031030, 23470030000, 23470030000);
				function_statistics s4ub(1, 0, 9994000000000, 9994000000000, 9994000000000);

				// >= 1000s
				function_statistics s5lb(1, 0, 9996000000000, 9996000000000, 9996000000000);
				function_statistics s5(1, 0, 65450031030567, 23470030000987, 23470030000987);
				function_statistics s5ub(1, 0, 99990031030567, 99990030000987, 99990030000987);
				
				// >= 10000s
				function_statistics s6lb(1, 0, 99999031030567, 99999030000987, 99999030000987);
				function_statistics s6(1, 0, 65450031030567000, 23470030000987000, 23470030000987000);

				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(10000000000, resolver)); // 10 * billion ticks per second

				static_cast<function_statistics &>(s[1118]) = s1;
				static_cast<function_statistics &>(s[2229]) = s2;
				static_cast<function_statistics &>(s[3118]) = s3;
				static_cast<function_statistics &>(s[5550]) = s4;
				static_cast<function_statistics &>(s[4550]) = s5;
				static_cast<function_statistics &>(s[6661]) = s6;

				static_cast<function_statistics &>(s[1990]) = s1ub;
				static_cast<function_statistics &>(s[2000]) = s2lb;
				static_cast<function_statistics &>(s[2990]) = s2ub;
				static_cast<function_statistics &>(s[3000]) = s3lb;
				static_cast<function_statistics &>(s[3990]) = s3ub;
				static_cast<function_statistics &>(s[4000]) = s4lb;
				static_cast<function_statistics &>(s[4990]) = s4ub;
				static_cast<function_statistics &>(s[5000]) = s5lb;
				static_cast<function_statistics &>(s[5990]) = s5ub;
				static_cast<function_statistics &>(s[6000]) = s6lb;
				serialize_single_threaded(ser, s);

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(fl->get_count(), s.size());
		
				// columns: name, times called, inclusive time, exclusive time, avg. inclusive time, avg. exclusive time, max reentrance, max call time
				assert_row(*fl, fl->get_index(1118), L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0", L"2.9ns");
				assert_row(*fl, fl->get_index(2229), L"000008B5", L"1", L"4.53\x03bcs", L"3.67\x03bcs", L"4.53\x03bcs", L"3.67\x03bcs", L"0", L"3.67\x03bcs");
				assert_row(*fl, fl->get_index(3118), L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0", L"3.23ms");
				assert_row(*fl, fl->get_index(5550), L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0", L"2.35s");
				assert_row(*fl, fl->get_index(4550), L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0", L"2347s");
				assert_row(*fl, fl->get_index(6661), L"00001A05", L"1", L"6.55e+06s", L"2.35e+06s", L"6.55e+06s", L"2.35e+06s", L"0", L"2.35e+06s");
				
				// ASSERT (boundary cases)
				assert_row(*fl, fl->get_index(1990), L"000007C6", L"1", L"999ns", L"999ns", L"999ns", L"999ns", L"0", L"999ns");
				assert_row(*fl, fl->get_index(2000), L"000007D0", L"1", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"0", L"1\x03bcs");
				assert_row(*fl, fl->get_index(2990), L"00000BAE", L"1", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"0", L"999\x03bcs");
				assert_row(*fl, fl->get_index(3000), L"00000BB8", L"1", L"1ms", L"1ms", L"1ms", L"1ms", L"0", L"1ms");
				assert_row(*fl, fl->get_index(3990), L"00000F96", L"1", L"999ms", L"999ms", L"999ms", L"999ms", L"0", L"999ms");
				assert_row(*fl, fl->get_index(4000), L"00000FA0", L"1", L"1s", L"1s", L"1s", L"1s", L"0", L"1s");
				assert_row(*fl, fl->get_index(4990), L"0000137E", L"1", L"999s", L"999s", L"999s", L"999s", L"0", L"999s");
				assert_row(*fl, fl->get_index(5000), L"00001388", L"1", L"999.6s", L"999.6s", L"999.6s", L"999.6s", L"0", L"999.6s");
				assert_row(*fl, fl->get_index(5990), L"00001766", L"1", L"9999s", L"9999s", L"9999s", L"9999s", L"0", L"9999s");
				assert_row(*fl, fl->get_index(6000), L"00001770", L"1", L"1e+04s", L"1e+04s", L"1e+04s", L"1e+04s", L"0", L"1e+04s");
			}


			test( FunctionListSorting )
			{
				// INIT
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer ih;

				static_cast<function_statistics &>(s[1990]) = function_statistics(15, 0, 31, 29, 3);
				static_cast<function_statistics &>(s[2000]) = function_statistics(35, 1, 453, 366, 4);
				static_cast<function_statistics &>(s[2990]) = function_statistics(2, 2, 33450030, 32333333, 5);
				static_cast<function_statistics &>(s[3000]) = function_statistics(15233, 3, 65460, 13470, 6);
				serialize_single_threaded(ser, s);

				ih.bind_to_model(*fl);

				const size_t data_size = s.size();

				dser(*fl);

				shared_ptr<const trackable> pt0 = fl->track(fl->get_index(1990));
				shared_ptr<const trackable> pt1 = fl->track(fl->get_index(2000));
				shared_ptr<const trackable> pt2 = fl->track(fl->get_index(2990));
				shared_ptr<const trackable> pt3 = fl->track(fl->get_index(3000));
				
				const trackable &t0 = *pt0;
				const trackable &t1 = *pt1;
				const trackable &t2 = *pt2;
				const trackable &t3 = *pt3;

				// ACT (times called, ascending)
				fl->set_order(2, true);
				
				// ASSERT
				assert_equal(2u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); // s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); // s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); // s4

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (times called, descending)
				fl->set_order(2, false);

				// ASSERT
				assert_equal(3u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (name, ascending; after times called to see that sorting in asc direction works)
				fl->set_order(1, true);

				// ASSERT
				assert_equal(4u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4
				
				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (name, descending)
				fl->set_order(1, false);

				// ASSERT
				assert_equal(5u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (exclusive time, ascending)
				fl->set_order(3, true);

				// ASSERT
				assert_equal(6u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (exclusive time, descending)
				fl->set_order(3, false);

				// ASSERT
				assert_equal(7u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (inclusive time, ascending)
				fl->set_order(4, true);

				// ASSERT
				assert_equal(8u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (inclusive time, descending)
				fl->set_order(4, false);

				// ASSERT
				assert_equal(9u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(1u, t3.index());
				
				// ACT (avg. exclusive time, ascending)
				fl->set_order(5, true);
				
				// ASSERT
				assert_equal(10u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(1u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (avg. exclusive time, descending)
				fl->set_order(5, false);
				
				// ASSERT
				assert_equal(11u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(2u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (avg. inclusive time, ascending)
				fl->set_order(6, true);
				
				// ASSERT
				assert_equal(12u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(0u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(3u, t2.index());
				assert_equal(1u, t3.index());

				// ACT (avg. inclusive time, descending)
				fl->set_order(6, false);
				
				// ASSERT
				assert_equal(13u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(0u, t2.index());
				assert_equal(2u, t3.index());

				// ACT (max reentrance, ascending)
				fl->set_order(7, true);
				
				// ASSERT
				assert_equal(14u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max reentrance, descending)
				fl->set_order(7, false);
				
				// ASSERT
				assert_equal(15u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());

				// ACT (max call time, ascending)
				fl->set_order(8, true);
				
				// ASSERT
				assert_equal(16u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(0u, t0.index());
				assert_equal(1u, t1.index());
				assert_equal(2u, t2.index());
				assert_equal(3u, t3.index());

				// ACT (max call time, descending)
				fl->set_order(8, false);
				
				// ASSERT
				assert_equal(17u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+07s", L"3.23e+07s", L"1.67e+07s", L"1.62e+07s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+04s", L"1.35e+04s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());
			}


			test( FunctionListPrintItsContent )
			{
				// INIT
				statistics_map_detailed s;
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
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
				dser(*fl);
				fl->set_order(2, true); // by times called
				fl->print(result);

				// ASSERT
				assert_equal(s.size(), fl->get_count());
				assert_equal(dp2cl("Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										"00000BAE\t2\t3.23333e+07\t3.345e+07\t1.61667e+07\t1.6725e+07\t2\t4\r\n"
										"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"), result);

				// ACT
				fl->set_order(5, true); // avg. exclusive time
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
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[1978];
				s1[1995];
				s2[2001];
				s2[2004];
				s2[2011];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl1);
				dser(*fl2);

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
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[1978];
				s1[1995];
				s2[2001];
				s2[2004];
				s2[2011];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl1);
				dser(*fl2);

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
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[1973];
				s[1990];
				serialize_single_threaded(ser, s);

				dser(*fl);

				// ACT
				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				// ASSERT
				assert_not_null(ls);
				assert_equal(0u, ls->get_count());
			}


			test( ChildrenStatisticsForNonEmptyChildren )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x1978].callees[0x2001];
				s[0x1995].callees[0x2004];
				s[0x1995].callees[0x2008];
				s[0x1995].callees[0x2011];
				serialize_single_threaded(ser, s);

				dser(*fl);
				
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
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[0x1978].callees[0x2001] = function_statistics(11);
				s2[0x1978].callees[0x2001] = function_statistics(11);
				s2[0x1978].callees[0x2004] = function_statistics(17);
				s2[0x1978].callees[0x2008] = function_statistics(18);
				s2[0x1978].callees[0x2011] = function_statistics(29);
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl);
				dser(*fl);
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				// ACT
				ls->set_order(1, false);

				// ASSERT
				assert_row(*ls, 0, L"00002011", L"29", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 1, L"00002008", L"18", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 2, L"00002004", L"17", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 3, L"00002001", L"22", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");

				// ACT
				ls->set_order(2, true);

				// ASSERT
				assert_row(*ls, 0, L"00002004", L"17", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 1, L"00002008", L"18", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 2, L"00002001", L"22", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
				assert_row(*ls, 3, L"00002011", L"29", L"0s", L"0s", L"0s", L"0s", L"0", L"0s");
			}


			test( ChildrenStatisticsGetText )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(10, resolver));
				statistics_map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);

				dser(*fl);
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);
				ls->set_order(1, true);

				// ACT / ASSERT
				assert_row(*ls, 0, L"00002001", L"11", L"100ms", L"700ms", L"9.09ms", L"63.6ms", L"0", L"9.1s");
				assert_row(*ls, 1, L"00002004", L"17", L"200ms", L"800ms", L"11.8ms", L"47.1ms", L"5", L"9.7s");
			}


			test( IncomingDetailStatisticsUpdateNoLinkedStatisticsAfterClear )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer t1, t2;
				statistics_map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);
				s[0x1978].callees.clear();
				serialize_single_threaded(ser, s);

				dser(*fl);

				shared_ptr<linked_statistics> ls1 = fl->watch_children(0);
				shared_ptr<linked_statistics> ls2 = fl->watch_parents(0);

				fl->clear();
				t1.bind_to_model(*ls1);
				t2.bind_to_model(*ls2);

				// ACT (linked statistics are detached)
				dser(*fl);

				// ASSERT
				assert_is_empty(t1.invalidations);
				assert_is_empty(t2.invalidations);
			}


			test( LinkedStatisticsInvalidatedToEmptyOnMasterDestruction )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer t1, t2;
				statistics_map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s);

				dser(*fl);

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
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer t;
				statistics_map_detailed s1, s2;

				s1[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				s1[0x1978].callees[0x2004] = function_statistics(17, 5, 2, 8, 97);
				s2[0x1978].callees[0x2004] = function_statistics(11, 0, 1, 7, 91);
				s2[0x1978].callees[0x2007] = function_statistics(17, 5, 2, 8, 97);
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				dser(*fl);
				_buffer.rewind();
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				t.bind_to_model(*ls);

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(1u, t.invalidations.size());
				assert_equal(2u, t.invalidations.back());

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(2u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());

				// INIT
				statistics_map_detailed s3;

				s3[0x1978].callees[0x2001] = function_statistics(11, 0, 1, 7, 91);
				serialize_single_threaded(ser, s3);

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(3u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());
			}


			test( GetFunctionAddressFromLinkedChildrenStatistics )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x1978].callees[0x2001] = function_statistics(11);
				s[0x1978].callees[0x2004] = function_statistics(17);
				s[0x1978].callees[0x2008] = function_statistics(18);
				s[0x1978].callees[0x2011] = function_statistics(29);
				serialize_single_threaded(ser, s);

				dser(*fl);
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				ls->set_order(1, true);

				// ACT / ASSERT
				assert_equal(0x2001u, ls->get_address(0));
				assert_equal(0x2004u, ls->get_address(1));
				assert_equal(0x2008u, ls->get_address(2));
				assert_equal(0x2011u, ls->get_address(3));
			}


			test( TrackableIsUsableOnReleasingModel )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				static_cast<function_statistics &>(s[0x2001]) = function_statistics(11);
				static_cast<function_statistics &>(s[0x2004]) = function_statistics(17);
				static_cast<function_statistics &>(s[0x2008]) = function_statistics(18);
				serialize_single_threaded(ser, s);

				dser(*fl);

				// ACT
				shared_ptr<const trackable> t(fl->track(1));

				fl = shared_ptr<functions_list>();

				// ACT / ASSERT
				assert_equal(trackable::npos(), t->index());
			}


			test( FailOnGettingParentsListFromNonEmptyRootList )
			{
				// INIT
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[0x1978];
				s1[0x1995];
				serialize_single_threaded(ser, s1);
				s2[0x2001];
				s2[0x2004];
				s2[0x2008];
				serialize_single_threaded(ser, s2);

				dser(*fl1);
				dser(*fl2);

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
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_per_second, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[0x1978];
				s1[0x1995];
				serialize_single_threaded(ser, s1);
				s2[0x2001];
				s2[0x2004];
				s2[0x2008];
				serialize_single_threaded(ser, s2);

				dser(*fl1);
				dser(*fl2);

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
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[2978].callees[3001];
				s[2995].callees[3001];
				s[3001].callees[2995];
				s[3001].callees[3001];
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				// ACT
				dser(*fl);

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
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s1, s2;

				s1[2978].callees[2978];
				s1[2995].callees[2978];
				s1[3001].callees[2978];
				s2[3002].callees[2978];
				serialize_single_threaded(ser, s1);
				serialize_single_threaded(ser, s2);

				fl->set_order(1, true);

				// ACT
				dser(*fl);

				shared_ptr<linked_statistics> p = fl->watch_parents(0);

				// ASSERT
				assert_equal(3u, p->get_count());

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(4u, p->get_count());
			}


			test( ParentStatisticsValuesAreFormatted )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				static_cast<function_statistics &>(s[0x122F]) = function_statistics(1);
				s[0x122F].callees[0x2340] = function_statistics(3);
				static_cast<function_statistics &>(s[0x2340]) = function_statistics(3);
				s[0x2340].callees[0x3451] = function_statistics(5000000000);
				static_cast<function_statistics &>(s[0x3451]) = function_statistics(5000000000);
				s[0x3451].callees[0x122F] = function_statistics(1);
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				dser(*fl);

				shared_ptr<linked_statistics> p0 = fl->watch_parents(0);
				shared_ptr<linked_statistics> p1 = fl->watch_parents(1);
				shared_ptr<linked_statistics> p2 = fl->watch_parents(2);

				// ACT / ASSERT
				assert_row(*p0, 0, L"00003451", L"1");
				assert_row(*p1, 0, L"0000122F", L"3");
				assert_row(*p2, 0, L"00002340", L"5000000000");
			}


			test( ParentStatisticsSorting )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(700);
				s[0x3001].callees[0x3001] = function_statistics(30);
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				dser(*fl);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				// ACT
				p->set_order(1, true);

				// ASSERT
				assert_row(*p, 0, L"00002978", L"3");
				assert_row(*p, 1, L"00002995", L"700");
				assert_row(*p, 2, L"00003001", L"30");

				// ACT
				p->set_order(1, false);

				// ASSERT
				assert_row(*p, 0, L"00003001", L"30");
				assert_row(*p, 1, L"00002995", L"700");
				assert_row(*p, 2, L"00002978", L"3");

				// ACT
				p->set_order(2, true);

				// ASSERT
				assert_row(*p, 0, L"00002978", L"3");
				assert_row(*p, 1, L"00003001", L"30");
				assert_row(*p, 2, L"00002995", L"700");

				// ACT
				p->set_order(2, false);

				// ASSERT
				assert_row(*p, 0, L"00002995", L"700");
				assert_row(*p, 1, L"00003001", L"30");
				assert_row(*p, 2, L"00002978", L"3");
			}


			test( ParentStatisticsResortingCausesInvalidation )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				invalidation_tracer t;
				statistics_map_detailed s1, s2;

				s1[0x2978].callees[0x3001];
				s1[0x2995].callees[0x3001];
				s1[0x3001].callees[0x3001];
				serialize_single_threaded(ser, s1);
				s2[0x3002].callees[0x3001];
				serialize_single_threaded(ser, s2);

				fl->set_order(1, true);
				dser(*fl);
				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				t.bind_to_model(*p);

				// ACT
				p->set_order(1, true);

				// ASSERT
				table_model::index_type reference1[] = { 3u, };

				assert_equal(reference1, t.invalidations);

				// ACT
				dser(*fl);
				p->set_order(2, false);

				// ASSERT
				table_model::index_type reference2[] = { 3u, 4u, 4u, };

				assert_equal(reference2, t.invalidations);
			}


			test( ParentStatisticsCausesInvalidationAfterTheSort )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(700);
				s[0x3001].callees[0x3001] = function_statistics(30);
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				dser(*fl);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				wpl::slot_connection c = p->invalidated += invalidation_at_sorting_check1(*p);

				// ACT / ASSERT
				p->set_order(2, false);
			}


			test( ParentStatisticsIsUpdatedOnGlobalUpdates2 )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(30);
				s[0x3001].callees[0x3001] = function_statistics(50);
				serialize_single_threaded(ser, s);
				s.erase(0x3001);
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				dser(*fl);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				// pre-ASSERT
				assert_row(*p, 0, L"00002978", L"3");
				assert_row(*p, 1, L"00002995", L"30");
				assert_row(*p, 2, L"00003001", L"50");

				// ACT
				dser(*fl);

				// ASSERT
				assert_row(*p, 0, L"00002978", L"6");
				assert_row(*p, 1, L"00003001", L"50");
				assert_row(*p, 2, L"00002995", L"60");
			}


			test( GettingAddressOfParentStatisticsItem )
			{
				// INIT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_per_second, resolver));
				statistics_map_detailed s;

				s[0x2978].callees[0x3001] = function_statistics(3);
				s[0x2995].callees[0x3001] = function_statistics(30);
				s[0x3001].callees[0x3001] = function_statistics(50);
				serialize_single_threaded(ser, s);

				fl->set_order(1, true);

				dser(*fl);

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				// ACT / ASSERT
				assert_equal(0x2978, p->get_address(0));
				assert_equal(0x2995, p->get_address(1));
				assert_equal(0x3001, p->get_address(2));
			}


			test( SymbolsRequiredAndTicksPerSecondAreSerializedForFile )
			{
				// INIT
				pair<address_t, string> symbols1[] = {
					make_pair(1, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl1(functions_list::create(16, mocks::symbol_resolver::create(symbols1)));
				pair<address_t, string> symbols2[] = {
					make_pair(7, "A"), make_pair(11, "B"), make_pair(19, "C"), make_pair(131, "D"), make_pair(113, "E"),
				};
				shared_ptr<functions_list> fl2(functions_list::create(25000000000, mocks::symbol_resolver::create(symbols2)));
				statistics_map_detailed s;
				timestamp_t ticks_per_second;

				s[1], s[17];
				serialize_single_threaded(ser, s);
				dser(*fl1);

				s.clear();
				s[7], s[11], s[131], s[113];
				serialize_single_threaded(ser, s);
				dser(*fl2);

				// ACT
				fl1->save(ser);

				// ASSERT
				symbol_resolver r(get_requestor());

				dser(ticks_per_second);
				dser(r);
				dser(s);

				assert_equal(16, ticks_per_second);
				assert_equal("Lorem", r.symbol_name_by_va(1));
				assert_equal("Amet", r.symbol_name_by_va(17));

				// ACT
				fl2->save(ser);

				// ASSERT
				dser(ticks_per_second);
				dser(r);
				dser(s);

				assert_equal(25000000000, ticks_per_second);
				assert_equal("A", r.symbol_name_by_va(7));
				assert_equal("B", r.symbol_name_by_va(11));
				assert_equal("D", r.symbol_name_by_va(131));
				assert_equal("E", r.symbol_name_by_va(113));
			}


			test( StatisticsFollowsTheSymbolsInFileSerialization )
			{
				// INIT
				pair<address_t, string> symbols[] = {
					make_pair(1, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl(functions_list::create(1, mocks::symbol_resolver::create(symbols)));
				statistics_map_detailed s;
				timestamp_t ticks_per_second;

				s[1], s[17], s[13];
				serialize_single_threaded(ser, s);
				dser(*fl);

				// ACT
				fl->save(ser);

				// ASSERT
				symbol_resolver r(get_requestor());
				statistics_map_detailed stats_read;

				dser(ticks_per_second);
				dser(r);
				dser(stats_read);

				assert_equal(3u, stats_read.size());
			}


			test( FunctionListIsComletelyRestoredWithSymbols )
			{
				// INIT
				pair<address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				statistics_map_detailed s;

				s[5].times_called = 123, s[17].times_called = 127, s[13].times_called = 12, s[123].times_called = 12000;
				s[5].inclusive_time = 1000, s[123].inclusive_time = 250;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);

				// ACT
				shared_ptr<functions_list> fl = functions_list::load(dser);
				fl->set_order(1, true);

				// ASSERT
				assert_equal(L"Amet", get_text(*fl, 0, 1));
				assert_equal(L"127", get_text(*fl, 0, 2));
				assert_equal(L"Ipsum", get_text(*fl, 1, 1));
				assert_equal(L"12", get_text(*fl, 1, 2));
				assert_equal(L"Lorem", get_text(*fl, 2, 1));
				assert_equal(L"123", get_text(*fl, 2, 2));
				assert_equal(L"2s", get_text(*fl, 2, 4));
				assert_equal(L"dolor", get_text(*fl, 3, 1));
				assert_equal(L"12000", get_text(*fl, 3, 2));
				assert_equal(L"500ms", get_text(*fl, 3, 4));
			}


			test( NonNullPiechartModelIsReturnedFromFunctionsList )
			{
				// INIT
				pair<address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl(functions_list::create(1, mocks::symbol_resolver::create(symbols)));

				// ACT
				shared_ptr< series<double> > m = fl->get_column_series();

				// ASSERT
				assert_equal(0u, m->size());
			}


			test( PiechartModelReturnsConvertedValuesForTimesCalled )
			{
				// INIT
				pair<address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				statistics_map_detailed s;

				s[5].times_called = 123, s[17].times_called = 127, s[13].times_called = 12, s[123].times_called = 12000;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);

				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT
				fl->set_order(2, false);

				// ASSERT
				assert_equal(4u, m->size());
				assert_approx_equal(12000.0, m->get_value(0), c_tolerance);
				assert_approx_equal(127.0, m->get_value(1), c_tolerance);
				assert_approx_equal(123.0, m->get_value(2), c_tolerance);
				assert_approx_equal(12.0, m->get_value(3), c_tolerance);

				// ACT
				fl->set_order(2, true);

				// ASSERT
				assert_equal(4u, m->size());
				assert_approx_equal(12000.0, m->get_value(3), c_tolerance);
				assert_approx_equal(127.0, m->get_value(2), c_tolerance);
				assert_approx_equal(123.0, m->get_value(1), c_tolerance);
				assert_approx_equal(12.0, m->get_value(0), c_tolerance);
			}


			test( FunctionListUpdateLeadsToPiechartModelUpdateAndSignal )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;
				int invalidated_count = 0;

				s[5].times_called = 123, s[17].times_called = 127, s[13].times_called = 12, s[123].times_called = 12000;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				s.clear();

				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				fl->set_order(2, false);

				s[120].times_called = 11001;
				serialize_single_threaded(ser, s);

				slot_connection conn = m->invalidated += bind(&increment, &invalidated_count);

				// ACT
				dser(*fl);

				// ASSERT
				assert_equal(1, invalidated_count);
				assert_equal(5u, m->size());
				assert_approx_equal(11001.0, m->get_value(1), c_tolerance);
				assert_approx_equal(127.0, m->get_value(2), c_tolerance);
				assert_approx_equal(123.0, m->get_value(3), c_tolerance);
				assert_approx_equal(12.0, m->get_value(4), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForExclusiveTime )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;

				s[5].exclusive_time = 13, s[17].exclusive_time = 127, s[13].exclusive_time = 12;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl1 = functions_list::load(dser);
				shared_ptr< series<double> > m1 = fl1->get_column_series();
				
				s[123].exclusive_time = 12000;
				emulate_save(ser, 100, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl2 = functions_list::load(dser);
				shared_ptr< series<double> > m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(3, false);
				fl2->set_order(3, false);

				// ASSERT
				assert_equal(3u, m1->size());
				assert_approx_equal(0.254, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.026, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.024, m1->get_value(2), c_tolerance);
				assert_equal(4u, m2->size());
				assert_approx_equal(120.0, m2->get_value(0), c_tolerance);
				assert_approx_equal(1.27, m2->get_value(1), c_tolerance);
				assert_approx_equal(0.13, m2->get_value(2), c_tolerance);
				assert_approx_equal(0.12, m2->get_value(3), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForTheRestOfTheFields )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;

				s[5].times_called = 100, s[17].times_called = 1000;
				s[5].exclusive_time = 16, s[17].exclusive_time = 130;
				s[5].inclusive_time = 15, s[17].inclusive_time = 120;
				s[5].max_call_time = 14, s[17].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl1 = functions_list::load(dser);
				shared_ptr< series<double> > m1 = fl1->get_column_series();
				
				emulate_save(ser, 1000, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl2 = functions_list::load(dser);
				shared_ptr< series<double> > m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(4, false);
				fl2->set_order(4, true);

				// ASSERT
				assert_approx_equal(0.240, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.030, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.015, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.120, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(5, false);
				fl2->set_order(5, true);

				// ASSERT
				assert_approx_equal(0.00032, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.00026, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.00013, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.00016, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(6, false);
				fl2->set_order(6, true);

				// ASSERT
				assert_approx_equal(0.00030, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.00024, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.00012, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.00015, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(8, false);
				fl2->set_order(8, true);

				// ASSERT
				assert_approx_equal(0.256, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.028, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.014, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.128, m2->get_value(1), c_tolerance);
			}




			test( PiechartModelReturnsZeroesAverageValuesForUncalledFunctions )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;

				s[5].times_called = 0, s[17].times_called = 0;
				s[5].exclusive_time = 16, s[17].exclusive_time = 0;
				s[5].inclusive_time = 15, s[17].inclusive_time = 0;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT
				fl->set_order(5, false);

				// ASSERT
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(1), c_tolerance);

				// ACT
				fl->set_order(6, false);

				// ASSERT
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(1), c_tolerance);
			}


			test( SwitchingSortOrderLeadsToPiechartModelUpdate )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;
				int invalidated_count = 0;

				s[5].times_called = 100, s[17].times_called = 1000;
				s[5].exclusive_time = 16, s[17].exclusive_time = 130;
				s[5].inclusive_time = 15, s[17].inclusive_time = 120;
				s[5].max_call_time = 14, s[17].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr< series<double> > m = fl->get_column_series();
				slot_connection conn = m->invalidated += bind(&increment, &invalidated_count);

				// ACT
				fl->set_order(2, false);

				// ASSERT
				assert_equal(1, invalidated_count);

				// ACT
				fl->set_order(2, true);
				fl->set_order(3, false);
				fl->set_order(4, false);
				fl->set_order(5, false);
				fl->set_order(6, false);
				fl->set_order(8, false);

				// ASSERT
				assert_equal(7, invalidated_count);
			}


			test( PiechartForUnsupportedColumnsIsSimplyZero )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;

				s[5].times_called = 100, s[17].times_called = 1000;
				s[5].exclusive_time = 16, s[17].exclusive_time = 130;
				s[5].inclusive_time = 15, s[17].inclusive_time = 120;
				s[5].max_call_time = 14, s[17].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT / ASSERT
				fl->set_order(2, true);
				fl->set_order(0, true);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
				fl->set_order(2, true);
				fl->set_order(1, false);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
				fl->set_order(2, true);
				fl->set_order(7, true);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
			}


			test( ChildrenStatisticsProvideColumnSeries )
			{
				// INIT
				pair<address_t, string> symbols[] = { make_pair(0, ""), };
				statistics_map_detailed s;
				shared_ptr< series<double> > m;

				s[5].times_called = 2000;
				s[5].callees[11].times_called = 29, s[5].callees[13].times_called = 31;
				s[5].callees[11].exclusive_time = 16, s[5].callees[13].exclusive_time = 10;

				s[17].times_called = 1999;
				s[17].callees[11].times_called = 101, s[17].callees[19].times_called = 103, s[17].callees[23].times_called = 1100;
				s[17].callees[11].exclusive_time = 3, s[17].callees[19].exclusive_time = 112, s[17].callees[23].exclusive_time = 9;

				emulate_save(ser, 100, *mocks::symbol_resolver::create(symbols), s);
				shared_ptr<functions_list> fl = functions_list::load(dser);
				shared_ptr<linked_statistics> ls;

				fl->set_order(2, false);

				// ACT
				ls = fl->watch_children(0);
				ls->set_order(2, true);
				m = ls->get_column_series();

				// ASSERT
				assert_equal(2u, m->size());
				assert_approx_equal(29.0, m->get_value(0), c_tolerance);
				assert_approx_equal(31.0, m->get_value(1), c_tolerance);

				// ACT
				ls = fl->watch_children(1);
				m = ls->get_column_series();
				ls->set_order(3, false);

				// ASSERT
				assert_equal(3u, m->size());
				assert_approx_equal(1.12, m->get_value(0), c_tolerance);
				assert_approx_equal(0.09, m->get_value(1), c_tolerance);
				assert_approx_equal(0.03, m->get_value(2), c_tolerance);
			}
		end_test_suite
	}
}
