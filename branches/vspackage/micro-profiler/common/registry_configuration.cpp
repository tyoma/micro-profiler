#include "configuration.h"

#include <atlstr.h>
#include <windows.h>
#include <vector>

using namespace std;

typedef ATL::CString x2t;
typedef ATL::CStringW x2w;

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

				::RegCreateKeyEx(*this, x2t(name), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
					NULL, &hkey, NULL);
				shared_ptr<void> key(hkey, &::RegCloseKey);
				return shared_ptr<hive>(new registry_hive(key));
			}

			virtual shared_ptr<const hive> open(const char *name) const
			{
				HKEY hkey = NULL;

				if (ERROR_SUCCESS == ::RegOpenKeyEx(*this, x2t(name), 0, KEY_READ, &hkey) && hkey)
				{
					shared_ptr<void> key(hkey, &::RegCloseKey);
					return shared_ptr<hive>(new registry_hive(key));
				}
				return shared_ptr<hive>();
			}

			virtual void store(const char *name, int value)
			{	::RegSetValueEx(*this, x2t(name), 0, REG_DWORD, reinterpret_cast<BYTE *>(&value), sizeof(value));	}

			virtual void store(const char *name, const wchar_t *value)
			{
				::RegSetValueExW(*this, x2w(name), 0, REG_SZ, reinterpret_cast<BYTE *>(const_cast<wchar_t *>(value)),
					static_cast<DWORD>(2 * (wcslen(value) + 1)));
			}

			virtual bool load(const char *name, int &value) const
			{
				DWORD type = 0, size = sizeof(value);

				return ERROR_SUCCESS == ::RegQueryValueEx(*this, x2t(name), 0, &type, reinterpret_cast<BYTE *>(&value),
					&size) && type == REG_DWORD && size == sizeof(value);
			}

			virtual bool load(const char *name, wstring &value) const
			{
				DWORD type = 0, size = sizeof(value);

				if (ERROR_SUCCESS == ::RegQueryValueExW(*this, x2w(name), 0, &type, NULL, &size) && REG_SZ == type)
				{
					vector<wchar_t> buffer(size / sizeof(wchar_t) + 1);

					::RegQueryValueExW(*this, x2w(name), 0, &type, reinterpret_cast<BYTE*>(&buffer[0]), &size);
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

		if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, x2t(path), 0, KEY_READ, &hkey) && hkey)
		{
			shared_ptr<void> key(hkey, &::RegCloseKey);
			return shared_ptr<hive>(new registry_hive(key));
		}
		throw runtime_error("Cannot open registry hive requested!");
	}
}
