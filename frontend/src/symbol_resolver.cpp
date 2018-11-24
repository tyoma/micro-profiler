//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/symbol_resolver.h>

#include <common/string.h>

#include <windows.h>
#include <dbghelp.h>
#include <unordered_map>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class dbghelp_symbol_resolver : public symbol_resolver
		{
		public:
			dbghelp_symbol_resolver();
			virtual ~dbghelp_symbol_resolver();

			virtual bool get_symbol(address_t address, symbol_t &symbol) const;
			virtual void add_image(const wstring &image, address_t base_address);

		private:
			HANDLE me() const;
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

		bool dbghelp_symbol_resolver::get_symbol(address_t address, symbol_t &symbol_) const
		{
			SYMBOL_INFO symbol = { };
			shared_ptr<SYMBOL_INFO> symbol2;
			DWORD displacement;
			IMAGEHLP_LINEW64 info = { sizeof(IMAGEHLP_LINEW64), };

			symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol.MaxNameLen = 5;
			do
			{
				symbol2.reset(static_cast<SYMBOL_INFO *>(malloc(sizeof(SYMBOL_INFO) + symbol.MaxNameLen)), &free);
				*symbol2 = symbol;
				symbol.MaxNameLen <<= 1;
				if (!::SymFromAddr(me(), address, 0, symbol2.get()))
					return false;
			} while (symbol2->MaxNameLen == symbol2->NameLen);
			symbol_.name = unicode(symbol2->Name);
			if (::SymGetLineFromAddrW64(me(), address, &displacement, &info))
				symbol_.file = info.FileName, symbol_.line = info.LineNumber;
			else
				symbol_.file.clear(), symbol_.line = 0;
			return true;
		}

		void dbghelp_symbol_resolver::add_image(const wstring &image, address_t base_address)
		{
			string image_path_ansi = unicode(image);

			::SetLastError(0);
			if (::SymLoadModule64(me(), NULL, image_path_ansi.c_str(), NULL, base_address, 0))
			{
				if (ERROR_SUCCESS == ::GetLastError() || INVALID_FILE_ATTRIBUTES != GetFileAttributesA(image_path_ansi.c_str()))
					return;
				::SymUnloadModule64(me(), base_address);
			}
			throw invalid_argument("");
		}
	}

	shared_ptr<symbol_resolver> symbol_resolver::create()
	{	return shared_ptr<dbghelp_symbol_resolver>(new dbghelp_symbol_resolver());	}
}
