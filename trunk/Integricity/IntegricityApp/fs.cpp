#include "fs.h"

#include <windows.h>

using namespace std;
using namespace mt;

namespace fs
{
	namespace
	{
		class change_notifier : public waitable
		{
			volatile HANDLE _change_notification;

		public:
			change_notifier(const wstring &path, bool recursive)
				: _change_notification(::FindFirstChangeNotificationW(path.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME))
			{
				if (INVALID_HANDLE_VALUE == _change_notification)
					throw runtime_error("Cannot create change wait object on the path specified!");
			}

			~change_notifier() throw()
			{	::FindCloseChangeNotification(_change_notification);	}

			virtual wait_status wait(unsigned int timeout) volatile
			{
				DWORD result = ::WaitForSingleObject(_change_notification, timeout);

				::FindNextChangeNotification(_change_notification);
				return WAIT_OBJECT_0 == result ? satisfied : waitable::timeout;
			}

			virtual void close() volatile
			{
				throw 0;
			}
		};
	}

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

	shared_ptr<waitable> create_change_notifier(const wstring &path, bool recursive)
	{	return shared_ptr<waitable>(new change_notifier(path, recursive));	}

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
