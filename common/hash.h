//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <utility>

namespace micro_profiler
{
	template <std::size_t type_size>
	struct knuth_hash_fixed;

	struct knuth_hash
	{
		std::size_t operator ()(unsigned int key) const throw();
		std::size_t operator ()(unsigned long long int key) const throw();
		std::size_t operator ()(const void *key) const throw();
		template <typename T1, typename T2> std::size_t operator ()(const std::pair<T1, T2> &key) const throw();
	};



	// knuth_hash - inline definitions
	template <>
	struct knuth_hash_fixed<4>
	{	std::size_t operator ()(unsigned int key) const throw() {	return key * 0x9e3779b9u;	}	};

	template <>
	struct knuth_hash_fixed<8>
	{	std::size_t operator ()(unsigned long long int key) const throw() {	return static_cast<std::size_t>(key * 0x7FFFFFFFFFFFFFFFull);	}	};


	inline std::size_t knuth_hash::operator ()(unsigned int key) const throw()
	{	return knuth_hash_fixed<sizeof(key)>()(key);	}

	inline std::size_t knuth_hash::operator ()(unsigned long long int key) const throw()
	{	return knuth_hash_fixed<sizeof(key)>()(key);	}

	inline std::size_t knuth_hash::operator ()(const void *key) const throw()
	{	return knuth_hash_fixed<sizeof(std::size_t)>()(reinterpret_cast<std::size_t>(key) >> 4);	}

	template <typename T1, typename T2>
	inline std::size_t knuth_hash::operator ()(const std::pair<T1, T2> &key) const throw()
	{
		std::size_t seed = (*this)(key.first);

		seed ^= (*this)(key.second) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
		return seed;
	}
}
