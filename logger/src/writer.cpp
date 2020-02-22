//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <logger/writer.h>

#include "file.h"
#include "format.h"

#include <common/path.h>
#include <memory>
#include <stdio.h>

using namespace std;

namespace micro_profiler
{
	namespace log
	{
		writer_t create_writer(const string &base_path)
		{
			shared_ptr<FILE> file;
			const string path = ~base_path;
			string filename = *base_path;
			const size_t dot = filename.find_last_of(".");
			const string ext = dot != string::npos ? filename.substr(dot) : string();
			unsigned u = 2;

			filename.resize(dot);
			for (string candidate = base_path; file = fopen_exclusive(candidate, "at"), !file; ++u)
			{
				candidate = path & filename;
				candidate += '-';
				uitoa<10>(candidate, u);
				candidate += ext;
			}
			setbuf(file.get(), NULL);
			return [file] (const char *message) { fputs(message, file.get()); };
		}
	}
}
