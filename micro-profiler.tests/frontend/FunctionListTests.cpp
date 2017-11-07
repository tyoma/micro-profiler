#include <frontend/function_list.h>

#include "../Helpers.h"

#include <common/fb_helpers.h>
#include <frontend/symbol_resolver.h>

#include <functional>
#include <list>
#include <utility>
#include <string>
#include <sstream>
#include <cmath>
#include <map>
#include <memory>
#include <locale>
#include <algorithm>
#include <ut/assert.h>
#include <ut/test.h>

namespace std 
{
	using namespace tr1;
	using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace tests
	{
		namespace 
		{
			__int64 test_ticks_resolution = 1;

			template <typename T>
			wstring to_string(const T &value)
			{
				wstringstream s;
				s << value;
				return s.str();
			}

			void assert_row(const listview::model &fl, size_t row, const wchar_t* name, const wchar_t* times_called)
			{
				wstring result;

				fl.get_text(row, 0, result); // row number
				assert_equal(result, to_string(row + 1));
				fl.get_text(row, 1, result); // name
				assert_equal(result, name);
				fl.get_text(row, 2, result); // times called
				assert_equal(result, times_called);
			}

			void assert_row(
				const listview::model &fl, 
				size_t row, 
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
				assert_equal(result, exclusive_time);
				fl.get_text(row, 4, result); // inclusive time
				assert_equal(result, inclusive_time);
				fl.get_text(row, 5, result); // avg. exclusive time
				assert_equal(result, avg_exclusive_time);
				fl.get_text(row, 6, result); // avg. inclusive time
				assert_equal(result, avg_inclusive_time);
				fl.get_text(row, 7, result); // max reentrance
				assert_equal(result, max_reentrance);
				fl.get_text(row, 8, result); // max reentrance
				assert_equal(result, max_call_time);
			}

			class invalidation_tracer
			{
				wpl::slot_connection _connection;

				void on_invalidate(listview::model::index_type count)
				{	invalidations.push_back(count);	}

			public:
				typedef vector<listview::model::index_type> _invalidations_log_t;

			public:
				void bind_to_model(listview::model &to)
				{	_connection = to.invalidated += bind(&invalidation_tracer::on_invalidate, this, _1);	}

				_invalidations_log_t invalidations;
			};


			class invalidation_at_sorting_check1
			{
				const listview::model &_model;

				const invalidation_at_sorting_check1 &operator =(const invalidation_at_sorting_check1 &);

			public:
				invalidation_at_sorting_check1(listview::model &m)
					: _model(m)
				{	}

				void operator ()(listview::model::index_type /*count*/) const
				{
					assert_row(_model, 0, L"00002995", L"700");
					assert_row(_model, 1, L"00003001", L"30");
					assert_row(_model, 2, L"00002978", L"3");
				}
			};


			class sri : public symbol_resolver
			{
				mutable map<const void *, wstring> _names;

			public:
				virtual const wstring &symbol_name_by_va(const void *address) const
				{
					return _names[address] = to_string(address);
				}

				virtual void add_image(const wchar_t * /*image*/, const void * /*base*/)
				{
				}
			};

			// convert decimal point to current(default) locale
			wstring dp2cl(const wchar_t *input, wchar_t stub_char = L'.')
			{
				static wchar_t decimal_point = use_facet< numpunct<wchar_t> > (locale("")).decimal_point();

				wstring result = input;
				replace(result.begin(), result.end(), stub_char, decimal_point);
				return result;
			}

			size_t find_row(const listview::model &m, const wstring &name)
			{
				wstring result;

				for (listview::index_type i = 0, c = m.get_count(); i != c; ++i)
					if (m.get_text(i, 1, result), result == name)
						return i;
				return (size_t)-1;
			}
		}


		begin_test_suite( FunctionListTests )
			flatbuffers::FlatBufferBuilder fbuilder;

			test( CanCreateEmptyFunctionList )
			{
				// INIT / ACT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> sp_fl(functions_list::create(test_ticks_resolution, resolver));
				functions_list &fl = *sp_fl;

				// ACT / ASSERT
				assert_equal(0u, fl.get_count());
			}


			test( FunctionListAcceptsUpdates )
			{
				// INIT
				FunctionStatistics s[] = {
					FunctionStatistics(1123, 19, 0, 31, 29, 0),
					FunctionStatistics(2234, 10, 3, 7, 5, 0),
				};
				shared_ptr<symbol_resolver> resolver(new sri);

				// ACT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				// ASSERT
				assert_equal(2u, fl->get_count());
			}


			test( FunctionListCanBeClearedAndUsedAgain )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				const UpdateStatisticsPayload &data = CreateUpdateStatisticsPayload2(fbuilder, 1123, 19, 0, 31, 29, 0);

				// ACT
				invalidation_tracer ih;
				ih.bind_to_model(*fl);
				fl->update(data);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(1u, ih.invalidations.size());
				assert_equal(1u, ih.invalidations.back()); //check what's coming as event arg

				// ACT
				shared_ptr<const listview::trackable> first = fl->track(0); // 2229

				// ASSERT
				assert_equal(0u, first->index());

				// ACT
				fl->clear();

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal(2u, ih.invalidations.size());
				assert_equal(0u, ih.invalidations.back()); //check what's coming as event arg
				assert_equal(listview::npos, first->index());

				// ACT
				fl->update(data);

				// ASSERT
				assert_equal(1u, fl->get_count());
				assert_equal(3u, ih.invalidations.size());
				assert_equal(1u, ih.invalidations.back()); //check what's coming as event arg
				assert_equal(0u, first->index()); // kind of side effect
			}


