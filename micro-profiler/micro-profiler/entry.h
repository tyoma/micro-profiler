#pragma once

#include <memory>

struct IProfilerSink;

namespace micro_profiler
{
	struct destructible
	{
		virtual ~destructible()	{	}
	};

	typedef void (*frontend_factory)(IProfilerSink **sink);

	__declspec(dllexport) std::auto_ptr<destructible> initialize_frontend(frontend_factory factory = 0);
}
