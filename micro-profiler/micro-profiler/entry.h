#pragma once

#include <memory>

struct IProfilerFrontend;

namespace micro_profiler
{
	struct destructible
	{
		virtual ~destructible()	{	}
	};

	typedef void (*frontend_factory)(IProfilerFrontend **frontend);

	__declspec(dllexport) std::auto_ptr<destructible> initialize_frontend(frontend_factory factory = 0);
}