			test( FunctionListGetByAddress )
			{
				// INIT
				FunctionStatistics s[] = {
					FunctionStatistics(1118, 19, 0, 31, 29, 0),
					FunctionStatistics(2229, 10, 3, 7, 5, 0),
					FunctionStatistics(5550, 5, 0, 10, 7, 0),
				};
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				vector<functions_list::index_type> expected;

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				functions_list::index_type idx1118 = fl->get_index((void *)1118);
				functions_list::index_type idx2229 = fl->get_index((void *)2229);
				functions_list::index_type idx5550 = fl->get_index((void *)5550);

				expected.push_back(idx1118);
				expected.push_back(idx2229);
				expected.push_back(idx5550);
				sort(expected.begin(), expected.end());
				
				// ASSERT
				assert_equal(3u, fl->get_count());

				for (size_t i = 0; i < expected.size(); ++i)
					assert_equal(expected[i], i);

				assert_not_equal(listview::npos, idx1118);
				assert_not_equal(listview::npos, idx2229);
				assert_not_equal(listview::npos, idx5550);
				assert_equal(listview::npos, fl->get_index((void *)1234));

				//Check twice. Kind of regularity check.
				assert_equal(fl->get_index((void *)1118), idx1118);
				assert_equal(fl->get_index((void *)2229), idx2229);
				assert_equal(fl->get_index((void *)5550), idx5550);
			}


