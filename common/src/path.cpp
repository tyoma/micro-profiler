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

#include <common/path.h>

using namespace std;

namespace micro_profiler
{
	string operator &(const string &lhs, const string &rhs)
	{
		if (lhs.empty())
			return rhs;
		if (lhs.back() == '\\' || lhs.back() =='/')
			return lhs + rhs;
		return lhs + '/' + rhs;
	}

	string operator ~(const string &value)
	{
		const char separators[] = {	'\\', '/', '\0'	};
		const auto pos = value.find_last_of(separators);

		if (pos != string::npos)
			return value.substr(0, pos);
		return string();
	}

	const char *operator *(const string &value)
	{
		const char separators[] = {	'\\', '/', '\0'	};
		const auto pos = value.find_last_of(separators);

		return value.c_str() + (pos != string::npos ? pos + 1 : 0u);
	}


	string normalize::exe(const string &path)
	{
#if defined(_WIN32)
		return path + ".exe";
#else
		return path;
#endif
	}

	string normalize::lib(const string &path)
	{
#if defined(_WIN32)
		return path + ".dll";
#elif defined(__APPLE__)
		return ~path & (string("lib") + *path + ".dylib");
#else
		return ~path & (string("lib") + *path + ".so");
#endif
	}
}
