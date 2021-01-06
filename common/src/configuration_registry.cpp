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

#include <common/win32/configuration_registry.h>

#include <common/string.h>

#include <stdexcept>
#include <vector>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<void> open_key(HKEY root, const char *relative_path, REGSAM rights)
		{
			HKEY hkey = NULL;

			return ERROR_SUCCESS == ::RegOpenKeyExW(root, unicode(relative_path).c_str(), 0, rights, &hkey) && hkey
				? shared_ptr<void>(hkey, &::RegCloseKey) : nullptr;
		}
	}

	registry_hive::registry_hive(shared_ptr<void> key)
		: _key(key)
	{	}

	shared_ptr<hive> registry_hive::open_user_settings(const char *path)
	{
		if (auto root = open_key(HKEY_CURRENT_USER, path, KEY_READ))
			return make_shared<registry_hive>(root);
		throw runtime_error("Cannot open registry hive requested!");
	}

	shared_ptr<hive> registry_hive::create(const char *name)
	{
		HKEY hkey = NULL;

		::RegCreateKeyExW(*this, unicode(name).c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &hkey, NULL);
		return make_shared<registry_hive>(shared_ptr<void>(hkey, &::RegCloseKey));
	}

	shared_ptr<const hive> registry_hive::open(const char *name) const
	{	return make_shared<registry_hive>(open_key(*this, name, KEY_READ));	}

	void registry_hive::store(const char *name, int value)
	{
		::RegSetValueExW(*this, unicode(name).c_str(), 0, REG_DWORD, reinterpret_cast<BYTE *>(&value),
			sizeof(value));
	}

	void registry_hive::store(const char *name, const char *value)
	{
		::RegSetValueExA(*this, name, 0, REG_SZ, reinterpret_cast<BYTE *>(const_cast<char *>(value)),
			static_cast<DWORD>(strlen(value) + 1));
	}

	bool registry_hive::load(const char *name, int &value) const
	{
		DWORD type = 0, size = sizeof(value);

		return ERROR_SUCCESS == ::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type,
			reinterpret_cast<BYTE *>(&value), &size) && type == REG_DWORD && size == sizeof(value);
	}

	bool registry_hive::load(const char *name, string &value) const
	{
		DWORD type = 0, size = sizeof(value);

		if (ERROR_SUCCESS == ::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type, NULL, &size)
			&& REG_SZ == type)
		{
			vector<wchar_t> buffer(size / sizeof(char) + 1);

			::RegQueryValueExW(*this, unicode(name).c_str(), 0, &type, reinterpret_cast<BYTE*>(&buffer[0]), &size);
			value = unicode(&buffer[0]);
			return true;
		}
		return false;
	}

	registry_hive::operator HKEY() const
	{	return static_cast<HKEY>(_key.get());	}
}
