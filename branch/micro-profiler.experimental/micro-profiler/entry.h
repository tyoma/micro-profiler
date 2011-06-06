//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

struct IProfilerFrontend;

namespace micro_profiler
{
	typedef void (*frontend_factory)(IProfilerFrontend **frontend);
	class calls_collector;

	void __declspec(dllexport) create_local_frontend(IProfilerFrontend **frontend);
	void __declspec(dllexport) create_inproc_frontend(IProfilerFrontend **frontend);

	class profiler_frontend
	{
		calls_collector &_collector;
		frontend_factory _factory;
		unsigned _frontend_threadid;
		void *_frontend_thread;

		static unsigned int __stdcall frontend_worker_proxy(void *param);
		void frontend_worker();

		profiler_frontend(const profiler_frontend &);
		void operator =(const profiler_frontend &);

	public:
		__declspec(dllexport) profiler_frontend(frontend_factory factory = &create_local_frontend);
		__declspec(dllexport) ~profiler_frontend();
	};
}
