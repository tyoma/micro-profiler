#pragma once

struct IProfilerFrontend;

namespace micro_profiler
{
	typedef void (*frontend_factory)(IProfilerFrontend **frontend);
	class calls_collector;

	class profiler_frontend
	{
		calls_collector &_collector;
		frontend_factory _factory;
		void *_stop_event, *_frontend_thread;

		static unsigned int __stdcall frontend_worker_proxy(void *param);
		void frontend_worker();

		profiler_frontend(const profiler_frontend &);
		void operator =(const profiler_frontend &);

	public:
		__declspec(dllexport) profiler_frontend(frontend_factory factory = 0);
		__declspec(dllexport) ~profiler_frontend();
	};
}
