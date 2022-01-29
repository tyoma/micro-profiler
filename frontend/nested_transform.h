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

#include <views/index.h>

namespace micro_profiler
{
	struct parent_id_keyer
	{
		typedef id_t key_type;

		key_type operator ()(const call_statistics &v) const
		{	return v.parent_id;	}
	};

	struct callees_transform : views::immutable_index<tables::statistics, parent_id_keyer>
	{
		typedef views::immutable_index<tables::statistics, parent_id_keyer> base;

		callees_transform(const tables::statistics &underlying)
			: base(underlying)
		{	}

		callees_transform(callees_transform &&other)
			: base(std::move(other))
		{	}

		template <typename T1, typename T2>
		static const T2 &get(const T1 &, const T2 &value)
		{	return value;	}
	};

	class callers_transform
	{
	private:
		struct address_keyer
		{
			typedef std::pair<long_address_t, bool /*root*/> key_type;

			key_type operator ()(const call_statistics &v) const
			{	return std::make_pair(v.address, !v.parent_id);	}
		};
		typedef views::immutable_index<tables::statistics, address_keyer> by_address_index;

	public:
		typedef by_address_index::const_iterator const_iterator;
		typedef by_address_index::value_type const_reference;
		typedef by_address_index::key_type key_type;
		typedef by_address_index::range_type range_type;
		typedef by_address_index::value_type value_type;

	public:
		callers_transform(const tables::statistics &underlying)
			: _underlying(underlying), _by_address(underlying)
		{	}

		callers_transform(callers_transform &&other)
			: _underlying(other._underlying), _by_address(std::move(other._by_address))
		{	}

		range_type equal_range(id_t id) const
		{
			if (auto self = _underlying.by_id.find(id))
				return _by_address.equal_range(std::make_pair(self->address, false));
			auto empty = _by_address.equal_range(std::make_pair(0, false));
			return empty.first = empty.second, empty;
		}

		template <typename T>
		call_statistics get(const T &, call_statistics value) const
		{
			value.address = _underlying.by_id[value.parent_id].address;
			return value;
		}

	private:
		const tables::statistics &_underlying;
		views::immutable_index<tables::statistics, address_keyer> _by_address;
	};
}
