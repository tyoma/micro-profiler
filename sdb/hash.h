//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <tuple>
#include <unordered_map>

namespace sdb
{
	template <typename T>
	struct hash : std::hash<T>
	{	};

	template <typename T1, typename T2>
	struct hash< std::tuple<T1, T2> >
	{
		std::size_t operator ()(const std::tuple<T1, T2> &value) const
		{	return hash<T1>()(std::get<0>(value)) ^ hash<T2>()(std::get<1>(value));	}
	};

	template <typename T1, typename T2, typename T3>
	struct hash< std::tuple<T1, T2, T3> >
	{
		std::size_t operator ()(const std::tuple<T1, T2, T3> &value) const
		{	return hash<T1>()(std::get<0>(value)) ^ hash<T2>()(std::get<1>(value)) ^ hash<T3>()(std::get<2>(value));	}
	};

	template <typename T>
	struct hash<T *>
	{
		std::size_t operator ()(T *value) const
		{	return reinterpret_cast<std::size_t>(value);	}
	};

	template <typename T>
	struct iterator_hash
	{
		std::size_t operator ()(T value) const
		{	return reinterpret_cast<std::size_t>(&*value);	}
	};
}

