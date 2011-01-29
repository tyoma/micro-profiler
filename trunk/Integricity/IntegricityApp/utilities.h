#pragma once

#include <string>
#include <algorithm>
#include <iterator>

template <typename CharT>
inline void trim_right(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{
	size_t pos = s.find_last_not_of(trimmed);

	if (pos != std::basic_string<CharT>::npos)
		s.erase(pos + 1);
}

template <typename CharT>
inline void trim_right(std::basic_string<CharT> &s, const CharT *trimmed)
{	trim_right(s, std::basic_string<CharT>(trimmed));	}


template <typename CharT>
inline void trim_left(std::basic_string<CharT> &s, const std::basic_string<CharT> &trimmed)
{	s.erase(0, s.find_first_not_of(trimmed));	}

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


template <typename IteratorT, typename OutputIteratorT>
inline OutputIteratorT split(IteratorT begin, IteratorT end, typename std::iterator_traits<IteratorT>::value_type separator, OutputIteratorT target)
{
	using namespace std;

	do
	{
		IteratorT match = find(begin, end, separator);

		*target++ = make_pair(begin, match);
		if (match == end)
			break;
		begin = match;
	}	while (begin++ != end);
	return target;
}
