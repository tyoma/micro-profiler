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

#include "primitives.h"

#include <common/hash.h>
#include <tuple>
#include <views/integrated_index.h>

namespace micro_profiler
{
	typedef std::tuple<id_t /*thread_id*/, id_t /*parent_id*/, long_address_t> call_node_key;
	typedef std::vector<long_address_t> callstack_key;

	template <>
	class views::hash<call_node_key>
	{
	public:
		std::size_t operator ()(const call_node_key &key) const
		{	return h(h(h(std::get<0>(key)), h(std::get<1>(key))), h(std::get<2>(key)));	}

	private:
		knuth_hash h;
	};

	template <typename T>
	class views::hash< std::vector<T> >
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

	namespace keyer
	{
		struct callnode
		{
			call_node_key operator ()(const call_statistics &record) const
			{	return call_node_key(record.thread_id, record.parent_id, record.address);	}

			template <typename IndexT>
			void operator ()(IndexT &/*index*/, call_statistics &record, const call_node_key &key) const
			{	record.thread_id = std::get<0>(key), record.parent_id = std::get<1>(key), record.address = std::get<2>(key);	}
		};

		struct id
		{
			template <typename T>
			id_t operator ()(const T &record) const
			{	return record.id;	}
		};

		struct parent_id
		{
			id_t operator ()(const call_statistics &record) const
			{	return record.parent_id;	}
		};

		struct address_and_rootness
		{
			std::pair<long_address_t, bool /*root*/> operator ()(const call_statistics &record) const
			{	return std::make_pair(record.address, !record.parent_id);	}
		};

		template <typename TableT>
		class callstack
		{
		public:
			callstack(const TableT &underlying)
				: _by_id(views::unique_index<id>(underlying))
			{	}

			template <typename T>
			const callstack_key &operator ()(const T &record) const
			{
				_key_buffer.clear();
				key_append(record);
				return _key_buffer;
			}

			template <typename IndexT, typename T>
			void operator ()(const IndexT &index, T &record, callstack_key key) const
			{
				record.thread_id = 0;
				record.address = key.back();
				key.pop_back();
				record.parent_id = key.empty() ? id_t() : index[key].id;
			}

		private:
			template <typename T>
			inline void key_append(const T &record) const
			{
				if (const auto parent_id = record.parent_id)
					key_append(_by_id[parent_id]);
				_key_buffer.push_back(record.address);
			}

		private:
			const views::immutable_unique_index<TableT, id> &_by_id;
			mutable callstack_key _key_buffer;
		};
	}

	typedef views::table<call_statistics, call_statistics_constructor> calls_statistics_table;
}
