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
}
