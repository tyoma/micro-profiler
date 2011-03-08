#pragma once

#include "basics.h"
#include "mt.h"
#include "utilities.h"

#include <memory>

namespace fs
{
	typedef unsigned __int64 filetime;
	enum entry_type {	entry_none, entry_file, entry_directory	};

	struct directory_entry
	{
		entry_type type;
		std::wstring name;
		filetime created, modified, accessed;
	};

	class directory_iterator : noncopyable
	{
		void *_find_handle;
		directory_entry _current;

	public:
		directory_iterator(const std::wstring &path);
		~directory_iterator();

		operator bool() const;
		directory_iterator &operator ++();
		const directory_entry &operator *() const;
	};

	std::wstring operator /(std::wstring lhs, std::wstring rhs);
	std::wstring get_base_directory(const std::wstring &path);
	std::wstring get_filename(std::wstring path);
	entry_type get_entry_type(const std::wstring &path);
	bool get_filetimes(const std::wstring &path, filetime *created_at, filetime *modified_at, filetime *accessed_at);
	std::shared_ptr<mt::waitable> create_directory_monitor(const std::wstring &path, bool recursive);
	filetime parse_ctime_to_filetime(const std::string &ctime);


	inline std::wstring operator /(std::wstring lhs, std::wstring rhs)
	{
		trim_right(lhs, L"/\\");
		trim_left(rhs, L"/\\");
		return lhs + L"\\" + rhs;
	}

	inline std::wstring get_base_directory(const std::wstring &path)
	{	return path.substr(0, path.find_last_of(L"/\\"));	}

	inline std::wstring get_filename(std::wstring path)
	{
		path = path.substr(path.find_last_of(L"/\\"));
		trim_left(path, L"/\\");
		return path;
	}
}
