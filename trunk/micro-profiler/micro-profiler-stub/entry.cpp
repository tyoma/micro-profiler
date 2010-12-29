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

#include "./../micro-profiler/entry.h"

extern "C" __declspec(naked, dllexport) void _penter()
{
	_asm 
	{
		ret
	}
}

extern "C" void __declspec(naked, dllexport) _cdecl _pexit()
{
	_asm 
	{
		ret
	}
}

namespace micro_profiler
{
	void __declspec(dllexport) create_local_frontend(IProfilerFrontend **frontend)
	{	*frontend = 0;	}

	void __declspec(dllexport) create_inproc_frontend(IProfilerFrontend **frontend)
	{	*frontend = 0;	}

	profiler_frontend::profiler_frontend(frontend_factory /*factory*/)
		: _collector(*reinterpret_cast<calls_collector *>(0))
	{	}

	profiler_frontend::~profiler_frontend()
	{	}
}