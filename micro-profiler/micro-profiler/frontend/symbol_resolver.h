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

#pragma once

#include <atlbase.h>
#include <string>
#include <tchar.h>

struct IDiaSession;
struct IDiaDataSource;
typedef std::basic_string<TCHAR> tstring;

class symbol_resolver
{
	CComPtr<IDiaDataSource> _data_source;
	CComPtr<IDiaSession> _session;

public:
	symbol_resolver(const tstring &image_path, unsigned __int64 load_address);
	~symbol_resolver();

	tstring symbol_name_by_va(unsigned __int64 address) const;
};
