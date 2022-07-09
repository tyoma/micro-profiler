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

#include "index.h"
#include "table.h"

namespace micro_profiler
{
	namespace views
	{
		template <typename KeyerT, typename T, typename C>
		inline const immutable_unique_index<table<T, C>, KeyerT> &unique_index(const table<T, C> &table_)
		{
			typedef immutable_unique_index<table<T, C>, KeyerT> index_t;
			return table_.component([&table_] {	return new index_t(const_cast<table<T, C> &>(table_));	});
		}

		template <typename KeyerT, typename T, typename C>
		inline immutable_unique_index<table<T, C>, KeyerT> &unique_index(table<T, C> &table_, const KeyerT &keyer = KeyerT())
		{
			typedef immutable_unique_index<table<T, C>, KeyerT> index_t;
			return table_.component([&table_, keyer] {	return new index_t(table_, keyer);	});
		}

		template <typename KeyerT, typename T, typename C>
		inline const immutable_index<table<T, C>, KeyerT> &multi_index(const table<T, C> &table_, const KeyerT &keyer = KeyerT())
		{
			typedef immutable_index<table<T, C>, KeyerT> index_t;
			return table_.component([&table_, keyer] {	return new index_t(table_, keyer);	});
		}

		template <typename KeyerT, typename T, typename C>
		inline const ordered_index<table<T, C>, KeyerT> &ordered_index_(const table<T, C> &table_, const KeyerT &keyer = KeyerT())
		{
			typedef ordered_index<table<T, C>, KeyerT> index_t;
			return table_.component([&table_, keyer] {	return new index_t(table_, keyer);	});
		}
	}
}
