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

#include <common/symbol_resolver.h>

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

			virtual const wstring &symbol_name_by_va(long_address_t address) const;
			virtual pair<wstring, unsigned> symbol_fileline_by_va(long_address_t address) const;
			virtual void add_image(const wchar_t *image, long_address_t load_address);
			virtual void enumerate_symbols(long_address_t image_address,
				const function<void(const symbol_info &symbol)> &symbol_callback);

		private:
			typedef unordered_map<long_address_t, wstring, address_compare> cached_names_map;

		private:
			HANDLE me() const;

		private:
			mutable cached_names_map _names;
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

		const wstring &dbghelp_symbol_resolver::symbol_name_by_va(long_address_t address) const
		{
			cached_names_map::iterator i = _names.find(address);

			if (i == _names.end())
			{
				SYMBOL_INFO symbol = { };
				shared_ptr<SYMBOL_INFO> symbol2;

				symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol.MaxNameLen = 5;
				do
				{
					symbol2.reset(static_cast<SYMBOL_INFO *>(malloc(sizeof(SYMBOL_INFO) + symbol.MaxNameLen)), &free);
					*symbol2 = symbol;
					symbol.MaxNameLen <<= 1;
					::SymFromAddr(me(), address, 0, symbol2.get());
				} while (symbol2->MaxNameLen == symbol2->NameLen);
				i = _names.insert(make_pair(address, unicode(symbol2->Name))).first;
			}
			return i->second;
		}

		pair<wstring, unsigned> dbghelp_symbol_resolver::symbol_fileline_by_va(long_address_t address) const
		{
			DWORD dummy;
			IMAGEHLP_LINEW64 info = { sizeof(IMAGEHLP_LINEW64), };

			::SymGetLineFromAddrW64(me(), address, &dummy, &info);
			return make_pair(info.FileName, info.LineNumber);
		}

		void dbghelp_symbol_resolver::add_image(const wchar_t *image, long_address_t load_address)
		{
			string image_path_ansi = unicode(image);

			::SetLastError(0);
			if (::SymLoadModule64(me(), NULL, image_path_ansi.c_str(), NULL, load_address, 0))
			{
				if (ERROR_SUCCESS == ::GetLastError() || INVALID_FILE_ATTRIBUTES != GetFileAttributesA(image_path_ansi.c_str()))
					return;
				::SymUnloadModule64(me(), load_address);
			}
			throw invalid_argument("");
		}

		void dbghelp_symbol_resolver::enumerate_symbols(long_address_t image_address,
			const function<void(const symbol_info &symbol)> &symbol_callback)
		{
			struct local
			{
				static BOOL CALLBACK callback(SYMBOL_INFO *symbol, ULONG size, void *context)
				{
					symbol_info si = { };
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						si.name = symbol->Name;
						si.location = reinterpret_cast<void *>(static_cast<size_t>(symbol->Address));
						si.size = size;

						(*static_cast<const function<void(const symbol_info &symbol)> *>(context))(si);
					}
					return TRUE;
				}
			};

			::SymEnumSymbols(me(), image_address, NULL, &local::callback, const_cast<void *>((const void *)&symbol_callback));
		}
	}

	void symbol_resolver::enumerate_symbols(long_address_t /*image_address*/,
		const function<void(const symbol_info &symbol)> &/*symbol_callback*/)
	{	}

	shared_ptr<symbol_resolver> symbol_resolver::create()
	{	return shared_ptr<dbghelp_symbol_resolver>(new dbghelp_symbol_resolver());	}
}
