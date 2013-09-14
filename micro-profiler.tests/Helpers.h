#pragma once

typedef struct FunctionStatisticsTag FunctionStatistics;
typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

namespace micro_profiler
{
	struct function_statistics;
	struct function_statistics_detailed;

	namespace tests
	{
		struct thread_function
		{
			virtual void operator ()() = 0;
		};


		class thread
		{
		public:
			typedef unsigned int id;

		public:
			explicit thread(thread_function &job, bool suspended = false);
			~thread();

			id get_id() const;
			void resume();

			static void sleep(unsigned int duration);
			static id current_thread_id();

		private:
			thread(const thread &other);
			const thread &operator =(const thread &rhs);

			id _threadid;
			void *_thread;
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
	}
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (AssertFailedException ^) { throw; } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
