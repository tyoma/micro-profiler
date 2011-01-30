#include "fs.h"

#include <windows.h>

using namespace std;

namespace
{
}

namespace fs
{
	entry_type get_entry_type(const wstring &path)
	{
		DWORD attributes = ::GetFileAttributesW(path.c_str());

		return attributes == INVALID_FILE_ATTRIBUTES ? entry_none : 0 != (attributes & FILE_ATTRIBUTE_DIRECTORY) ? entry_directory : entry_file;
	}

	bool get_filetimes(const wstring &path, filetime *created_at, filetime *modified_at, filetime *accessed_at)
	{
		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hfind(::FindFirstFileW(path.c_str(), &fd));

		if (hfind != INVALID_HANDLE_VALUE)
		{
			if (created_at)
				*created_at = reinterpret_cast<const filetime &>(fd.ftCreationTime);
			if (modified_at)
				*modified_at = reinterpret_cast<const filetime &>(fd.ftLastWriteTime);
			if (accessed_at)
				*accessed_at = reinterpret_cast<const filetime &>(fd.ftLastAccessTime);
			::FindClose(hfind);
			return true;
		}
		return false;
	}

	filetime parse_ctime_to_filetime(const string &ctime)
	{
		static string months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };
		char dow_str[4] = { 0 };
		char month_str[4] = { 0 };
		int year, month, day, hour, minute, second;

		sscanf(ctime.c_str(), "%3s %3s %2d %2d:%2d:%2d %4d", dow_str, month_str, &day, &hour, &minute, &second, &year);
		month = 1 + distance(months, find(months, months + _countof(months), month_str));

		filetime ft;
		SYSTEMTIME time = { year, month, 0, day, hour, minute, second, 0 };

		::SystemTimeToFileTime(&time, reinterpret_cast<FILETIME *>(&ft));
		return ft;
	}
}
