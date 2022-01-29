//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <functional>
#include <memory>
#include <string>
#include <utility>

#pragma warning(disable:4099) // older versions of VisualStudio define std::hash as class

namespace micro_profiler
{
	class file_id : std::pair<unsigned long long, unsigned long long>, std::shared_ptr<void>
	{
	public:
		file_id(const std::string &path);

		bool operator ==(const file_id &rhs) const;

	private:
		template <typename T>
		friend struct std::hash;
	};



	inline bool file_id::operator ==(const file_id &rhs) const
	{	return *this == static_cast<const std::pair<unsigned long long, unsigned long long> &>(rhs);	}
}

namespace std
{
	template <>
	struct hash<micro_profiler::file_id>
	{
		size_t operator ()(const micro_profiler::file_id &fid) const
		{
			return hash<micro_profiler::file_id::first_type>()(fid.first)
				^ (hash<micro_profiler::file_id::second_type>()(fid.second) << 1);
		}
	};
}
