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

#include <unordered_map>

namespace micro_profiler
{
	namespace containers
	{
		template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>, typename CompT = std::equal_to<KeyT> >
		class unordered_map : private std::unordered_map<KeyT, ValueT, HashT, CompT>
		{
		private:
			typedef std::unordered_map<KeyT, ValueT, HashT, CompT> base_t;

		public:
			using typename base_t::key_type;
			using typename base_t::mapped_type;
			typedef std::pair<const KeyT, ValueT> value_type;
			using typename base_t::const_iterator;
			using typename base_t::iterator;

		public:
			using base_t::begin;
			using base_t::end;
			using base_t::insert;
			using base_t::clear;
			using base_t::empty;
			using base_t::size;
			using base_t::find;
			using base_t::operator [];
		};
	}
}
