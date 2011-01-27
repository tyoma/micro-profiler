#pragma once

#include <string>

template <typename CharT>
inline void trim_right(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{	s.substr(0, s.find_last_not_of(trimmed));	}

template <typename CharT>
inline void trim_right(std::basic_string<CharT> &s, const CharT *trimmed)
{	trim_right(s, std::basic_string<CharT>(trimmed));	}

template <typename CharT>
inline void trim_left(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{	s.substr(s.find_first_not_of(trimmed));	}

template <typename CharT>
inline void trim_left(std::basic_string<CharT> &s, const CharT *trimmed)
{	trim_left(s, std::basic_string<CharT>(trimmed));	}

template <typename CharT>
inline void trim(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{
	trim_right(s, trimmed);
	trim_left(s, trimmed);
}

template <typename CharT>
inline void trim(std::basic_string<CharT> &s, const CharT *trimmed)
{	trim(s, std::basic_string<CharT>(trimmed));	}
