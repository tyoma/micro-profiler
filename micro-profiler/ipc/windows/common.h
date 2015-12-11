#pragma once

#include <string>

namespace micro_profiler
{
	namespace ipc
	{
		const std::string c_pipe_ns = "\\\\.\\pipe\\";
		const std::string c_prefix = "F55E85115A374F0AA7D423BDAC119637.";

		struct zero_init
		{
			template <typename T>
			operator T() const
			{
				T t = { };
				return t;
			}
		};
	}
}
