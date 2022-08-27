#pragma once

#include <string>

namespace micro_profiler
{
	namespace tests
	{
		enum runner_commands {
			load_module,
			unload_module,
			execute_function,
			get_process_id,
			run_load,
		};

		template <typename ArchiveT>
		void serialize(ArchiveT &archive, runner_commands &data)
		{	archive(reinterpret_cast<int &>(data));	}
	}
}
