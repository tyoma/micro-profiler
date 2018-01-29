//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "string.h"

#include <cstdlib>
#include <vector>

using namespace std;

namespace micro_profiler
{
	string unicode(const wstring &value)
	{
		vector<char> buffer(wcstombs(0, value.c_str(), 0) + 1u);
		
		wcstombs(&buffer[0], value.c_str(), buffer.size());
		return &buffer[0];
	}

	wstring unicode(const string &value)
	{
		vector<wchar_t> buffer(mbstowcs(0, value.c_str(), 0) + 1u);
		
		mbstowcs(&buffer[0], value.c_str(), buffer.size());
		return &buffer[0];
	}
}
