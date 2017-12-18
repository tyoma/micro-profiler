#pragma once

#include <string>

namespace micro_profiler
{
	inline std::wstring operator &(const std::wstring &lhs, const std::wstring &rhs)
	{
		if (lhs.empty())
			return rhs;
		if (lhs.back() == L'\\' || lhs.back() == L'/')
			return lhs + rhs;
		return lhs + L"/" + rhs;
	}

	inline std::wstring operator ~(const std::wstring &value)
	{
		const size_t pos = value.find_last_of(L"\\/");

		if (pos != std::wstring::npos)
			return value.substr(0, pos);
		return std::wstring();
	}

	inline std::wstring operator *(const std::wstring &value)
	{
		const size_t pos = value.find_last_of(L"\\/");

		if (pos != std::wstring::npos)
			return value.substr(pos + 1);
		return value;
	}
}
