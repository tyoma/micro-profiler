//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "primitives.h"

#include <common/hash.h>
#include <tuple>
#include <views/table.h>
#include <views/index.h>

namespace micro_profiler
{
	typedef std::tuple<id_t /*thread_id*/, id_t /*parent_id*/, long_address_t> call_node_key;
	typedef std::vector<long_address_t> callstack_key;

	namespace views
	{
		template <>
		class hash<call_node_key>
		{
		public:
			std::size_t operator ()(const call_node_key &key) const
			{	return knuth_hash()(std::get<0>(key)) ^ knuth_hash()(std::get<1>(key)) ^ knuth_hash()(std::get<2>(key));	}
		};

		template <typename T>
		class hash< std::vector<T> >
		{
		public:
			std::size_t operator ()(const std::vector<T> &key) const
			{
				size_t v = h((T()));

				for (auto i = key.begin(); i != key.end(); ++i)
					v = h(v, h(*i));
				return v;
			}

		private:
			knuth_hash h;
		};
	}

	struct call_statistics_constructor
	{
		call_statistics_constructor()
			: _next_id(1)
		{	}

		call_statistics operator ()()
		{
			call_statistics s;

			s.id = _next_id++;
			return s;
		}

	private:
		id_t _next_id;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, call_statistics_constructor &data, unsigned int ver);
	};

	struct call_node_keyer
	{
		typedef call_node_key key_type;

		key_type operator ()(const call_statistics &value) const
		{	return key_type(value.thread_id, value.parent_id, value.address);	}

		void operator ()(call_statistics &value, const key_type &key) const
		{	value.thread_id = std::get<0>(key), value.parent_id = std::get<1>(key), value.address = std::get<2>(key);	}
	};

	struct id_keyer
	{
		typedef id_t key_type;

		template <typename T>
		key_type operator ()(const T &value) const
		{	return value.id;	}
	};

	template <typename IndexT>
	class callstack_keyer
	{
	public:
		typedef callstack_key key_type;

	public:
		callstack_keyer(const IndexT &by_id)
			: _by_id(by_id)
		{	}

		template <typename T>
		const key_type &operator ()(const T &value) const
		{
			_key_buffer.clear();
			key_append(value);
			return _key_buffer;
		}

	private:
		template <typename T>
		inline void key_append(const T &entry) const
		{
			if (const auto parent_id = entry.parent_id)
				key_append(_by_id[parent_id]);
			_key_buffer.push_back(entry.address);
		}

	private:
		const IndexT &_by_id;
		mutable callstack_key _key_buffer;
	};

	typedef views::table<call_statistics, call_statistics_constructor> calls_statistics_table;
	typedef views::immutable_unique_index<calls_statistics_table, call_node_keyer> call_nodes_index;
	typedef views::immutable_unique_index<calls_statistics_table, id_keyer> primary_id_index;
	typedef views::immutable_index< calls_statistics_table, callstack_keyer<primary_id_index> > callstack_index;
}
