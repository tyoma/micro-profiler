#include <frontend/function_list.h>
#include <com_helpers.h>

#include <functional>
#include <list>
#include <utility>
#include <string>
#include <sstream>
#include <cmath>

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
		const wchar_t* max_reentrance)
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
	}

	class myhandler
	{
	public:
		typedef listview::model::index_type index_type;
		typedef std::list<index_type> counter;

		void bind2(listview::model &to) { _connection = to.invalidated += bind(&myhandler::handle, this, _1); }

		size_t times() const { _counter.size(); }

		counter::const_iterator begin() const { return _counter.begin(); }
		counter::const_iterator end() const { return _counter.end(); }
	private:
		void handle(index_type count)
		{
			_counter.push_back(count);
		}
		
		wpl::slot_connection _connection;
		counter _counter;
	};

	class sri : public symbol_resolver_itf
	{
	public:
		virtual std::wstring symbol_name_by_va(const void *address)
		{
			return to_string(address);
		}

	};

}


namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class FunctinoListTests
		{
		public: 

			[TestMethod]
			void CanCreateEmptyFunctinoList()
			{
				sri resolver;
				functions_list fl(test_ticks_resolution, resolver);

				Assert::IsTrue(fl.get_count() == 0);
			}

			[TestMethod]
			void FunctinoListAcceptsUpdates()
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
				sri resolver;
				functions_list fl(test_ticks_resolution, resolver);
				fl.update(data, 2);

				// ASSERT
				Assert::IsTrue(fl.get_count() == 2);
			}

			[TestMethod]
			void FunctinoListCanBeClearedAndUsedAgain()
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
				sri resolver;
				functions_list fl(test_ticks_resolution, resolver);
				fl.update(&ms1, 1);

				Assert::IsTrue(fl.get_count() == 1);

				// ACT & ASSERT
				fl.clear();
				Assert::IsTrue(fl.get_count() == 0);

				// ACT & ASSERT
				fl.update(&ms1, 1);

				Assert::IsTrue(fl.get_count() == 1);
			}
			
			[TestMethod]
			void FunctinoListCollectsUpdates()
			{
				//TODO: add 2 entries of same function in one burst
				// INIT
				function_statistics_detailed s1, s2, s3, s4;
				FunctionStatisticsDetailed ms1, ms2, ms3, ms4;
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

				s4.times_called = 15;
				s4.max_reentrance = 1024;
				s4.inclusive_time = 1011;
				s4.exclusive_time = 723;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)1123, s3), ms3, dummy_children_buffer);
				copy(std::make_pair((void *)5555, s4), ms4, dummy_children_buffer);

				FunctionStatisticsDetailed data1[] = {ms1, ms2};
				FunctionStatisticsDetailed data2[] = {ms3, ms4};

				sri resolver;
				functions_list fl(test_ticks_resolution, resolver);

				// ACT & ASSERT
				fl.update(data1, 2);
				Assert::IsTrue(fl.get_count() == 2);
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */

				assert_row(fl, 0, L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0");
				assert_row(fl, 1, L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3");

				// ACT & ASSERT
				fl.update(data2, 2);
				Assert::IsTrue(fl.get_count() == 3);
				assert_row(fl, 0, L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0");
				assert_row(fl, 1, L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3");
				assert_row(fl, 2, L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024");
			}

			//myhandler h;
			//h.bind2(fl);

			[TestMethod]
			void FunctinoListTextFormatter()
			{
				// INIT
				function_statistics_detailed s1, s2, s3, s4, s5, s6;
				FunctionStatisticsDetailed ms1, ms2, ms3, ms4, ms5, ms6;
				std::vector<FunctionStatistics> dummy_children_buffer;
				// ns
				s1.times_called = 1;
				s1.max_reentrance = 0;
				s1.inclusive_time = 31;
				s1.exclusive_time = 29;
				// us
				s2.times_called = 1;
				s2.max_reentrance = 0;
				s2.inclusive_time = 45340;
				s2.exclusive_time = 36666;
				// ms
				s3.times_called = 1;
				s3.max_reentrance = 0;
				s3.inclusive_time = 33450030;
				s3.exclusive_time = 32333333;
				// s
				s4.times_called = 1;
				s4.max_reentrance = 0;
				s4.inclusive_time = 65450031030;
				s4.exclusive_time = 23470030000;
				// > 1000 s
				s5.times_called = 1;
				s5.max_reentrance = 0;
				s5.inclusive_time = 65450031030567;
				s5.exclusive_time = 23470030000987;
				// > 10000 s
				s6.times_called = 1;
				s6.max_reentrance = 0;
				s6.inclusive_time = 65450031030567000;
				s6.exclusive_time = 23470030000987000;

				copy(std::make_pair((void *)1123, s1), ms1, dummy_children_buffer);
				copy(std::make_pair((void *)2234, s2), ms2, dummy_children_buffer);
				copy(std::make_pair((void *)3123, s3), ms3, dummy_children_buffer);
				copy(std::make_pair((void *)5555, s4), ms4, dummy_children_buffer);
				copy(std::make_pair((void *)4555, s5), ms5, dummy_children_buffer);
				copy(std::make_pair((void *)6666, s6), ms6, dummy_children_buffer);

				FunctionStatisticsDetailed data1[] = {ms1, ms2, ms3, ms4, ms5, ms6};

				sri resolver;
				functions_list fl(10000000000, resolver); // 10 * billion for ticks resolution

				// ACT & ASSERT
				fl.update(data1, 6);
				Assert::IsTrue(fl.get_count() == 6);
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */

				assert_row(fl, 0, L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0");
				assert_row(fl, 1, L"000008B5", L"1", L"4.53us", L"3.67us", L"4.53us", L"3.67us", L"0");
				assert_row(fl, 2, L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0");
				assert_row(fl, 3, L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0");
				assert_row(fl, 4, L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0");
				assert_row(fl, 5, L"00001A05", L"1", L"6.55e+006s", L"2.35e+006s", L"6.55e+006s", L"2.35e+006s", L"0");
			}
		};


	} // namespace tests
} // namespace micro_profiler
