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

#include "module.h"
#include "noncopyable.h"
#include "primitives.h"
#include "protocol.h"
#include "range.h"

#include <strmd/deserializer.h>

namespace strmd
{
	using namespace micro_profiler;
	using namespace std;

	template <typename AddressT> struct is_container< statistics_map_detailed_t<AddressT> > { static const bool value = true; };
	template <typename AddressT> struct is_associative<statistics_map_detailed_t<AddressT>, true> { static const bool value = true; };

	template <typename KeyT, typename T> struct container_reader<unordered_map<KeyT, T, address_compare>, true>
	{
		typedef unordered_map<KeyT, T, address_compare> data_t;

		template <typename ArchiveT>
		void operator()(ArchiveT &archive, size_t count, data_t &data)
		{
			typename remove_const<typename data_t::key_type>::type key;

			while (count--)
			{
				archive(key);
				serialize_append_statistics(archive, data, key, data[key]);
			}
		}
	};

	template <typename AddressT> struct container_reader<statistics_map_detailed_t<AddressT>, true>
		: container_reader<unordered_map<AddressT, function_statistics_detailed_t<AddressT>, address_compare>, true>
	{	};

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, const void *&data)
	{	archive(reinterpret_cast<uintptr_t &>(data));	}
}

namespace micro_profiler
{
	typedef strmd::varint packer;

	class buffer_reader : noncopyable
	{
	public:
		buffer_reader(const_byte_range payload);

		void read(void *data, size_t size);

	private:
		const byte *_ptr;
		size_t _remaining;
	};

	template <typename BufferT>
	class buffer_writer : noncopyable
	{
	public:
		buffer_writer(BufferT &buffer);

		void write(const void *data, size_t size);

	private:
		BufferT &_buffer;
	};



	template <typename ArchiveT, typename ContainerT, typename AddressT>
	inline void serialize_append_statistics(ArchiveT &archive, ContainerT &/*container*/, AddressT /*key*/,
		function_statistics &data)
	{
		function_statistics v;

		archive(v);
		data += v;
	}

	template <typename ArchiveT, typename ContainerT, typename AddressT>
	inline void serialize_append_statistics(ArchiveT &archive, ContainerT &container, AddressT key,
		function_statistics_detailed_t<AddressT> &data)
	{
		serialize_append_statistics(archive, container, key, static_cast<function_statistics &>(data));
		archive(data.callees);
		update_parent_statistics(container, key, data);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, initialization_data &data)
	{
		archive(data.executable);
		archive(data.ticks_per_second);
	}	

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, function_statistics &data)
	{
		archive(data.times_called);
		archive(data.max_reentrance);
		archive(data.inclusive_time);
		archive(data.exclusive_time);
		archive(data.max_call_time);
	}

	template <typename ArchiveT, typename AddressT>
	inline void serialize(ArchiveT &archive, function_statistics_detailed_t<AddressT> &data)
	{
		archive(static_cast<function_statistics &>(data));
		archive(data.callees);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, commands &data)
	{	archive(reinterpret_cast<int &>(data));	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, mapped_module_identified &data)
	{
		archive(data.instance_id);
		archive(data.persistent_id);
		archive(data.base);
		archive(data.path);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_info &data)
	{
		archive(data.id);
		archive(data.name);
		archive(data.rva);
		archive(data.size);
		archive(data.file_id);
		archive(data.line);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, module_info_metadata &data)
	{
		archive(data.symbols);
		archive(data.source_files);
	}


	template <typename BufferT>
	inline buffer_writer<BufferT>::buffer_writer(BufferT &buffer)
		: _buffer(buffer)
	{	_buffer.clear();	}

	template <typename BufferT>
	inline void buffer_writer<BufferT>::write(const void *data, size_t size)
	{
		const byte *data_ = static_cast<const byte *>(data);

		if (size == 1)
			_buffer.push_back(*data_);
		else
			_buffer.append(data_, data_ + size);
	}
}
