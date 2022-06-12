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

#include <common/compiler.h>
#include <common/hash.h>
#include <common/primitives.h>
#include <stdexcept>

namespace micro_profiler
{
	struct call_statistics : function_statistics
	{
		typedef std::vector<id_t> path_t;

		template <typename LookupT>
		const path_t &path(const LookupT &lookup) const;

		template <typename LookupT>
		unsigned int reentrance(const LookupT &lookup) const;

		id_t id;
		id_t thread_id;
		id_t parent_id;
		long_address_t address;

	private:
		template <typename LookupT>
		void initialize_path(const LookupT &lookup) const;

	private:
		mutable path_t _path;
		mutable unsigned int _reentrance;
	};

	struct symbol_key
	{
		unsigned int persistent_id;
		unsigned int rva;
	};



	template <typename LookupT>
	inline const call_statistics::path_t &call_statistics::path(const LookupT &lookup) const
	{	return _path.empty() ? initialize_path(lookup), _path : _path;	}

	template <typename LookupT>
	inline unsigned int call_statistics::reentrance(const LookupT &lookup) const
	{	return _path.empty() ? initialize_path(lookup), _reentrance : _reentrance;	}

	template <typename LookupT>
	FORCE_NOINLINE inline void call_statistics::initialize_path(const LookupT &lookup) const
	{
		auto item = this;

		for(_path.push_back(item->id), _reentrance = 0; item = lookup(item->parent_id), item; )
			_path.push_back(item->id), _reentrance += item->address == address;
		std::reverse(_path.begin(), _path.end());
	}


	inline bool operator ==(const symbol_key &lhs, const symbol_key &rhs)
	{	return !((lhs.persistent_id - rhs.persistent_id) | (lhs.rva - rhs.rva));	}
}

namespace std
{
	template <>
	struct hash<micro_profiler::symbol_key> : micro_profiler::knuth_hash
	{
		using micro_profiler::knuth_hash::operator ();

		size_t operator ()(const micro_profiler::symbol_key &value) const
		{	return (*this)((*this)(value.persistent_id), (*this)(value.rva));	}
	};

	template <typename T1, typename T2>
	struct hash< pair<T1, T2> > : micro_profiler::knuth_hash
	{	};
}
