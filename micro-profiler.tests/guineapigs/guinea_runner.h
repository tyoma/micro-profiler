#pragma once

#include <string>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>

namespace micro_profiler
{
	namespace tests
	{
		enum runner_commands {
			load_module,
			unload_module,
			execute_function,
		};
	}
}

namespace strmd
{
	template <typename ArchiveT>
	void serialize(ArchiveT &archive, micro_profiler::tests::runner_commands &data)
	{	archive(reinterpret_cast<int &>(data));	}
}