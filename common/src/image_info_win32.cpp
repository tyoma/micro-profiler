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

#include <common/image_info.h>

#include <common/module.h>
#include <common/noncopyable.h>
#include <common/path.h>
#include <common/primitives.h>
#include <common/string.h>

#pragma warning(disable: 4091)

#include <windows.h>
#include <dbghelp.h>
#include <set>
#include <stdexcept>
#include <common/unordered_map.h>

#define GET_PROC_ADDRESS(hmodule, symbol) reinterpret_cast<decltype(::symbol) *>(::GetProcAddress(hmodule, #symbol))

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
#ifdef _M_IX86
		const string c_bitness_directory = "x86";
#elif _M_X64
		const string c_bitness_directory = "x64";
#endif
		const string c_dbghelps[] = {
			~get_module_info(&c_bitness_directory).path & "windows7+" & c_bitness_directory & "dbghelp.dll",
			~get_module_info(&c_bitness_directory).path & "windowsxp+" & c_bitness_directory & "dbghelp.dll",
			"dbghelp.dll",
		};

		class dbghelp : noncopyable
		{
		public:
			dbghelp();
			~dbghelp();

			DWORD64 load_image(const string &path);
			decltype(::SymEnumSymbols) *SymEnumSymbols;
			decltype(::SymGetLineFromAddr64) *SymGetLineFromAddr64;

		private:
			typedef DWORD64 (__stdcall *load_module_ex_t)(HANDLE hProcess, HANDLE hFile, PCWSTR ImageName,
				PCWSTR ModuleName, DWORD64 BaseOfDll,DWORD DllSize, PMODLOAD_DATA Data, DWORD Flags);

		private:
			shared_ptr<void> _module;

			decltype(::SymInitialize) *_SymInitialize;
			decltype(::SymCleanup) *_SymCleanup;
			decltype(::SymLoadModuleExW) *_SymLoadModuleExW;
			decltype(::SymLoadModule64) *_SymLoadModule64;
		};


		class dbghelp_image_info : public image_info
		{
		public:
			dbghelp_image_info(const shared_ptr<dbghelp> &dbghelp_, const string &path);

		private:
			virtual void enumerate_functions(const symbol_callback_t &callback) const override;
			virtual void enumerate_files(const file_callback_t &callback) const override;

		private:
			string _path;
			shared_ptr<dbghelp> _dbghelp;
			long_address_t _base;
		};



		dbghelp::dbghelp()
		{
			auto path_env = _wgetenv(L"PATH");

			path_env = path_env ? path_env : L"";
			for (auto i = begin(c_dbghelps); !_module && i != end(c_dbghelps); ++i)
			{
				::SetEnvironmentVariableW(L"PATH", (wstring(path_env) + L";" + unicode(~*i)).c_str());
				_module = load_library(*i);
			}
			::SetEnvironmentVariableW(L"PATH", path_env);

			const auto hmodule = static_cast<HMODULE>(_module.get());

			SymEnumSymbols = GET_PROC_ADDRESS(hmodule, SymEnumSymbols);
			SymGetLineFromAddr64 = GET_PROC_ADDRESS(hmodule, SymGetLineFromAddr64);
			_SymInitialize = GET_PROC_ADDRESS(hmodule, SymInitialize);
			_SymCleanup = GET_PROC_ADDRESS(hmodule, SymCleanup);
			_SymLoadModuleExW = GET_PROC_ADDRESS(hmodule, SymLoadModuleExW);
			_SymLoadModule64 = GET_PROC_ADDRESS(hmodule, SymLoadModule64);

			if (!_SymInitialize(this, NULL, FALSE))
				throw 0;
		}

		dbghelp::~dbghelp()
		{	_SymCleanup(this);	}

		DWORD64 dbghelp::load_image(const string &path)
		{
			::SetLastError(0);

			// Attempt to obtain symbols via dbghelp.dll/v6.0.
			if (_SymLoadModuleExW)
			{
				if (DWORD64 base = _SymLoadModuleExW(this, NULL, unicode(path).c_str(), NULL, 0, 0, NULL, 0))
					return base;
			}
			// Fallback attempt to obtain symbols via dbghelp.dll/v5.1 (who knows, maybe we're on Windows XP).
			else if (DWORD64 base = _SymLoadModule64(this, NULL, path.c_str(), NULL, 0, 0))
			{
				return base;
			}
			throw invalid_argument("The symbols for module '" + path + "' can not be loaded!");
		}


		dbghelp_image_info::dbghelp_image_info(const shared_ptr<dbghelp> &dbghelp_, const string &path)
			: _path(path), _dbghelp(dbghelp_), _base(dbghelp_->load_image(path))
		{	}

		void dbghelp_image_info::enumerate_functions(const symbol_callback_t &callback) const
		{
			class local
			{
			public:
				local(const shared_ptr<dbghelp> &dbghelp_, const symbol_callback_t &callback)
					: _file_id(0), _dbghelp(dbghelp_), _callback(callback)
				{	}

				static BOOL CALLBACK on_symbol(SYMBOL_INFO *symbol, ULONG, void *context)
				{
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						local *self = static_cast<local *>(context);

						DWORD displacement;
						IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64), };
						unsigned int file_id = 0, line = 0;

						if (self->_dbghelp->SymGetLineFromAddr64(self->_dbghelp.get(), symbol->Address, &displacement, &info))
						{
							pair<containers::unordered_map<string, unsigned int>::iterator, bool> r
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
				containers::unordered_map<string, unsigned int> files;
				shared_ptr<dbghelp> _dbghelp;
				symbol_callback_t _callback;
				symbol_info _si;
			};

			local cb(_dbghelp, callback);

			_dbghelp->SymEnumSymbols(_dbghelp.get(), _base, NULL, &local::on_symbol, &cb);
		}

		void dbghelp_image_info::enumerate_files(const file_callback_t &callback) const
		{
			class local
			{
			public:
				local(const shared_ptr<dbghelp> &dbghelp_, const file_callback_t &callback)
					: _file_id(0), _dbghelp(dbghelp_), _callback(callback)
				{	}

				static BOOL CALLBACK on_symbol(SYMBOL_INFO *symbol, ULONG, void *context)
				{
					if (5 /*SymTagFunction*/ == symbol->Tag)
					{
						local *self = static_cast<local *>(context);
						DWORD displacement;
						IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64), };

						if (self->_dbghelp->SymGetLineFromAddr64(self->_dbghelp.get(), symbol->Address, &displacement, &info))
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
				shared_ptr<dbghelp> _dbghelp;
				file_callback_t _callback;
			};

			local cb(_dbghelp, callback);

			_dbghelp->SymEnumSymbols(_dbghelp.get(), _base, NULL, &local::on_symbol, &cb);
		}
	}


	shared_ptr<image_info> load_image_info(const string &image_path)
	{
		shared_ptr<dbghelp> dh(new dbghelp);

		return shared_ptr<image_info>(new dbghelp_image_info(dh, image_path));
	}
}
