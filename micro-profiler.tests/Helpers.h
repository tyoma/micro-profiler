#pragma once

#include <_generated/frontend_generated.h>

#include <wpl/mt/thread.h>
#include <algorithm>
#include <string>
#include <tchar.h>
#include <vector>

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

		std::string get_current_process_executable();

		bool is_com_initialized();

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

		template <typename T, size_t size>
		std::vector<T> mkvector(T (&array_ptr)[size])
		{	return std::vector<T>(array_ptr, array_ptr + size);	}

		function_statistics_detailed function_statistics_ex(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time);

		template <typename T, size_t size>
		inline const UpdateStatisticsPayload &CreateUpdateStatisticsPayload2(flatbuffers::FlatBufferBuilder &builder,
			T (&statistics)[size])
		{
			std::vector< flatbuffers::Offset<FunctionStatisticsDetailed> > detailed;

			for (size_t i = 0; i != size; ++i)
				detailed.push_back(CreateFunctionStatisticsDetailed(builder, statistics + i));
			builder.Finish(CreateMessage(builder, Payload_UpdateStatisticsPayload, CreateUpdateStatisticsPayload(builder, builder.CreateVector(detailed)).Union()));
			return *GetMessage(builder.GetBufferPointer())->payload_as<UpdateStatisticsPayload>();
		}

		template <typename T1, typename T2, size_t size>
		inline const UpdateStatisticsPayload &CreateUpdateStatisticsPayload2(flatbuffers::FlatBufferBuilder &builder,
			T1 (&statistics)[size], T2 (&children)[size])
		{
			std::vector< flatbuffers::Offset<FunctionStatisticsDetailed> > detailed;

			for (size_t i = 0; i != size; ++i)
				detailed.push_back(CreateFunctionStatisticsDetailed(builder, statistics + i, builder.CreateVectorOfStructs(children[i])));
			builder.Finish(CreateMessage(builder, Payload_UpdateStatisticsPayload, CreateUpdateStatisticsPayload(builder, builder.CreateVector(detailed)).Union()));
			return *GetMessage(builder.GetBufferPointer())->payload_as<UpdateStatisticsPayload>();
		}

		inline const UpdateStatisticsPayload &CreateUpdateStatisticsPayload2(flatbuffers::FlatBufferBuilder &builder,
			unsigned __int64 address, unsigned __int64 times_called, unsigned __int64 max_reentrance,
			__int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time)
		{
			FunctionStatistics statistics[] = {
				FunctionStatistics(address, times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time),
			};

			return CreateUpdateStatisticsPayload2(builder, statistics);
		}

		template <typename T, size_t size>
		inline const UpdateStatisticsPayload &CreateUpdateStatisticsPayload2(flatbuffers::FlatBufferBuilder &builder,
			unsigned __int64 address, unsigned __int64 times_called, unsigned __int64 max_reentrance,
			__int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time, T (&children_)[size])
		{
			FunctionStatistics statistics[] = {
				FunctionStatistics(address, times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time),
			};
			std::vector<FunctionStatistics> children[] = { mkvector(children_), };

			return CreateUpdateStatisticsPayload2(builder, statistics, children);
		}

		inline const FunctionStatisticsDetailed &CreateFunctionStatisticsDetailed2(flatbuffers::FlatBufferBuilder &builder,
			unsigned __int64 address, unsigned __int64 times_called, unsigned __int64 max_reentrance,
			__int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time)
		{
			return *CreateUpdateStatisticsPayload2(builder, address, times_called, max_reentrance,
				inclusive_time, exclusive_time, max_call_time).statistics()->Get(0);
		}

		template <typename T, size_t size>
		inline const FunctionStatisticsDetailed &CreateFunctionStatisticsDetailed2(flatbuffers::FlatBufferBuilder &builder,
			unsigned __int64 address, unsigned __int64 times_called, unsigned __int64 max_reentrance,
			__int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time, T (&children_)[size])
		{
			return *CreateUpdateStatisticsPayload2(builder, address, times_called, max_reentrance,
				inclusive_time, exclusive_time, max_call_time, children_).statistics()->Get(0);
		}

		template <typename T, size_t size>
		inline T *end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}

		inline void toupper(std::wstring &s)
		{	std::transform(s.begin(), s.end(), s.begin(), &towupper);	}
	}

	bool operator ==(const function_statistics &lhs, const function_statistics &rhs);
}
