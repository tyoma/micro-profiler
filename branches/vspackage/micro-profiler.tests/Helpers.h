#pragma once

#include <wpl/mt/thread.h>
#include <algorithm>
#include <string>
#include <tchar.h>

typedef struct FunctionStatisticsTag FunctionStatistics;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

namespace micro_profiler
{
	struct function_statistics;
	struct function_statistics_detailed;

	namespace tests
	{
		struct running_thread
		{
			virtual ~running_thread() throw() { }
			virtual void join() = 0;
			virtual wpl::mt::thread::id get_id() const = 0;
			virtual bool is_running() const = 0;
			
			virtual void suspend() = 0;
			virtual void resume() = 0;
		};

		int get_current_process_id();

		namespace this_thread
		{
			void sleep_for(unsigned int duration);
			wpl::mt::thread::id get_id();
			std::shared_ptr<running_thread> open();
		};

		class image : private std::shared_ptr<void>
		{
			std::wstring _fullpath;

		public:
			explicit image(const TCHAR *path);

			const void *load_address() const;
			const wchar_t *absolute_path() const;
			const void *get_symbol_address(const char *name) const;
		};


		bool less_fs(const FunctionStatistics &lhs, const FunctionStatistics &rhs);
		bool less_fsd(const FunctionStatisticsDetailed &lhs, const FunctionStatisticsDetailed &rhs);
		bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
		function_statistics_detailed function_statistics_ex(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time);


		template <typename T, size_t size>
		inline T *end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}

		inline void toupper(std::wstring &s)
		{	std::transform(s.begin(), s.end(), s.begin(), &towupper);	}
	}
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (AssertFailedException ^) { throw; } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
