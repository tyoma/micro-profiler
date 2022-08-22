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

namespace micro_profiler
{
	std::string operator &(const std::string &lhs, const std::string &rhs)
	{
		if (lhs.empty())
			return rhs;
		if (lhs.back() == '\\' || lhs.back() =='/')
			return lhs + rhs;
		return lhs + '/' + rhs;
	}

	std::string operator ~(const std::string &value)
	{
		const char separators[] = {	'\\', '/', '\0'	};
		const auto pos = value.find_last_of(separators);

		if (pos != std::string::npos)
			return value.substr(0, pos);
		return std::string();
	}

	const char *operator *(const std::string &value)
	{
		const char separators[] = {	'\\', '/', '\0'	};
		const auto pos = value.find_last_of(separators);

		return value.c_str() + (pos != std::string::npos ? pos + 1 : 0u);
	}


	std::string normalize::exe(const std::string &path)
	{
#if defined(_WIN32)
		return path + ".exe";
#else
		return path;
#endif
	}

	std::string normalize::lib(const std::string &path)
	{
#if defined(_WIN32)
		return path + ".dll";
#elif defined(__APPLE__)
		return ~path & (std::string("lib") + *path + ".dylib");
#else
		return ~path & (std::string("lib") + *path + ".so");
#endif
	}
}
