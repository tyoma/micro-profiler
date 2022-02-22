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

#include "tables.h"

#include <views/integrated_index.h>

namespace micro_profiler
{
	struct parent_id_keyer
	{
		typedef id_t key_type;

		key_type operator ()(const call_statistics &v) const
		{	return v.parent_id;	}
	};

	struct address_keyer
	{
		typedef std::pair<long_address_t, bool /*root*/> key_type;

		key_type operator ()(const call_statistics &v) const
		{	return std::make_pair(v.address, !v.parent_id);	}
	};


	template <typename U>
	class callees_transform
	{
	private:
		typedef views::immutable_index<U, parent_id_keyer> index_t;

	public:
		typedef typename index_t::const_iterator const_iterator;
		typedef typename index_t::const_reference const_reference;
		typedef typename index_t::range_type range_type;
		typedef typename index_t::value_type value_type;

	public:
		callees_transform(const U &underlying)
			: _underlying(underlying)
		{	}

		range_type equal_range(id_t id) const
		{	return views::multi_index<parent_id_keyer>(_underlying).equal_range(id);	}

		template <typename T1, typename T2>
		static const T2 &get(const T1 &, const T2 &value)
		{	return value;	}

	private:
		const U &_underlying;
	};

	template <typename U>
	class callers_transform
	{
	private:
		typedef views::immutable_index<U, address_keyer> by_address_index;

	public:
		typedef typename by_address_index::const_iterator const_iterator;
		typedef typename by_address_index::value_type const_reference;
		typedef typename by_address_index::range_type range_type;
		typedef typename by_address_index::value_type value_type;

	public:
		callers_transform(const U &underlying)
			: _underlying(underlying)
		{	}

		range_type equal_range(id_t id) const
		{
			auto &by_id = views::unique_index<id_keyer>(_underlying);
			auto &by_address = views::multi_index<address_keyer>(_underlying);

			if (auto self = by_id.find(id))
				return by_address.equal_range(std::make_pair(self->address, false));
			auto empty = by_address.equal_range(std::make_pair(0, false));
			return empty.first = empty.second, empty;
		}

		template <typename T1, typename T2>
		const T2 &get(const T1 &, const T2 &value) const
		{	return value;	}

	private:
		const U &_underlying;
	};
}
