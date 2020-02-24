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

#include <common/string.h>

#include <common/formatting.h>

#include <cstdlib>
#include <stdexcept>
#include <stdio.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	string unicode(const wstring &value)
	{
		string result;

		for (wstring::const_iterator i = value.begin(); i != value.end(); ++i)
		{
			if (*i <= 0x007F)
			{
				// Plain single-byte ASCII.
				result.push_back(static_cast<char>(*i));
			}
			else if (*i <= 0x07FF)
			{
				// Two bytes.
				result.push_back(static_cast<char>(0xC0 | (*i >> 6)));
				result.push_back(static_cast<char>(0x80 | ((*i >> 0) & 0x3F)));
			}
			else
			{
				// Three bytes.
				result.push_back(static_cast<char>(0xE0 | (*i >> 12)));
				result.push_back(static_cast<char>(0x80 | ((*i >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | ((*i >> 0) & 0x3F)));
			}
		}
		return result;
	}

	wstring unicode(const string &utf8)
	{
		// taken from here: https://stackoverflow.com/a/7154226/1166346
		vector<unsigned> unicode;
		size_t i = 0;

		while (i < utf8.size())
		{
			unsigned long uni;
			size_t todo;
			unsigned char ch = utf8[i++];

			if (ch <= 0x7F)
			{
				uni = ch;
				todo = 0;
			}
			else if (ch <= 0xBF)
			{
				throw invalid_argument("not a UTF-8 string");
			}
			else if (ch <= 0xDF)
			{
				uni = ch&0x1F;
				todo = 1;
			}
			else if (ch <= 0xEF)
			{
				uni = ch&0x0F;
				todo = 2;
			}
			else if (ch <= 0xF7)
			{
				uni = ch&0x07;
				todo = 3;
			}
			else
			{
				throw invalid_argument("not a UTF-8 string");
			}
			for (size_t j = 0; j < todo; ++j)
			{
				if (i == utf8.size())
					throw invalid_argument("not a UTF-8 string");
				unsigned char ch2 = utf8[i++];
				if (ch2 < 0x80 || ch2 > 0xBF)
					throw invalid_argument("not a UTF-8 string");
				uni <<= 6;
				uni += ch2 & 0x3F;
			}
			if (uni >= 0xD800 && uni <= 0xDFFF)
				throw invalid_argument("not a UTF-8 string");
			if (uni > 0x10FFFF)
				throw invalid_argument("not a UTF-8 string");
			unicode.push_back(uni);
		}
		wstring utf16;
		for (size_t j = 0; j < unicode.size(); ++j)
		{
			unsigned long uni = unicode[j];
			if (uni <= 0xFFFF)
			{
				utf16 += (wchar_t)uni;
			}
			else
			{
				uni -= 0x10000;
				utf16 += (wchar_t)((uni >> 10) + 0xD800);
				utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
			}
		}
		return utf16;
	}

	string to_string(const guid_t &id)
	{
		string result;
		const byte (&v)[16] = id.values;

		result += '{';
		itoa<16>(result, *reinterpret_cast<const unsigned int *>(&v[0]), 8, '0');
		result += '-';
		itoa<16>(result, *reinterpret_cast<const unsigned short *>(&v[4]), 4, '0');
		result += '-';
		itoa<16>(result, *reinterpret_cast<const unsigned short *>(&v[6]), 4, '0');
		result += '-';
		itoa<16>(result, v[8], 2, '0');
		itoa<16>(result, v[9], 2, '0');
		result += '-';
		itoa<16>(result, v[10], 2, '0');
		itoa<16>(result, v[11], 2, '0');
		itoa<16>(result, v[12], 2, '0');
		itoa<16>(result, v[13], 2, '0');
		itoa<16>(result, v[14], 2, '0');
		itoa<16>(result, v[15], 2, '0');
		result += '}';
		return result;
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
