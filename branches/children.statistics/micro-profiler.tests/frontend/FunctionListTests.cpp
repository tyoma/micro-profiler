#include <frontend/function_list.h>
#include <frontend/symbol_resolver.h>
#include <com_helpers.h>

#include <functional>
#include <list>
#include <utility>
#include <string>
#include <sstream>
#include <cmath>
#include <memory>
#include <locale>
#include <algorithm>

namespace std 
{
	using namespace std::tr1;
	using namespace std::tr1::placeholders;
}

using namespace wpl::ui;
using namespace micro_profiler;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace 
{
	using namespace std;

	__int64 test_ticks_resolution = 1;

	template <typename T>
	wstring to_string(const T &value)
	{
		wstringstream s;
		s << value;
		return s.str();
	}

	void assert_row(
		const functions_list& fl, 
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
		std::wstring result;
		result.reserve(1000);
		fl.get_text(row, 0, result); // row number
		Assert::IsTrue(result == to_string(row));
		fl.get_text(row, 1, result); // name
		Assert::IsTrue(result == name);
		fl.get_text(row, 2, result); // times called
		Assert::IsTrue(result == times_called);
		fl.get_text(row, 3, result); // exclusive time
		Assert::IsTrue(result == exclusive_time);
		fl.get_text(row, 4, result); // inclusive time
		Assert::IsTrue(result == inclusive_time);
		fl.get_text(row, 5, result); // avg. exclusive time
		Assert::IsTrue(result == avg_exclusive_time);
		fl.get_text(row, 6, result); // avg. inclusive time
		Assert::IsTrue(result == avg_inclusive_time);
		fl.get_text(row, 7, result); // max reentrance
		Assert::IsTrue(result == max_reentrance);
		fl.get_text(row, 8, result); // max reentrance
		Assert::IsTrue(result == max_call_time);
	}

	class i_handler
	{
	public:
		typedef listview::model::index_type index_type;
		typedef std::list<index_type> counter;

		void bind2(listview::model &to) { _connection = to.invalidated += bind(&i_handler::handle, this, _1); }

		size_t times() const { return _counter.size(); }

		counter::const_iterator begin() const { return _counter.begin(); }
		counter::const_iterator end() const { return _counter.end(); }

		counter::const_reverse_iterator rbegin() {return _counter.rbegin(); }
		counter::const_reverse_iterator rend() {return _counter.rend(); }
	private:
		void handle(index_type count)
		{
			_counter.push_back(count);
		}
		
		wpl::slot_connection _connection;
		counter _counter;
	};

	class sri : public symbol_resolver
	{
	public:
		virtual std::wstring symbol_name_by_va(const void *address) const
		{
			return to_string(address);
		}
	};

	// convert decimal point to current(default) locale
	std::wstring dp2cl(const wchar_t *input, wchar_t stub_char = L'.')
	{
		static wchar_t decimal_point = use_facet< numpunct<wchar_t> > (locale("")).decimal_point();

		std::wstring result = input;
		std::replace(result.begin(), result.end(), stub_char, decimal_point);
		return result;
	}
}

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class FunctionListTests
		{
		public: 
			[TestMethod]
			void CanCreateEmptyFunctionList()
			{
				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl;

				Assert::IsTrue(fl.get_count() == 0);
			}


			[TestMethod]
			void FunctionListAcceptsUpdates()
			{
				// INIT
				function_statistics_detailed s1, s2;
				FunctionStatisticsDetailed ms1, ms2;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 19;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;

				s2.times_called = 10;
				s2.max_reentrance = 3;
				s2.inclusive_time = 7;
				s2.exclusive_time = 5;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);

				FunctionStatisticsDetailed data[] = {ms1, ms2};
				
				// ACT
				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl;
				fl.update(data, 2);

				// ASSERT
				Assert::IsTrue(fl.get_count() == 2);
			}


			[TestMethod]
			void FunctionListCanBeClearedAndUsedAgain()
			{
				// INIT
				function_statistics_detailed s1;
				FunctionStatisticsDetailed ms1;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 19;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);

				// ACT & ASSERT
				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl;
				i_handler ih;
				ih.bind2(fl);
				fl.update(&ms1, 1);

				Assert::IsTrue(fl.get_count() == 1);
				Assert::IsTrue(ih.times() == 1);
				Assert::IsTrue(*ih.rbegin() == 1); //check what's coming as event arg

				std::shared_ptr<const listview::trackable> first = fl.track(0); // 2229

				Assert::IsTrue(first->index() == 0);
				// ACT & ASSERT
				fl.clear();
				Assert::IsTrue(fl.get_count() == 0);
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == 0); //check what's coming as event arg

				Assert::IsTrue(first->index() == functions_list::npos);
				// ACT & ASSERT
				fl.update(&ms1, 1);

				Assert::IsTrue(fl.get_count() == 1);
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == 1); //check what's coming as event arg

				Assert::IsTrue(first->index() == 0); // kind of side effect
			}


			[TestMethod]
			void FunctionListGetByAddress()
			{
				// INIT
				function_statistics_detailed s1, s2, s3;
				FunctionStatisticsDetailed ms1, ms2, ms3;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 19;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;

				s2.times_called = 10;
				s2.max_reentrance = 3;
				s2.inclusive_time = 7;
				s2.exclusive_time = 5;

				s3.times_called = 5;
				s3.max_reentrance = 0;
				s3.inclusive_time = 10;
				s3.exclusive_time = 7;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)5555, s3), ms3, dummy_children_buffer);

				FunctionStatisticsDetailed data[] = {ms1, ms2, ms3};

				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl;

				std::vector<functions_list::index_type> expected;
				
				// ACT
				fl.update(data, 3);

				functions_list::index_type idx1118 = fl.get_index((void *)1118);
				functions_list::index_type idx2229 = fl.get_index((void *)2229);
				functions_list::index_type idx5550 = fl.get_index((void *)5550);

				expected.push_back(idx1118);
				expected.push_back(idx2229);
				expected.push_back(idx5550);
				std::sort(expected.begin(), expected.end());
				
				// ASSERT
				Assert::IsTrue(fl.get_count() == 3);

				for (size_t i = 0; i < expected.size(); ++i)
					Assert::IsTrue(expected[i] == i);

				Assert::IsTrue(idx1118 != functions_list::npos);
				Assert::IsTrue(idx2229 != functions_list::npos);
				Assert::IsTrue(idx5550 != functions_list::npos);
				Assert::IsTrue(fl.get_index((void *)1234) == functions_list::npos);

				//Check twice. Kind of regularity check.
				Assert::IsTrue(fl.get_index((void *)1118) == idx1118);
				Assert::IsTrue(fl.get_index((void *)2229) == idx2229);
				Assert::IsTrue(fl.get_index((void *)5550) == idx5550);
			}


			[TestMethod]
			void FunctionListCollectsUpdates()
			{
				//TODO: add 2 entries of same function in one burst
				//TODO: possibly trackable on update tests should see that it works with every sorting given.

				// INIT
				function_statistics_detailed s1, s2, s3, s4, s5;
				FunctionStatisticsDetailed ms1, ms2, ms3, ms4, ms5;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 19;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;
				s1.max_call_time = 3;

				s2.times_called = 10;
				s2.max_reentrance = 3;
				s2.inclusive_time = 7;
				s2.exclusive_time = 5;
				s2.max_call_time = 4;

				s3.times_called = 5;
				s3.max_reentrance = 0;
				s3.inclusive_time = 10;
				s3.exclusive_time = 7;
				s3.max_call_time = 6;

				s4.times_called = 15;
				s4.max_reentrance = 1024;
				s4.inclusive_time = 1011;
				s4.exclusive_time = 723;
				s4.max_call_time = 215;

				s5.times_called = 1;
				s5.max_reentrance = 0;
				s5.inclusive_time = 4;
				s5.exclusive_time = 4;
				s5.max_call_time = 4;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)1123, s3), ms3, dummy_children_buffer);
				copy(std::make_pair((void *)5555, s4), ms4, dummy_children_buffer);
				copy(std::make_pair((void *)1123, s5), ms5, dummy_children_buffer);

				FunctionStatisticsDetailed data1[] = {ms1, ms2};
				FunctionStatisticsDetailed data2[] = {ms3, ms4};
				FunctionStatisticsDetailed data3[] = {ms5};

				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl;
				fl.set_order(2, true); // by times called
				
				i_handler ih;
				ih.bind2(fl);

				// ACT
				fl.update(data1, 2);
				std::shared_ptr<const listview::trackable> first = fl.track(0); // 2229
				std::shared_ptr<const listview::trackable> second = fl.track(1); // 1118

				// ASSERT
				Assert::IsTrue(fl.get_count() == 2);
				Assert::IsTrue(ih.times() == 1);
				Assert::IsTrue(*ih.rbegin() == 2); //check what's coming as event arg
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */
				assert_row(fl, fl.get_index((void *)1118), L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0", L"3s");
				assert_row(fl, fl.get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 1);

				// ACT
				fl.update(data2, 2);
				
				// ASSERT
				Assert::IsTrue(fl.get_count() == 3);
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == 3); //check what's coming as event arg

				assert_row(fl, fl.get_index((void *)1118), L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0", L"6s");
				assert_row(fl, fl.get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(fl, fl.get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 2); // kind of moved down

				// ACT
				fl.update(data3, 1);
				
				// ASSERT
				Assert::IsTrue(fl.get_count() == 3);
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == 3); //check what's coming as event arg

				assert_row(fl, fl.get_index((void *)1118), L"0000045E", L"25", L"45s", L"40s", L"1.8s", L"1.6s", L"0", L"6s");
				assert_row(fl, fl.get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(fl, fl.get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 2); // stand still
			}


			[TestMethod]
			void FunctionListTimeFormatter()
			{
				// INIT
				function_statistics_detailed s1, s2, s3, s4, s5, s6, s1ub, s2lb, s2ub, s3lb, s3ub, s4lb, s4ub, s5lb, s5ub, s6lb;
				FunctionStatisticsDetailed ms1, ms2, ms3, ms4, ms5, ms6, ms1ub, ms2lb, ms2ub, ms3lb, ms3ub, ms4lb, ms4ub, ms5lb, ms5ub, ms6lb;
				std::vector<FunctionStatistics> dummy_children_buffer;
				// ns
				s1.times_called = 1;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;
				s1.max_call_time = 29;

				s1ub.times_called = 1;
				s1ub.max_reentrance = 0;
				s1ub.inclusive_time = 9994;
				s1ub.exclusive_time = 9994;
				s1ub.max_call_time = 9994;
				// us
				s2lb.times_called = 1;
				s2lb.max_reentrance = 0;
				s2lb.inclusive_time = 9996;
				s2lb.exclusive_time = 9996;
				s2lb.max_call_time = 9996;

				s2.times_called = 1;
				s2.max_reentrance = 0;
				s2.inclusive_time = 45340;
				s2.exclusive_time = 36666;
				s2.max_call_time = 36666;

				s2ub.times_called = 1;
				s2ub.max_reentrance = 0;
				s2ub.inclusive_time = 9994000;
				s2ub.exclusive_time = 9994000;
				s2ub.max_call_time = 9994000;
				// ms
				s3lb.times_called = 1;
				s3lb.max_reentrance = 0;
				s3lb.inclusive_time = 9996000;
				s3lb.exclusive_time = 9996000;
				s3lb.max_call_time = 9996000;

				s3.times_called = 1;
				s3.max_reentrance = 0;
				s3.inclusive_time = 33450030;
				s3.exclusive_time = 32333333;
				s3.max_call_time = 32333333;

				s3ub.times_called = 1;
				s3ub.max_reentrance = 0;
				s3ub.inclusive_time = 9994000000;
				s3ub.exclusive_time = 9994000000;
				s3ub.max_call_time = 9994000000;
				// s
				s4lb.times_called = 1;
				s4lb.max_reentrance = 0;
				s4lb.inclusive_time = 9996000000;
				s4lb.exclusive_time = 9996000000;
				s4lb.max_call_time = 9996000000;

				s4.times_called = 1;
				s4.max_reentrance = 0;
				s4.inclusive_time = 65450031030;
				s4.exclusive_time = 23470030000;
				s4.max_call_time = 23470030000;

				s4ub.times_called = 1;
				s4ub.max_reentrance = 0;
				s4ub.inclusive_time = 9994000000000;
				s4ub.exclusive_time = 9994000000000;
				s4ub.max_call_time = 9994000000000;
				// > 1000 s
				s5lb.times_called = 1;
				s5lb.max_reentrance = 0;
				s5lb.inclusive_time = 9996000000000;
				s5lb.exclusive_time = 9996000000000;
				s5lb.max_call_time = 9996000000000;

				s5.times_called = 1;
				s5.max_reentrance = 0;
				s5.inclusive_time = 65450031030567;
				s5.exclusive_time = 23470030000987;
				s5.max_call_time = 23470030000987;

				s5ub.times_called = 1;
				s5ub.max_reentrance = 0;
				s5ub.inclusive_time = 99990031030567;
				s5ub.exclusive_time = 99990030000987;
				s5ub.max_call_time = 99990030000987;
				// > 10000 s
				s6lb.times_called = 1;
				s6lb.max_reentrance = 0;
				s6lb.inclusive_time = 99999031030567;
				s6lb.exclusive_time = 99999030000987;
				s6lb.max_call_time = 99999030000987;

				s6.times_called = 1;
				s6.max_reentrance = 0;
				s6.inclusive_time = 65450031030567000;
				s6.exclusive_time = 23470030000987000;
				s6.max_call_time = 23470030000987000;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)3123, s3), ms3, dummy_children_buffer);
				copy(std::make_pair((void *)5555, s4), ms4, dummy_children_buffer);
				copy(std::make_pair((void *)4555, s5), ms5, dummy_children_buffer);
				copy(std::make_pair((void *)6666, s6), ms6, dummy_children_buffer);

				copy(std::make_pair((void *)1995, s1ub), ms1ub, dummy_children_buffer);
				copy(std::make_pair((void *)2005, s2lb), ms2lb, dummy_children_buffer);
				copy(std::make_pair((void *)2995, s2ub), ms2ub, dummy_children_buffer);
				copy(std::make_pair((void *)3005, s3lb), ms3lb, dummy_children_buffer);
				copy(std::make_pair((void *)3995, s3ub), ms3ub, dummy_children_buffer);
				copy(std::make_pair((void *)4005, s4lb), ms4lb, dummy_children_buffer);
				copy(std::make_pair((void *)4995, s4ub), ms4ub, dummy_children_buffer);
				copy(std::make_pair((void *)5005, s5lb), ms5lb, dummy_children_buffer);
				copy(std::make_pair((void *)5995, s5ub), ms5ub, dummy_children_buffer);
				copy(std::make_pair((void *)6005, s6lb), ms6lb, dummy_children_buffer);

				FunctionStatisticsDetailed data[] = {ms1, ms2, ms3, ms4, ms5, ms6, ms1ub, ms2lb, ms2ub, ms3lb, ms3ub, ms4lb, ms4ub, ms5lb, ms5ub, ms6lb};

				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(10000000000, resolver); // 10 * billion for ticks resolution
				functions_list& fl = *sp_fl; 

				// ACT
				fl.update(data, sizeof(data)/sizeof(data[0]));

				// ASSERT
				Assert::IsTrue(fl.get_count() == sizeof(data)/sizeof(data[0]));
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */

				assert_row(fl, fl.get_index((void *)1118), L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0", L"2.9ns");
				assert_row(fl, fl.get_index((void *)2229), L"000008B5", L"1", L"4.53us", L"3.67us", L"4.53us", L"3.67us", L"0", L"3.67us");
				assert_row(fl, fl.get_index((void *)3118), L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0", L"3.23ms");
				assert_row(fl, fl.get_index((void *)5550), L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0", L"2.35s");
				assert_row(fl, fl.get_index((void *)4550), L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0", L"2347s");
				assert_row(fl, fl.get_index((void *)6661), L"00001A05", L"1", L"6.55e+006s", L"2.35e+006s", L"6.55e+006s", L"2.35e+006s", L"0", L"2.35e+006s");
				//boundary
				assert_row(fl, fl.get_index((void *)1990), L"000007C6", L"1", L"999ns", L"999ns", L"999ns", L"999ns", L"0", L"999ns");
				assert_row(fl, fl.get_index((void *)2000), L"000007D0", L"1", L"1us", L"1us", L"1us", L"1us", L"0", L"1us");
				assert_row(fl, fl.get_index((void *)2990), L"00000BAE", L"1", L"999us", L"999us", L"999us", L"999us", L"0", L"999us");
				assert_row(fl, fl.get_index((void *)3000), L"00000BB8", L"1", L"1ms", L"1ms", L"1ms", L"1ms", L"0", L"1ms");
				assert_row(fl, fl.get_index((void *)3990), L"00000F96", L"1", L"999ms", L"999ms", L"999ms", L"999ms", L"0", L"999ms");
				assert_row(fl, fl.get_index((void *)4000), L"00000FA0", L"1", L"1s", L"1s", L"1s", L"1s", L"0", L"1s");
				assert_row(fl, fl.get_index((void *)4990), L"0000137E", L"1", L"999s", L"999s", L"999s", L"999s", L"0", L"999s");
				assert_row(fl, fl.get_index((void *)5000), L"00001388", L"1", L"999.6s", L"999.6s", L"999.6s", L"999.6s", L"0", L"999.6s");
				assert_row(fl, fl.get_index((void *)5990), L"00001766", L"1", L"9999s", L"9999s", L"9999s", L"9999s", L"0", L"9999s");
				assert_row(fl, fl.get_index((void *)6000), L"00001770", L"1", L"1e+004s", L"1e+004s", L"1e+004s", L"1e+004s", L"0", L"1e+004s");
			}


			[TestMethod]
			void FunctionListSorting()
			{
				// INIT
				function_statistics_detailed s1, s2, s3, s4;
				FunctionStatisticsDetailed ms1, ms2, ms3, ms4;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 15;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;
				s1.max_call_time = 3;

				s2.times_called = 35;
				s2.max_reentrance = 1;
				s2.inclusive_time = 453;
				s2.exclusive_time = 366;
				s2.max_call_time = 4;

				s3.times_called = 2;
				s3.max_reentrance = 2;
				s3.inclusive_time = 33450030;
				s3.exclusive_time = 32333333;
				s3.max_call_time = 5;

				s4.times_called = 15233;
				s4.max_reentrance = 3;
				s4.inclusive_time = 65450;
				s4.exclusive_time = 13470;
				s4.max_call_time = 6;

				copy(std::make_pair((void *)1995, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2005, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)2995, s3), ms3, dummy_children_buffer);
				copy(std::make_pair((void *)3005, s4), ms4, dummy_children_buffer);

				FunctionStatisticsDetailed data[] = {ms1, ms2, ms3, ms4};

				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl; 
				i_handler ih;
				ih.bind2(fl);

				const size_t data_size = sizeof(data)/sizeof(data[0]);

				fl.update(data, data_size);

				std::shared_ptr<const listview::trackable> pt0 = fl.track(fl.get_index((void *)1990));
				std::shared_ptr<const listview::trackable> pt1 = fl.track(fl.get_index((void *)2000));
				std::shared_ptr<const listview::trackable> pt2 = fl.track(fl.get_index((void *)2990));
				std::shared_ptr<const listview::trackable> pt3 = fl.track(fl.get_index((void *)3000));
				
				const listview::trackable& t0 = *pt0;
				const listview::trackable& t1 = *pt1;
				const listview::trackable& t2 = *pt2;
				const listview::trackable& t3 = *pt3;

				/*========== times called ============*/
				// ACT
				fl.set_order(2, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); // s2
				assert_row(fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); // s3
				assert_row(fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); // s4

				Assert::IsTrue(t0.index() == 1);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 3);

				// ACT
				fl.set_order(2, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 2);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 0);
				/*========== name (after times called to see that sorting in asc direction works) ============*/
				// ACT
				fl.set_order(1, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 4);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4
				
				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT
				fl.set_order(1, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 5);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);
				/*========== exclusive time ============*/
				// ACT
				fl.set_order(3, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 6);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 2);

				// ACT
				fl.set_order(3, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 7);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 1);
				/*========== inclusive time ============*/
				// ACT
				fl.set_order(4, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 8);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 2);

				// ACT
				fl.set_order(4, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 9);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 1);
				/*========== avg. exclusive time ============*/
				// ACT
				fl.set_order(5, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 10);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 1);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 0);

				// ACT
				fl.set_order(5, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 11);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 2);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 3);
				/*========== avg. inclusive time ============*/
				// ACT
				fl.set_order(6, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 12);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 1);

				// ACT
				fl.set_order(6, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 13);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 2);
				/*========== max reentrance ============*/
				// ACT
				fl.set_order(7, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 14);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT
				fl.set_order(7, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 15);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);
				/*========== max call time ============*/
				// ACT
				fl.set_order(8, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 16);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT
				fl.set_order(8, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 17);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);
			}

			[TestMethod] [Ignore]
			void FunctionListPrintItsContent()
			{
				// INIT
				function_statistics_detailed s1, s2, s3;
				FunctionStatisticsDetailed ms1, ms2, ms3;
				std::vector<FunctionStatistics> dummy_children_buffer;

				s1.times_called = 15;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;
				s1.max_call_time = 2;

				s2.times_called = 35;
				s2.max_reentrance = 1;
				s2.inclusive_time = 453;
				s2.exclusive_time = 366;
				s2.max_call_time = 3;

				s3.times_called = 2;
				s3.max_reentrance = 2;
				s3.inclusive_time = 33450030;
				s3.exclusive_time = 32333333;
				s3.max_call_time = 4;

				copy(std::make_pair((void *)1995, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2005, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)2995, s3), ms3, dummy_children_buffer);

				FunctionStatisticsDetailed data[] = {ms1, ms2, ms3};
				const size_t data_size = sizeof(data)/sizeof(data[0]);

				std::shared_ptr<symbol_resolver> resolver(new sri);
				std::shared_ptr<functions_list> sp_fl = functions_list::create(test_ticks_resolution, resolver);
				functions_list& fl = *sp_fl; 

				wstring result;
				// ACT
				fl.print(result);

				// ASSERT
				Assert::IsTrue(fl.get_count() == 0);
				Assert::IsTrue(result == L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										 L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n");
				// ACT
				fl.update(data, data_size);
				fl.set_order(2, true); // by times called
				fl.print(result);

				// ASSERT
				Assert::IsTrue(fl.get_count() == data_size);
				Assert::IsTrue(result == dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										 L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										 L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"
										 L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										 L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"));

				// ACT
				fl.set_order(5, true); // avg. exclusive time
				fl.print(result);

				// ASSERT
				Assert::IsTrue(fl.get_count() == data_size);
				Assert::IsTrue(result == dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
					L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
					L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
					L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"
					L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"));
			}
		};
	} // namespace tests
} // namespace micro_profiler
