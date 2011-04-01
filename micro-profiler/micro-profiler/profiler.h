#pragma once

#include "system.h"

#include <hash_map>
#include <vector>
#include <utility>

namespace micro_profiler
{
	struct function_statistics
	{
		function_statistics();

		unsigned int calls;
		unsigned __int64 inclusive_time;
		unsigned __int64 exclusive_time;
	};

	struct thread_calls_statistics
	{
		unsigned int thread_id;
		std::vector< std::pair<void * /*function*/, function_statistics> > statistics;
	};

	class thread_profiler;

	class profiler
	{
		bool _collection_enabled;
		stdext::hash_map<unsigned int, thread_profiler> _profilers;
		mutable mutex _thread_profilers_mtx;

		void enter(void *caller);
		void exit();

	public:
		profiler();
		~profiler();

		static profiler &instance();

		void clear();
		void retrieve(std::vector<thread_calls_statistics> &statistics) const;

		static void enter_function(void *caller);
		static void exit_function();
	};


	inline function_statistics::function_statistics()
		: calls(0), inclusive_time(0), exclusive_time(0)
	{	}
}
