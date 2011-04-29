#pragma once

struct IProfilerFrontend;

namespace micro_profiler
{
	typedef void (*frontend_factory)(IProfilerFrontend **frontend);

	class profiler_frontend
	{
		frontend_factory _factory;
		void *_stop_event, *_frontend_thread;

		static unsigned int __stdcall profiler_frontend::frontend_proc(void *param);

		profiler_frontend(const profiler_frontend &);
		void operator =(const profiler_frontend &);

	public:
		__declspec(dllexport) profiler_frontend(frontend_factory factory = 0);
		__declspec(dllexport) ~profiler_frontend();
	};
}
