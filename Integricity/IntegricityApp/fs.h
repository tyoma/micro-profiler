#pragma once

#include "utilities.h"

#include <string>

namespace fs
{
	enum entry_type {	entry_none, entry_file, entry_directory	};

	inline std::wstring operator /(std::wstring lhs, std::wstring rhs)
	{
		trim_right(lhs, L"/\\");
		trim_left(rhs, L"/\\");
		return lhs + L"\\" + rhs;
	}

	entry_type get_entry_type(const std::wstring &path);

	inline std::wstring get_base_directory(const std::wstring &path)
	{	return path.substr(0, path.find_last_of(L"/\\"));	}

	inline std::wstring get_filename(std::wstring path)
	{
		path = path.substr(path.find_last_of(L"/\\"));
		trim_left(path, L"/\\");
		return path;
	}
}
