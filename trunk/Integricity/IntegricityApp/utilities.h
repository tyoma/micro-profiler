#pragma once

template <typename CharT>
void trim_right(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{
	s.substr(0, s.find_last_not_of(trimmed));
}

template <typename CharT>
void trim_left(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{
	s.substr(s.find_first_not_of(trimmed));
}

template <typename CharT>
void trim(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{
	trim_right(s, trimmed);
	trim_left(s, trimmed);
}

template <typename CharT>
std::basic_string<CharT> operator /(std::basic_string<CharT> lhs, std::basic_string<CharT> rhs)
{
	trim_right(lhs, L"/\\");
	trim_left(rhs, L"/\\");
	return lhs + L"\\" + rhs;
}
