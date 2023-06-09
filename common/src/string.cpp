//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <clocale>
#include <cstdlib>
#include <stdexcept>
#include <stdio.h>
#include <utfia/iterator.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	locale_lock::locale_lock(const char *locale_)
		: _locked_ok(false)
	{
		if (const auto previous = ::setlocale(LC_NUMERIC, locale_))
			_previous = previous, _locked_ok = true;
	}

	locale_lock::~locale_lock()
	{
		if (_locked_ok)
			::setlocale(LC_NUMERIC, _previous.c_str());
	}


	void unicode(string &destination, const char *ansi_value)
	{	destination = ansi_value;	} // TODO: must convert from ANSI-string to UTF-8

	void unicode(string &destination, const wchar_t *i)
	{
		destination.clear();
		for (; *i; ++i)
		{
			unsigned char l;
			char buffer[3];

			if (*i <= 0x007F)
			{
				// Plain single-byte ASCII.
				buffer[0] = static_cast<char>(*i);
				l = 1;
			}
			else if (*i <= 0x07FF)
			{
				// Two bytes.
				buffer[0] = static_cast<char>(0xC0 | (*i >> 6));
				buffer[1] = static_cast<char>(0x80 | ((*i >> 0) & 0x3F));
				l = 2;
			}
			else
			{
				// Three bytes.
				buffer[0] = static_cast<char>(0xE0 | (*i >> 12));
				buffer[1] = static_cast<char>(0x80 | ((*i >> 6) & 0x3F));
				buffer[2] = static_cast<char>(0x80 | ((*i >> 0) & 0x3F));
				l = 2;
			}
			destination.append(buffer, buffer + l);
		}
	}

	wstring unicode(const string &utf8)
	{
		auto i = utf8.begin();
		const auto end = utf8.end();
		wstring utf16;

		while (i != end)
		{
			auto uni = utfia::utf8::next(i, end);

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
