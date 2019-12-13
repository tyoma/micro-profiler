//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <set>
#include <stdexcept>
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
			virtual void enumerate_files(const file_callback_t &callback) const;

		private:
			string _path;
			shared_ptr<void> _dbghelp;
			long_address_t _base;
		};



		dbghelp_image_info::dbghelp_image_info(const shared_ptr<void> &dbghelp, const string &path, long_address_t base)
			: _path(path), _dbghelp(dbghelp), _base(base)
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
				local(const shared_ptr<void> &dbghelp, const symbol_callback_t &callback)
					: _file_id(0), _dbghelp(dbghelp), _callback(callback)
				{	}

				static BOOL CALLBACK on_symbol(SYMBOL_INFO *symbol, ULONG, void *context)
				{
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						local *self = static_cast<local *>(context);

						DWORD displacement;
						IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64), };
						unsigned int file_id = 0, line = 0;

						if (::SymGetLineFromAddr64(self->_dbghelp.get(), symbol->Address, &displacement, &info))
						{
							pair<unordered_map<string, unsigned int>::iterator, bool> r
								= self->files.insert(make_pair(info.FileName, 0));

							if (r.second)
								r.first->second = self->_file_id++;
							file_id = r.first->second;
							line = info.LineNumber;
						}

						self->_si.name = symbol->Name;
						self->_si.rva = static_cast<unsigned>(symbol->Address - symbol->ModBase);
						self->_si.size = symbol->Size;
						self->_si.file_id = file_id;
						self->_si.line = line;
						self->_callback(self->_si);
					}
					return TRUE;
				}

			private:
				unsigned int _file_id;
				unordered_map<string, unsigned int> files;
				shared_ptr<void> _dbghelp;
				symbol_callback_t _callback;
				symbol_info _si;
			};

			local cb(_dbghelp, callback);

			::SymEnumSymbols(_dbghelp.get(), _base, NULL, &local::on_symbol, &cb);
		}

		void dbghelp_image_info::enumerate_files(const file_callback_t &callback) const
		{
			class local
			{
			public:
				local(const shared_ptr<void> &dbghelp, const file_callback_t &callback)
					: _file_id(0), _dbghelp(dbghelp), _callback(callback)
				{	}

				static BOOL CALLBACK on_symbol(SYMBOL_INFO *symbol, ULONG, void *context)
				{
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						local *self = static_cast<local *>(context);
						DWORD displacement;
						IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64), };

						if (::SymGetLineFromAddr64(self->_dbghelp.get(), symbol->Address, &displacement, &info))
						{
							if (self->files.insert(info.FileName).second)
								self->_callback(make_pair(self->_file_id++, info.FileName));
						}
					}
					return TRUE;
				}

			private:
				unsigned int _file_id;
				set<string> files;
				shared_ptr<void> _dbghelp;
				file_callback_t _callback;
			};

			local cb(_dbghelp, callback);

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
	}


	shared_ptr< image_info<symbol_info> > load_image_info(const char *image_path)
	{	return shared_ptr< image_info<symbol_info> >(new dbghelp_image_info(create_dbghelp(), image_path, 1));	}
}
