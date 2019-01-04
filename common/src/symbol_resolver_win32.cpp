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

#include <common/primitives.h>

#include <windows.h>
#include <dbghelp.h>
#include <unordered_map>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		class dbghelp_image_info : public image_info<symbol_info>
		{
		public:
			dbghelp_image_info(const shared_ptr<void> &dbghelp, const string &path, long_address_t base);

		private:
			virtual void enumerate_functions(const symbol_callback_t &callback) const;

		private:
			shared_ptr<void> _dbghelp;
			long_address_t _base;
		};

		class dbghelp_symbol_resolver : public symbol_resolver
		{
		public:
			dbghelp_symbol_resolver();

			virtual const string &symbol_name_by_va(long_address_t address) const;
			virtual pair<string, unsigned> symbol_fileline_by_va(long_address_t address) const;
			virtual void add_image(const char *image, long_address_t load_address);

		private:
			typedef unordered_map<long_address_t, string, address_compare> cached_names_map;

		private:
			shared_ptr<void> _dbghelp;
			vector< shared_ptr< image_info<symbol_info> > > _loaded_images;
			mutable cached_names_map _names;
		};



		dbghelp_image_info::dbghelp_image_info(const shared_ptr<void> &dbghelp, const string &path, long_address_t base)
			: _dbghelp(dbghelp), _base(base)
		{
			::SetLastError(0);
			if (::SymLoadModule64(_dbghelp.get(), NULL, path.c_str(), NULL, _base, 0))
			{
				if (ERROR_SUCCESS == ::GetLastError() || INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path.c_str()))
					return;
				::SymUnloadModule64(_dbghelp.get(), _base);
			}
			throw invalid_argument("");
		}

		void dbghelp_image_info::enumerate_functions(const symbol_callback_t &callback) const
		{
			class local
			{
			public:
				local(const symbol_callback_t &callback)
					: _callback(callback)
				{	}

				static BOOL CALLBACK on_symbol(SYMBOL_INFO *symbol, ULONG, void *context)
				{
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						local *self = static_cast<local *>(context);

						self->_si.name = symbol->Name;
						self->_si.rva = static_cast<unsigned>(symbol->Address - symbol->ModBase);
						self->_si.size = symbol->Size;
						self->_callback(self->_si);
					}
					return TRUE;
				}

			private:
				symbol_callback_t _callback;
				symbol_info _si;
			};

			local cb(callback);

			::SymEnumSymbols(_dbghelp.get(), _base, NULL, &local::on_symbol, &cb);
		}


		shared_ptr<void> create_dbghelp()
		{
			struct dbghelp
			{
				dbghelp()
				{
					if (!::SymInitialize(this, NULL, FALSE))
						throw 0;
				}

				~dbghelp()
				{	::SymCleanup(this);	}
			};

			return shared_ptr<void>(new dbghelp);
		}


		dbghelp_symbol_resolver::dbghelp_symbol_resolver()
			: _dbghelp(create_dbghelp())
		{	}

		const string &dbghelp_symbol_resolver::symbol_name_by_va(long_address_t address) const
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
					::SymFromAddr(_dbghelp.get(), address, 0, symbol2.get());
				} while (symbol2->MaxNameLen == symbol2->NameLen);
				i = _names.insert(make_pair(address, symbol2->Name)).first;
			}
			return i->second;
		}

		pair<string, unsigned> dbghelp_symbol_resolver::symbol_fileline_by_va(long_address_t address) const
		{
			DWORD displacement;
			IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64), };

			return ::SymGetLineFromAddr64(_dbghelp.get(), address, &displacement, &info)
				? pair<string, unsigned>(info.FileName, info.LineNumber) : pair<string, unsigned>();
		}

		void dbghelp_symbol_resolver::add_image(const char *image, long_address_t load_address)
		{
			shared_ptr< image_info<symbol_info> > ii(new dbghelp_image_info(_dbghelp, image, load_address));
			_loaded_images.push_back(ii);
		}
	}


	shared_ptr< image_info<symbol_info> > load_image_info(const char *image_path)
	{	return shared_ptr< image_info<symbol_info> >(new dbghelp_image_info(create_dbghelp(), image_path, 1));	}


	shared_ptr<symbol_resolver> symbol_resolver::create()
	{	return shared_ptr<dbghelp_symbol_resolver>(new dbghelp_symbol_resolver());	}
}
