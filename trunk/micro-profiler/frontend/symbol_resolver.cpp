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

#include "symbol_resolver.h"

#include "../common/primitives.h"

#include <atlstr.h>
#include <Windows.h>
#include <dbghelp.h>
#include <unordered_map>

namespace std
{
	using std::tr1::unordered_map;
}

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class dbghelp_symbol_resolver : public symbol_resolver
		{
			typedef unordered_map<const void *, wstring, address_compare> cached_names_map;

			mutable cached_names_map _names;

			HANDLE me() const;

		public:
			dbghelp_symbol_resolver();
			virtual ~dbghelp_symbol_resolver();

			void load(const wstring &module, const void *at);

			virtual const wstring &symbol_name_by_va(const void *address) const;
		};


		dbghelp_symbol_resolver::dbghelp_symbol_resolver()
		{
			if (!::SymInitialize(me(), NULL, FALSE))
				throw 0;
		}

		dbghelp_symbol_resolver::~dbghelp_symbol_resolver()
		{	::SymCleanup(me());	}

		HANDLE dbghelp_symbol_resolver::me() const
		{	return reinterpret_cast<HANDLE>(const_cast<dbghelp_symbol_resolver *>(this));	}

		void dbghelp_symbol_resolver::load(const wstring &module, const void *at)
		{
			if (!::SymLoadModule64(me(), NULL, CStringA(module.c_str()), NULL, reinterpret_cast<DWORD64>(at), 0))
				throw invalid_argument("");
		}

		const wstring &dbghelp_symbol_resolver::symbol_name_by_va(const void *address) const
		{
			cached_names_map::iterator i = _names.find(address);

			if (i == _names.end())
			{
				const size_t max_name_length = 300;
				SYMBOL_INFO dummy;
				char buffer[sizeof(SYMBOL_INFO) + max_name_length * sizeof(dummy.Name[0])] = { 0 };
				SYMBOL_INFO &symbol = *reinterpret_cast<SYMBOL_INFO *>(buffer);

				dummy;
				symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol.MaxNameLen = max_name_length;
				::SymFromAddr(me(), reinterpret_cast<DWORD64>(address), 0, &symbol);
				i = _names.insert(make_pair(address, (const wchar_t *)CStringW(symbol.Name))).first;
			}
			return i->second;
		}
	}

	shared_ptr<symbol_resolver> symbol_resolver::create(const wstring &image_path, unsigned __int64 load_address)
	{
		shared_ptr<dbghelp_symbol_resolver> r(new dbghelp_symbol_resolver());
		
		r->load(image_path, reinterpret_cast<const void *>(load_address));
		return r;
	}
}
