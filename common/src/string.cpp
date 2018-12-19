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

#include <common/string.h>

#include <cstdlib>
#include <stdio.h>
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

	string to_string(const guid_t &id)
	{
		char buffer[100];
		const byte (&v)[16] = id.values;

		sprintf(buffer, "{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
			*reinterpret_cast<const unsigned int *>(&v[0]),
			*reinterpret_cast<const unsigned short *>(&v[4]),  *reinterpret_cast<const unsigned short *>(&v[6]),
			v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
		return buffer;
	}

	guid_t from_string(const string &text)
	{
		struct {
			int padding1;
			guid_t id;
			int padding2;
		} x;
		byte (&v)[16] = x.id.values;

		sscanf(text.c_str(), "{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
			reinterpret_cast<unsigned int *>(&v[0]),
			reinterpret_cast<unsigned short *>(&v[4]),  reinterpret_cast<unsigned short *>(&v[6]),
			&v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);
		return x.id;
	}
}