			test( FunctionListCollectsUpdates )
			{
				//TODO: add 2 entries of same function in one burst
				//TODO: possibly trackable on update tests should see that it works with every sorting given.

				// INIT
				FunctionStatistics s1[] = {
					FunctionStatistics(1118, 19, 0, 31, 29, 3),
					FunctionStatistics(2229, 10, 3, 7, 5, 4),
				};
				FunctionStatistics s2[] = {
					FunctionStatistics(1118, 5, 0, 10, 7, 6),
					FunctionStatistics(5550, 15, 1024, 1011, 723, 215),
				};
				FunctionStatistics s3[] = { FunctionStatistics(1118, 1, 0, 4, 4, 4), };

				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				fl->set_order(2, true); // by times called
				
				invalidation_tracer ih;
				ih.bind_to_model(*fl);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1));
				shared_ptr<const listview::trackable> first = fl->track(0); // 2229
				shared_ptr<const listview::trackable> second = fl->track(1); // 1118

				// ASSERT
				assert_equal(2u, fl->get_count());
				assert_equal(1u, ih.invalidations.size());
				assert_equal(2u, ih.invalidations.back()); //check what's coming as event arg
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */
				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0", L"3s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");

				assert_equal(0u, first->index());
				assert_equal(1u, second->index());

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s2));
				
				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(2u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0", L"6s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // kind of moved down

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s3));
				
				// ASSERT
				assert_equal(3u, fl->get_count());
				assert_equal(3u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"25", L"45s", L"40s", L"1.8s", L"1.6s", L"0", L"6s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				assert_equal(0u, first->index());
				assert_equal(2u, second->index()); // stand still
			}


			test( FunctionListTimeFormatter )
			{
				// INIT
				// ~ ns
				FunctionStatistics s1(1118, 1, 0, 31, 29, 29);
				FunctionStatistics s1ub(1990, 1, 0, 9994, 9994, 9994);

				// >= 1us
				FunctionStatistics s2lb(2000, 1, 0, 9996, 9996, 9996);
				FunctionStatistics s2(2229, 1, 0, 45340, 36666, 36666);
				FunctionStatistics s2ub(2990, 1, 0, 9994000, 9994000, 9994000);

				// >= 1ms
				FunctionStatistics s3lb(3000, 1, 0, 9996000, 9996000, 9996000);
				FunctionStatistics s3(3118, 1, 0, 33450030, 32333333, 32333333);
				FunctionStatistics s3ub(3990, 1, 0, 9994000000, 9994000000, 9994000000);

				// >= 1s
				FunctionStatistics s4lb(4000, 1, 0, 9996000000, 9996000000, 9996000000);
				FunctionStatistics s4(5550, 1, 0, 65450031030, 23470030000, 23470030000);
				FunctionStatistics s4ub(4990, 1, 0, 9994000000000, 9994000000000, 9994000000000);

				// >= 1000s
				FunctionStatistics s5lb(5000, 1, 0, 9996000000000, 9996000000000, 9996000000000);
				FunctionStatistics s5(4550, 1, 0, 65450031030567, 23470030000987, 23470030000987);
				FunctionStatistics s5ub(5990, 1, 0, 99990031030567, 99990030000987, 99990030000987);
				
				// >= 10000s
				FunctionStatistics s6lb(6000, 1, 0, 99999031030567, 99999030000987, 99999030000987);
				FunctionStatistics s6(6661, 1, 0, 65450031030567000, 23470030000987000, 23470030000987000);

				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(10000000000, resolver)); // 10 * billion for ticks resolution

				FunctionStatistics s[] = {
					s1,
					s2,
					s3,
					s4,
					s5,
					s6,

					s1ub,
					s2lb,
					s2ub,
					s3lb,
					s3ub,
					s4lb,
					s4ub,
					s5lb,
					s5ub,
					s6lb,
				};
				
				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				// ASSERT
				assert_equal(fl->get_count(), array_size(s));
		
				// columns: name, times called, inclusive time, exclusive time, avg. inclusive time, avg. exclusive time, max reentrance, max call time
				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0", L"2.9ns");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"1", L"4.53\x03bcs", L"3.67\x03bcs", L"4.53\x03bcs", L"3.67\x03bcs", L"0", L"3.67\x03bcs");
				assert_row(*fl, fl->get_index((void *)3118), L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0", L"3.23ms");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0", L"2.35s");
				assert_row(*fl, fl->get_index((void *)4550), L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0", L"2347s");
				assert_row(*fl, fl->get_index((void *)6661), L"00001A05", L"1", L"6.55e+006s", L"2.35e+006s", L"6.55e+006s", L"2.35e+006s", L"0", L"2.35e+006s");
				
				// ASSERT (boundary cases)
				assert_row(*fl, fl->get_index((void *)1990), L"000007C6", L"1", L"999ns", L"999ns", L"999ns", L"999ns", L"0", L"999ns");
				assert_row(*fl, fl->get_index((void *)2000), L"000007D0", L"1", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"1\x03bcs", L"0", L"1\x03bcs");
				assert_row(*fl, fl->get_index((void *)2990), L"00000BAE", L"1", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"999\x03bcs", L"0", L"999\x03bcs");
				assert_row(*fl, fl->get_index((void *)3000), L"00000BB8", L"1", L"1ms", L"1ms", L"1ms", L"1ms", L"0", L"1ms");
				assert_row(*fl, fl->get_index((void *)3990), L"00000F96", L"1", L"999ms", L"999ms", L"999ms", L"999ms", L"0", L"999ms");
				assert_row(*fl, fl->get_index((void *)4000), L"00000FA0", L"1", L"1s", L"1s", L"1s", L"1s", L"0", L"1s");
				assert_row(*fl, fl->get_index((void *)4990), L"0000137E", L"1", L"999s", L"999s", L"999s", L"999s", L"0", L"999s");
				assert_row(*fl, fl->get_index((void *)5000), L"00001388", L"1", L"999.6s", L"999.6s", L"999.6s", L"999.6s", L"0", L"999.6s");
				assert_row(*fl, fl->get_index((void *)5990), L"00001766", L"1", L"9999s", L"9999s", L"9999s", L"9999s", L"0", L"9999s");
				assert_row(*fl, fl->get_index((void *)6000), L"00001770", L"1", L"1e+004s", L"1e+004s", L"1e+004s", L"1e+004s", L"0", L"1e+004s");
			}


			test( FunctionListSorting )
			{
				// INIT
				FunctionStatistics s[] = {
					FunctionStatistics(1990, 15, 0, 31, 29, 3),
					FunctionStatistics(2000, 35, 1, 453, 366, 4),
					FunctionStatistics(2990, 2, 2, 33450030, 32333333, 5),
					FunctionStatistics(3000, 15233, 3, 65450, 13470, 6),
				};
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				invalidation_tracer ih;

				ih.bind_to_model(*fl);

				const size_t data_size = array_size(s);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				shared_ptr<const listview::trackable> pt0 = fl->track(fl->get_index((void *)1990));
				shared_ptr<const listview::trackable> pt1 = fl->track(fl->get_index((void *)2000));
				shared_ptr<const listview::trackable> pt2 = fl->track(fl->get_index((void *)2990));
				shared_ptr<const listview::trackable> pt3 = fl->track(fl->get_index((void *)3000));
				
				const listview::trackable &t0 = *pt0;
				const listview::trackable &t1 = *pt1;
				const listview::trackable &t2 = *pt2;
				const listview::trackable &t3 = *pt3;

				// ACT (times called, ascending)
				fl->set_order(2, true);
				
				// ASSERT
				assert_equal(2u, ih.invalidations.size());
				assert_equal(data_size, ih.invalidations.back()); //check what's coming as event arg

				assert_row(*fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); // s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); // s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); // s4

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
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4
				
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
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

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
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				assert_equal(3u, t0.index());
				assert_equal(2u, t1.index());
				assert_equal(1u, t2.index());
				assert_equal(0u, t3.index());
			}

			test( FunctionListPrintItsContent )
			{
				// INIT
				FunctionStatistics s[] = {
					FunctionStatistics(1990, 15, 0, 31, 29, 2),
					FunctionStatistics(2000, 35, 1, 453, 366, 3),
					FunctionStatistics(2990, 2, 2, 33450030, 32333333, 4),
				};
				const size_t data_size = array_size(s);
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				wstring result;

				// ACT
				fl->print(result);

				// ASSERT
				assert_equal(0u, fl->get_count());
				assert_equal(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
					L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n", result);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));
				fl->set_order(2, true); // by times called
				fl->print(result);

				// ASSERT
				assert_equal(data_size, fl->get_count());
				assert_equal(dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"
										L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"), result);

				// ACT
				fl->set_order(5, true); // avg. exclusive time
				fl->print(result);

				// ASSERT
				assert_equal(data_size, fl->get_count());
				assert_equal(dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"
										L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"), result);
			}


			test( FailOnGettingChildrenListFromEmptyRootList )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));

				// ACT / ASSERT
				assert_throws(fl->watch_children(0), out_of_range);
			}


			test( FailOnGettingChildrenListFromNonEmptyRootList )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_resolution, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_resolution, resolver));
				
				FunctionStatistics s1[] = {
					FunctionStatistics(1973, 0, 0, 0, 0, 0),
					FunctionStatistics(1990, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s2[] = {
					FunctionStatistics(1996, 0, 0, 0, 0, 0),
					FunctionStatistics(1999, 0, 0, 0, 0, 0),
					FunctionStatistics(2006, 0, 0, 0, 0, 0),
				};

				fl1->update(CreateUpdateStatisticsPayload2(fbuilder, s1));
				fl2->update(CreateUpdateStatisticsPayload2(fbuilder, s2));

				// ACT / ASSERT
				assert_throws(fl1->watch_children(2), out_of_range);
				assert_throws(fl1->watch_children(20), out_of_range);
				assert_throws(fl1->watch_children((size_t)-1), out_of_range);
				assert_throws(fl2->watch_children(3), out_of_range);
				assert_throws(fl2->watch_children(30), out_of_range);
				assert_throws(fl2->watch_children((size_t)-1), out_of_range);
			}


			test( ReturnChildrenModelForAValidRecord )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_resolution, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(1973, 0, 0, 0, 0, 0),
					FunctionStatistics(1990, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s2[] = {
					FunctionStatistics(1996, 0, 0, 0, 0, 0),
					FunctionStatistics(1999, 0, 0, 0, 0, 0),
					FunctionStatistics(2006, 0, 0, 0, 0, 0),
				};

				fl1->update(CreateUpdateStatisticsPayload2(fbuilder, s1));
				fl2->update(CreateUpdateStatisticsPayload2(fbuilder, s2));

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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(1973, 0, 0, 0, 0, 0),
					FunctionStatistics(1990, 0, 0, 0, 0, 0),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				// ACT
				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				// ASSERT
				assert_not_null(ls);
				assert_equal(0u, ls->get_count());
			}


			test( childrenstatisticsfornonemptychildren )
			{
				// init
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(0x1978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x1995, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x2001, 0, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = {
					FunctionStatistics(0x2004, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2008, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2011, 0, 0, 0, 0, 0),
				};
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s, s_children));
				
				// act
				shared_ptr<linked_statistics> ls_0 = fl->watch_children(find_row(*fl, L"00001978"));
				shared_ptr<linked_statistics> ls_1 = fl->watch_children(find_row(*fl, L"00001995"));

				// act / assert
				assert_equal(1u, ls_0->get_count());
				assert_not_equal((size_t)-1, find_row(*ls_0, L"00002001"));
				assert_equal(3u, ls_1->get_count());
				assert_not_equal((size_t)-1, find_row(*ls_1, L"00002004"));
				assert_not_equal((size_t)-1, find_row(*ls_1, L"00002008"));
				assert_not_equal((size_t)-1, find_row(*ls_1, L"00002011"));
			}


			test( ChildrenStatisticsSorting )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s_children1[] = {
					FunctionStatistics(0x2001, 11, 0, 0, 0, 0),
				};
				FunctionStatistics s_children2[] = {
					FunctionStatistics(0x2001, 11, 0, 0, 0, 0),
					FunctionStatistics(0x2004, 17, 0, 0, 0, 0),
					FunctionStatistics(0x2008, 18, 0, 0, 0, 0),
					FunctionStatistics(0x2011, 29, 0, 0, 0, 0),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children1));
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children2));
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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(10, resolver));
				FunctionStatistics s_children[] = {
					FunctionStatistics(0x2001, 11, 0, 1, 7, 91),
					FunctionStatistics(0x2004, 17, 5, 2, 8, 97),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children));
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);
				ls->set_order(1, true);

				// ACT / ASSERT
				assert_row(*ls, 0, L"00002001", L"11", L"100ms", L"700ms", L"9.09ms", L"63.6ms", L"0", L"9.1s");
				assert_row(*ls, 1, L"00002004", L"17", L"200ms", L"800ms", L"11.8ms", L"47.1ms", L"5", L"9.7s");
			}


			test( IncomingDetailStatisticsUpdateNoChildrenStatisticsUpdatesScenarios )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(1, resolver));
				invalidation_tracer t;
				FunctionStatistics s_children[] = {
					FunctionStatistics(0x2001, 11, 0, 1, 7, 91),
					FunctionStatistics(0x2004, 17, 5, 2, 8, 97),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children));

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				t.bind_to_model(*ls);

				// ACT (update with no children)
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0));

				// ASSERT
				assert_is_empty(t.invalidations);

				// ACT (update with children, but another entry)
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x2978, 0, 0, 0, 0, 0, s_children));

				// ASSERT
				assert_is_empty(t.invalidations);
			}


			test( IncomingDetailStatisticsUpdatenoChildrenStatisticsUpdatesScenarios )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(1, resolver));
				invalidation_tracer t;
				FunctionStatistics s_children1[] = {
					FunctionStatistics(0x2001, 11, 0, 1, 7, 91),
					FunctionStatistics(0x2004, 17, 5, 2, 8, 97),
				};
				FunctionStatistics s_children2[] = {
					FunctionStatistics(0x2004, 17, 5, 2, 8, 97),
					FunctionStatistics(0x2007, 17, 5, 2, 8, 97),
				};
				const UpdateStatisticsPayload &data = CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children1);

				fl->update(data);
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				t.bind_to_model(*ls);

				// ACT
				fl->update(data);

				// ASSERT
				assert_equal(1u, t.invalidations.size());
				assert_equal(2u, t.invalidations.back());

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children2));

				// ASSERT
				assert_equal(2u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());

				// INIT
				FunctionStatistics s_children3[] = {
					FunctionStatistics(0x2001, 11, 0, 1, 7, 91),
				};

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children3));

				// ASSERT
				assert_equal(3u, t.invalidations.size());
				assert_equal(3u, t.invalidations.back());
			}


			test( GetFunctionAddressFromLinkedChildrenStatistics )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s_children[] = {
					FunctionStatistics(0x2001, 11, 0, 0, 0, 0),
					FunctionStatistics(0x2004, 17, 0, 0, 0, 0),
					FunctionStatistics(0x2008, 18, 0, 0, 0, 0),
					FunctionStatistics(0x2011, 29, 0, 0, 0, 0),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x1978, 0, 0, 0, 0, 0, s_children));
				fl->set_order(1, true);

				shared_ptr<linked_statistics> ls = fl->watch_children(0);

				ls->set_order(1, true);

				// ACT / ASSERT
				assert_equal((void *)0x2001, ls->get_address(0));
				assert_equal((void *)0x2004, ls->get_address(1));
				assert_equal((void *)0x2008, ls->get_address(2));
				assert_equal((void *)0x2011, ls->get_address(3));
			}


			test( TrackableIsUsableOnReleasingModel )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(0x2001, 11, 0, 0, 0, 0),
					FunctionStatistics(0x2004, 17, 0, 0, 0, 0),
					FunctionStatistics(0x2008, 18, 0, 0, 0, 0),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s));

				// ACT
				shared_ptr<const listview::trackable> t(fl->track(1));
			
				fl = shared_ptr<functions_list>();

				// ACT / ASSERT
				assert_equal((listview::index_type)-1, t->index());
			}


			test( FailOnGettingParentsListFromNonEmptyRootList )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_resolution, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(1973, 0, 0, 0, 0, 0),
					FunctionStatistics(1990, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s2[] = {
					FunctionStatistics(1996, 0, 0, 0, 0, 0),
					FunctionStatistics(1999, 0, 0, 0, 0, 0),
					FunctionStatistics(2006, 0, 0, 0, 0, 0),
				};

				fl1->update(CreateUpdateStatisticsPayload2(fbuilder, s1));
				fl2->update(CreateUpdateStatisticsPayload2(fbuilder, s2));

				// ACT / ASSERT
				assert_throws(fl1->watch_parents(2), out_of_range);
				assert_throws(fl1->watch_parents(20), out_of_range);
				assert_throws(fl1->watch_parents((size_t)-1), out_of_range);
				assert_throws(fl2->watch_parents(3), out_of_range);
				assert_throws(fl2->watch_parents(30), out_of_range);
				assert_throws(fl2->watch_parents((size_t)-1), out_of_range);
			}


			test( ReturnParentsModelForAValidRecord )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl1(functions_list::create(test_ticks_resolution, resolver));
				shared_ptr<functions_list> fl2(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(1973, 0, 0, 0, 0, 0),
					FunctionStatistics(1990, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s2[] = {
					FunctionStatistics(1996, 0, 0, 0, 0, 0),
					FunctionStatistics(1999, 0, 0, 0, 0, 0),
					FunctionStatistics(2006, 0, 0, 0, 0, 0),
				};

				fl1->update(CreateUpdateStatisticsPayload2(fbuilder, s1));
				fl2->update(CreateUpdateStatisticsPayload2(fbuilder, s2));

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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(2973, 0, 0, 0, 0, 0),
					FunctionStatistics(2990, 0, 0, 0, 0, 0),
					FunctionStatistics(2996, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(2996, 0, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(2996, 0, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = {
					FunctionStatistics(2990, 0, 0, 0, 0, 0),
					FunctionStatistics(2996, 0, 0, 0, 0, 0),
				};
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->set_order(1, true);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s, s_children));

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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(2973, 0, 0, 0, 0, 0),
					FunctionStatistics(2990, 0, 0, 0, 0, 0),
					FunctionStatistics(2996, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s2[] = { FunctionStatistics(2997, 0, 0, 0, 0, 0), };
				FunctionStatistics s_children[] = { FunctionStatistics(2973, 0, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children1[] = {
					mkvector(s_children),
					mkvector(s_children),
					mkvector(s_children),
				};
				vector<FunctionStatistics> s_children2[] = {
					mkvector(s_children),
				};

				fl->set_order(1, true);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1, s_children1));

				shared_ptr<linked_statistics> p = fl->watch_parents(0);

				// ASSERT
				assert_equal(3u, p->get_count());

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s2, s_children2));

				// ASSERT
				assert_equal(4u, p->get_count());
			}


			test( ParentStatisticsValuesAreFormatted )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(0x122F, 1, 0, 0, 0, 0),
					FunctionStatistics(0x2340, 3, 0, 0, 0, 0),
					FunctionStatistics(0x3451, 5000000000, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x2340, 3, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(0x3451, 5000000000, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = { FunctionStatistics(0x122F, 1, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s, s_children));

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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x3001, 3, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(0x3001, 700, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = { FunctionStatistics(0x3001, 30, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->set_order(1, true);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s, s_children));

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
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children[] = { FunctionStatistics(0x3001, 3, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children1[] = {
					mkvector(s_children),
					mkvector(s_children),
					mkvector(s_children),
				};
				invalidation_tracer t;

				fl->set_order(1, true);
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1, s_children1));
				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				t.bind_to_model(*p);

				// ACT
				p->set_order(1, true);

				// ASSERT
				assert_equal(1u, t.invalidations.size());
				assert_equal(3u, t.invalidations[0]);

				// INIT
				FunctionStatistics s2[] = {
					FunctionStatistics(0x3002, 0, 0, 0, 0, 0),
				};
				vector<FunctionStatistics> s_children2[] = {
					mkvector(s_children),
				};

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s2, s_children2));
				t.invalidations.clear();

				// ACT
				p->set_order(2, false);

				// ASSERT
				assert_equal(1u, t.invalidations.size());
				assert_equal(4u, t.invalidations[0]);
			}


			test( ParentStatisticsCausesInvalidationAfterTheSort )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x3001, 3, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(0x3001, 700, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = { FunctionStatistics(0x3001, 30, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->set_order(1, true);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s, s_children));

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				wpl::slot_connection c = p->invalidated += invalidation_at_sorting_check1(*p);

				// ACT / ASSERT
				p->set_order(2, false);
			}


			test( ParentStatisticsIsUpdatedOnGlobalUpdates2 )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x3001, 3, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(0x3001, 30, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = { FunctionStatistics(0x3001, 50, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children_1[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->set_order(1, true);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1, s_children_1));

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				// pre-ASSERT
				assert_row(*p, 0, L"00002978", L"3");
				assert_row(*p, 1, L"00002995", L"30");
				assert_row(*p, 2, L"00003001", L"50");

				// INIT
				FunctionStatistics s2[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
				};
				vector<FunctionStatistics> s_children_2[] = {
					mkvector(s_children1),
					mkvector(s_children2),
				};

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s2, s_children_2));

				// ASSERT
				assert_row(*p, 0, L"00002978", L"6");
				assert_row(*p, 1, L"00003001", L"50");
				assert_row(*p, 2, L"00002995", L"60");
			}


			test( ParentStatisticsInvalidationOnGlobalUpdates )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children[] = { FunctionStatistics(0x2978, 3, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children1[] = {
					mkvector(s_children),
					mkvector(s_children),
					mkvector(s_children),
				};
				invalidation_tracer ih;

				fl->set_order(1, true);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1, s_children1));

				shared_ptr<linked_statistics> p = fl->watch_parents(0);

				ih.bind_to_model(*p);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x7270, 0, 0, 0, 0, 0));

				// ASSERT
				assert_is_empty(ih.invalidations);

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x2978, 0, 0, 0, 0, 0, s_children));

				// ASSERT
				assert_equal(1u, ih.invalidations.size());
				assert_equal(3u, ih.invalidations[0]);

				// INIT
				ih.invalidations.clear();

				// ACT
				fl->update(CreateUpdateStatisticsPayload2(fbuilder, 0x8520, 0, 0, 0, 0, 0, s_children));

				// ASSERT
				assert_equal(1u, ih.invalidations.size());
				assert_equal(4u, ih.invalidations[0]);
			}


			test( GettingAddressOfParentStatisticsItem )
			{
				// INIT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				FunctionStatistics s1[] = {
					FunctionStatistics(0x2978, 0, 0, 0, 0, 0),
					FunctionStatistics(0x2995, 0, 0, 0, 0, 0),
					FunctionStatistics(0x3001, 0, 0, 0, 0, 0),
				};
				FunctionStatistics s_children1[] = { FunctionStatistics(0x3001, 3, 0, 0, 0, 0), };
				FunctionStatistics s_children2[] = { FunctionStatistics(0x3001, 30, 0, 0, 0, 0), };
				FunctionStatistics s_children3[] = { FunctionStatistics(0x3001, 50, 0, 0, 0, 0), };
				vector<FunctionStatistics> s_children[] = {
					mkvector(s_children1),
					mkvector(s_children2),
					mkvector(s_children3),
				};

				fl->set_order(1, true);

				fl->update(CreateUpdateStatisticsPayload2(fbuilder, s1, s_children));

				shared_ptr<linked_statistics> p = fl->watch_parents(2);

				p->set_order(2, true);

				// ACT / ASSERT
				assert_equal((void *)0x2978, p->get_address(0));
				assert_equal((void *)0x2995, p->get_address(1));
				assert_equal((void *)0x3001, p->get_address(2));
			}
		end_test_suite
	}
}
