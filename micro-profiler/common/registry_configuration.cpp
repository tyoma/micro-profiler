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

#include "configuration.h"

#include "string.h"

#include <vector>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class registry_hive : public hive
		{
		public:
			registry_hive(shared_ptr<void> key)
				: _key(key)
			{	}

		private:
			operator HKEY() const
			{	return static_cast<HKEY>(_key.get());	}

			virtual shared_ptr<hive> create(const char *name)
			{
				HKEY hkey = NULL;

				::RegCreateKeyExW(*this, unicode(name).c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
					NULL, &hkey, NULL);
				shared_ptr<void> key(hkey, &::RegCloseKey);
				return shared_ptr<hive>(new registry_hive(key));
			}

			virtual shared_ptr<const hive> open(const char *name) const
			{
				HKEY hkey = NULL;

				if (ERROR_SUCCESS == ::RegOpenKeyExW(*this, unicode(name).c_str(), 0, KEY_READ, &hkey) && hkey)
				{
					shared_ptr<void> key(hkey, &::RegCloseKey);
					return shared_ptr<hive>(new registry_hive(key));
				}
				return shared_ptr<hive>();
			}

			virtual void store(const char *name, int value)
			{
				::RegSetValueExW(*this, unicode(name).c_str(), 0, REG_DWORD, reinterpret_cast<BYTE *>(&value),
					sizeof(value));
			}

			virtual void store(const char *name, const wchar_t *value)
			{
				::RegSetValueExW(*this, unicode(name).c_str(), 0, REG_SZ,
					reinterpret_cast<BYTE *>(const_cast<wchar_t *>(value)), static_cast<DWORD>(2 * (wcslen(value) + 1)));
			}

			virtual bool load(const char *name, int &value) const
			{
				DWORD type = 0, size = sizeof(value);

				return ERROR_SUCCESS == ::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type,
					reinterpret_cast<BYTE *>(&value), &size) && type == REG_DWORD && size == sizeof(value);
			}

			virtual bool load(const char *name, wstring &value) const
			{
				DWORD type = 0, size = sizeof(value);

				if (ERROR_SUCCESS == ::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type, NULL, &size)
					&& REG_SZ == type)
				{
					vector<wchar_t> buffer(size / sizeof(wchar_t) + 1);

					::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type, reinterpret_cast<BYTE*>(&buffer[0]), &size);
					value = &buffer[0];
					return true;
				}
				return false;
			}

		private:
			shared_ptr<void> _key;
		};
	}

	shared_ptr<hive> hive::user_settings(const char *path)
	{
		HKEY hkey = NULL;

		if (ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_CURRENT_USER, unicode(path).c_str(), 0, KEY_READ, &hkey) && hkey)
		{
			shared_ptr<void> key(hkey, &::RegCloseKey);
			return shared_ptr<hive>(new registry_hive(key));
		}
		throw runtime_error("Cannot open registry hive requested!");
	}
}
