//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/constants.h>

#include <common/path.h>
#include <common/string.h>
#include <stdlib.h>

using namespace std;

namespace micro_profiler
{
#ifdef _WIN32
	const char *constants::home_ev = "LOCALAPPDATA";
#else
	const char *constants::home_ev = "HOME";
#endif
	const char *constants::profiler_name = ".microprofiler";
	const char *constants::profilerdir_ev = "MICROPROFILERDIR";
	const char *constants::frontend_id_ev = "MICROPROFILERFRONTEND";

	// {0ED7654C-DE8A-4964-9661-0B0C391BE15E}
	const guid_t constants::standalone_frontend_id = {
		{ 0x4c, 0x65, 0xd7, 0x0e, 0x8a, 0xde, 0x64, 0x49, 0x96, 0x61, 0x0b, 0x0c, 0x39, 0x1b, 0xe1, 0x5e, }
	};

	// {91C0CE12-C677-4A50-A522-C86040AC5052}
	const guid_t constants::integrated_frontend_id = {
		{ 0x12, 0xce, 0xc0, 0x91, 0x77, 0xc6, 0x50, 0x4a, 0xa5, 0x22, 0xc8, 0x60, 0x40, 0xac, 0x50, 0x52, }
	};

	string constants::data_directory()
	{
		const char *localdata = getenv(home_ev);

		return string(localdata ? localdata : ".") & string(profiler_name);
	}
}
