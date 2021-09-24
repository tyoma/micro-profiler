//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <logger/log.h>

#include <string.h>

using namespace std;

namespace micro_profiler
{
	namespace log
	{
		logger *g_logger;

		void to_string(buffer_t &buffer, bool value)
		{	to_string(buffer, value ? "true" : "false");	}

		void to_string(buffer_t &buffer, const void *value)
		{
			buffer.push_back('0'), buffer.push_back('x');
			itoa<16>(buffer, reinterpret_cast<size_t>(value), sizeof(size_t) * 2, '0');
		}

		void to_string(buffer_t &buffer, const string &value)
		{	buffer.insert(buffer.end(), value.begin(), value.end());	}

		void to_string(buffer_t &buffer, const char *value)
		{
			static const char null[] = "<null>";

			value = value ? value : null;

			if (const size_t l = strlen(value))
				buffer.insert(buffer.end(), value, value + l);
		}
	}
}
